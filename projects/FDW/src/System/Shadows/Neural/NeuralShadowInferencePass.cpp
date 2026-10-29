#include <pch.h>

#include <MainRenderer/GlobalRenderThreadManager.h>
#include <MainRenderer/PSOManager.h>
#include <System/Shadows/Neural/NeuralShadowInferencePass.h>

#include <DirectML.h>
#include <dml_provider_factory.h>
#include <onnxruntime_c_api.h>

const OrtApi &NeuralShadowInferencePass::GetRuntimeApi() {
    return *OrtGetApiBase()->GetApi(ORT_API_VERSION);
}

bool NeuralShadowInferencePass::CheckRuntimeStatus(OrtStatus *status, std::string &error) {
    if (!status) return true;

    const auto &api = GetRuntimeApi();
    error = api.GetErrorMessage(status);
    api.ReleaseStatus(status);
    return false;
}

bool NeuralShadowInferencePass::RequireTensor(OrtSession *session, std::size_t index, bool input, const char *expectedName,
                                              const std::vector<std::int64_t> &expectedShape, std::string &error) {
    const auto &api = GetRuntimeApi();
    OrtAllocator *allocator = nullptr;
    if (!CheckRuntimeStatus(api.GetAllocatorWithDefaultOptions(&allocator), error)) return false;

    char *name = nullptr;
    auto *nameStatus = input ? api.SessionGetInputName(session, index, allocator, &name) : api.SessionGetOutputName(session, index, allocator, &name);
    if (!CheckRuntimeStatus(nameStatus, error)) return false;

    const bool hasExpectedName = name && std::string_view(name) == expectedName;
    allocator->Free(allocator, name);
    if (!hasExpectedName) {
        error = "neural-shadow tensor name mismatch";
        return false;
    }

    OrtTypeInfo *typeInfo = nullptr;
    auto *typeStatus = input ? api.SessionGetInputTypeInfo(session, index, &typeInfo) : api.SessionGetOutputTypeInfo(session, index, &typeInfo);
    if (!CheckRuntimeStatus(typeStatus, error)) return false;

    const OrtTensorTypeAndShapeInfo *tensorInfo = nullptr;
    if (!CheckRuntimeStatus(api.CastTypeInfoToTensorInfo(typeInfo, &tensorInfo), error)) {
        api.ReleaseTypeInfo(typeInfo);
        return false;
    }

    ONNXTensorElementDataType elementType = ONNX_TENSOR_ELEMENT_DATA_TYPE_UNDEFINED;
    std::size_t dimensionCount = 0u;
    bool valid = CheckRuntimeStatus(api.GetTensorElementType(tensorInfo, &elementType), error) &&
                 CheckRuntimeStatus(api.GetDimensionsCount(tensorInfo, &dimensionCount), error);
    std::vector<std::int64_t> shape(dimensionCount);
    if (valid && dimensionCount) valid = CheckRuntimeStatus(api.GetDimensions(tensorInfo, shape.data(), shape.size()), error);

    api.ReleaseTypeInfo(typeInfo);
    if (!valid)  return false;

    if (elementType != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT || shape != expectedShape) {
        error = "neural-shadow tensor shape/type mismatch";
        return false;
    }

    return true;
}

struct NeuralShadowInferencePass::Runtime {
    ~Runtime();
    void ResetBindings();

    wrl::ComPtr<IDMLDevice> m_xDmlDevice;
    
    const OrtDmlApi *m_pDmlApi = nullptr;
    
    OrtEnv *m_pEnvironment = nullptr;
    OrtSessionOptions *m_pOptions = nullptr;
    OrtSession *m_pSession = nullptr;
    OrtMemoryInfo *m_pMemory = nullptr;
    
    OrtValue *m_pBaseTensor = nullptr;
    OrtValue *m_pDescriptorTensor = nullptr;
    OrtValue *m_pHardTensor = nullptr;
    OrtValue *m_pOutputTensor = nullptr;
    
    void *m_pBaseAllocation = nullptr;
    void *m_pDescriptorAllocation = nullptr;
    void *m_pHardAllocation = nullptr;
    void *m_pOutputAllocation = nullptr;
    
    std::uint32_t m_uWidth = 0u;
    std::uint32_t m_uHeight = 0u;
};

void NeuralShadowInferencePass::Runtime::ResetBindings() {
    const auto& api = NeuralShadowInferencePass::GetRuntimeApi();
    if (m_pOutputTensor) {
        api.ReleaseValue(m_pOutputTensor);
        m_pOutputTensor = nullptr;
    }

    if (m_pHardTensor) {
        api.ReleaseValue(m_pHardTensor);
        m_pHardTensor = nullptr;
    }

    if (m_pDescriptorTensor) {
        api.ReleaseValue(m_pDescriptorTensor);
        m_pDescriptorTensor = nullptr;
    }

    if (m_pBaseTensor) {
        api.ReleaseValue(m_pBaseTensor);
        m_pBaseTensor = nullptr;
    }

    if (m_pDmlApi) {
        if (m_pOutputAllocation) {
            m_pDmlApi->FreeGPUAllocation(m_pOutputAllocation);
        }
        if (m_pHardAllocation) {
            m_pDmlApi->FreeGPUAllocation(m_pHardAllocation);
        }
        if (m_pDescriptorAllocation) {
            m_pDmlApi->FreeGPUAllocation(m_pDescriptorAllocation);
        }
        if (m_pBaseAllocation) {
            m_pDmlApi->FreeGPUAllocation(m_pBaseAllocation);
        }
    }

    m_pBaseAllocation = nullptr;
    m_pDescriptorAllocation = nullptr;
    m_pHardAllocation = nullptr;
    m_pOutputAllocation = nullptr;
    if (m_pSession) {
        api.ReleaseSession(m_pSession);
        m_pSession = nullptr;
    }

    if (m_pOptions) {
        api.ReleaseSessionOptions(m_pOptions);
        m_pOptions = nullptr;
    }

    m_uWidth = 0u;
    m_uHeight = 0u;
}

NeuralShadowInferencePass::Runtime::~Runtime() {
    ResetBindings();

    const auto& api = NeuralShadowInferencePass::GetRuntimeApi();
    if (m_pMemory) api.ReleaseMemoryInfo(m_pMemory);
    
    if (m_pEnvironment) api.ReleaseEnv(m_pEnvironment);
}

NeuralShadowInferencePass::NeuralShadowInferencePass(IShadowRenderServices &services) : m_pServices(&services) { }
NeuralShadowInferencePass::~NeuralShadowInferencePass() { Shutdown(); }

NeuralShadowInferencePass::State NeuralShadowInferencePass::Initialize(const std::filesystem::path &graph) {
    std::lock_guard lock(m_xMutex);
    if (m_xState == State::Ready) return m_xState;

    std::string error;
    if (!NeuralShadowModel::PackageLoader::LoadAndValidate(graph, m_xPackage, error)) {
        SetFailure(std::move(error));
        return m_xState;
    }

    auto *device = m_pServices->GetDevice();
    auto *dxr = m_pServices->GetDXRDevice();
    auto *queue = GlobalRenderThreadManager::GetInstance()->GetQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    if (!device || !dxr || !queue || !queue->GetQueue()) {
        SetFailure("neural shadows require a D3D12/DXR direct queue");
        return m_xState;
    }

    auto* psoManager = PSOManager::GetInstance();
    m_pNeuralRayPipeline = dynamic_cast<FD3DW::RTPipelineObject*>(psoManager->GetOrCreatePSOObject(PSOType::NeuralShadowRayTracing));
    if (!m_pNeuralRayPipeline) {
        SetFailure("neural-shadow DXR pipeline creation failed");
        return m_xState;
    }

    m_pNeuralRayBindingTable = std::make_unique<FD3DW::RTShaderBindingTable>(m_pNeuralRayPipeline);
    m_pNeuralRayBindingTable->InitSBT(dxr);

    m_pTensorPipeline = dynamic_cast<FD3DW::ComputePipelineObject*>(psoManager->GetOrCreatePSOObject(PSOType::NeuralShadowTensorPacking));
    if (!m_pTensorPipeline) {
        SetFailure("neural-shadow tensor pipeline creation failed");
        return m_xState;
    }

    m_pNeuralRayParameters = FD3DW::UploadBuffer<NeuralRayParameters>::CreateConstantBuffer(device, 16u);
    m_pTensorParams = FD3DW::UploadBuffer<TensorParams>::CreateConstantBuffer(device, 8u);
    m_pTexturePack = std::make_unique<FD3DW::SRV_UAVPacker>(device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV), 2u, 0u,
                                                            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, device);

    m_pRuntime = std::make_unique<Runtime>();
    const auto dmlResult = DMLCreateDevice(device, DML_CREATE_DEVICE_FLAG_NONE, IID_PPV_ARGS(m_pRuntime->m_xDmlDevice.ReleaseAndGetAddressOf()));
    if (FAILED(dmlResult)) {
        SetFailure("neural-shadow DML device creation failed");
        return m_xState;
    }

    const auto &api = GetRuntimeApi();
    if (!CheckRuntimeStatus(api.CreateEnv(ORT_LOGGING_LEVEL_WARNING, "fdw_neural_shadows", &m_pRuntime->m_pEnvironment), error) ||
        !CheckRuntimeStatus(api.GetExecutionProviderApi("DML", ORT_API_VERSION, reinterpret_cast<const void **>(&m_pRuntime->m_pDmlApi)), error) ||
        !CheckRuntimeStatus(api.CreateMemoryInfo("DML", OrtDeviceAllocator, 0, OrtMemTypeDefault, &m_pRuntime->m_pMemory), error)) {
        
        
        SetFailure(std::move(error));
        return m_xState;
    }

    m_xState = State::Ready;
    m_xRuntimeState.Status = m_xState;
    return m_xState;
}

void NeuralShadowInferencePass::SetInputResources(FD3DW::FResource *normal, ID3D12Device *device) {
    std::lock_guard lock(m_xMutex);
    m_pNormal = normal;

    if (m_pTexturePack && normal) m_pTexturePack->AddResource(normal->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 1u, device);
}

bool NeuralShadowInferencePass::RebuildSession(std::uint32_t width, std::uint32_t height) {
    auto &runtime = *m_pRuntime;
    runtime.ResetBindings();
    const auto &api = GetRuntimeApi();
    std::string error;
    
    if (!CheckRuntimeStatus(api.CreateSessionOptions(&runtime.m_pOptions), error) ||
        !CheckRuntimeStatus(api.DisableMemPattern(runtime.m_pOptions), error) ||
        !CheckRuntimeStatus(api.SetSessionExecutionMode(runtime.m_pOptions, ORT_SEQUENTIAL), error) ||
        !CheckRuntimeStatus(api.SetSessionGraphOptimizationLevel(runtime.m_pOptions, ORT_ENABLE_ALL), error) ||
        !CheckRuntimeStatus(api.AddSessionConfigEntry(runtime.m_pOptions, "session.disable_cpu_ep_fallback", "1"), error) ||
        !CheckRuntimeStatus(api.AddFreeDimensionOverrideByName(runtime.m_pOptions, "height", height), error) ||
        !CheckRuntimeStatus(api.AddFreeDimensionOverrideByName(runtime.m_pOptions, "width", width), error)) {
        
        
        SetFailure(std::move(error));
        return false;
    }

    auto *queue = GlobalRenderThreadManager::GetInstance()->GetQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    if (!queue || !queue->GetQueue() ||
        !CheckRuntimeStatus(runtime.m_pDmlApi->SessionOptionsAppendExecutionProvider_DML1(runtime.m_pOptions, runtime.m_xDmlDevice.Get(), queue->GetQueue()), error) ||
        !CheckRuntimeStatus( api.CreateSession(runtime.m_pEnvironment, m_xPackage.GraphPath.c_str(), runtime.m_pOptions, &runtime.m_pSession), error)) {
        
        if (error.empty()) error = "neural-shadow direct queue is unavailable";

        SetFailure(std::move(error));
        return false;
    }

    std::size_t inputCount = 0u;
    std::size_t outputCount = 0u;
    if (!CheckRuntimeStatus(api.SessionGetInputCount(runtime.m_pSession, &inputCount), error) ||
        !CheckRuntimeStatus(api.SessionGetOutputCount(runtime.m_pSession, &outputCount), error)) {
        
        SetFailure(std::move(error));
        return false;
    }
    if (inputCount != 3u || outputCount != 1u) {
        
        SetFailure("neural-shadow model I/O count mismatch");
        return false;
    }

    const std::int64_t h = height;
    const std::int64_t w = width;
    if (!RequireTensor(runtime.m_pSession, 0u, true, "base", {1, 9, h, w}, error) ||
        !RequireTensor(runtime.m_pSession, 1u, true, "descriptor", {1, 18, h, w}, error) ||
        !RequireTensor(runtime.m_pSession, 2u, true, "hard", {1, 1, h, w}, error) ||
        !RequireTensor(runtime.m_pSession, 0u, false, "visibility", {1, 1, h, w}, error) ||
        !CheckRuntimeStatus(runtime.m_pDmlApi->CreateGPUAllocationFromD3DResource(m_pBase->GetResource(), &runtime.m_pBaseAllocation), error) ||
        !CheckRuntimeStatus(runtime.m_pDmlApi->CreateGPUAllocationFromD3DResource(m_pDescriptor->GetResource(), &runtime.m_pDescriptorAllocation), error) ||
        !CheckRuntimeStatus(runtime.m_pDmlApi->CreateGPUAllocationFromD3DResource(m_pHard->GetResource(), &runtime.m_pHardAllocation), error) ||
        !CheckRuntimeStatus(runtime.m_pDmlApi->CreateGPUAllocationFromD3DResource(m_pDmlOutput->GetResource(), &runtime.m_pOutputAllocation), error)) {
        
        
        SetFailure(std::move(error));
        return false;
    }

    const std::int64_t baseShape[]{1, 9, h, w};
    const std::int64_t descriptorShape[]{1, 18, h, w};
    const std::int64_t scalarShape[]{1, 1, h, w};
    const auto pixels = static_cast<std::size_t>(width) * height;
    if (!CheckRuntimeStatus(api.CreateTensorWithDataAsOrtValue(runtime.m_pMemory, runtime.m_pBaseAllocation, pixels * 9u * sizeof(float),
                                                               baseShape, _countof(baseShape), ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
                                                               &runtime.m_pBaseTensor),error) ||
        !CheckRuntimeStatus(api.CreateTensorWithDataAsOrtValue(runtime.m_pMemory, runtime.m_pDescriptorAllocation,
                                                               pixels * 18u * sizeof(float), descriptorShape, _countof(descriptorShape),
                                                               ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &runtime.m_pDescriptorTensor), error) ||
        !CheckRuntimeStatus(api.CreateTensorWithDataAsOrtValue(runtime.m_pMemory, runtime.m_pHardAllocation, pixels * sizeof(float),
                                                               scalarShape, _countof(scalarShape), ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
                                                               &runtime.m_pHardTensor), error) ||
        !CheckRuntimeStatus(api.CreateTensorWithDataAsOrtValue(runtime.m_pMemory, runtime.m_pOutputAllocation, pixels * sizeof(float),
                                                               scalarShape, _countof(scalarShape), ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
                                                               &runtime.m_pOutputTensor), error)) {
        
        
        SetFailure(std::move(error));
        return false;
    }

    runtime.m_uWidth = width;
    runtime.m_uHeight = height;
    return true;
}

bool NeuralShadowInferencePass::EnsureResources(std::uint32_t width, std::uint32_t height, std::uint32_t lights) {
    if (width == m_uAllocatedWidth && height == m_uAllocatedHeight && lights == m_uAllocatedLights && m_pVisibility) return true;

    if (!width || !height || !lights)  return false;

    if (m_pRuntime) m_pRuntime->ResetBindings();

    auto *device = m_pServices->GetDevice();
    const auto pixels = static_cast<std::uint64_t>(width) * height;
    const auto lowPixels = static_cast<std::uint64_t>(FDW_SD::Model::LowExtent(width)) * FDW_SD::Model::LowExtent(height);
    const auto regions = static_cast<std::uint64_t>(FDW_SD::Spatial::RegionCountX(width)) * FDW_SD::Spatial::RegionCountY(height);
    const auto works = regions * lights;
    if (pixels > 16ull * 1024ull * 1024ull || works > 1'000'000ull) {
        SetFailure("neural-shadow frame exceeds supported capacity");
        return false;
    }

    m_pWorkItems = FD3DW::StructuredBuffer::CreateStructuredBuffer<FDW_SD::Spatial::WorkItem>(device, static_cast<UINT>(works), true);
    m_pOutputHeader = FD3DW::StructuredBuffer::CreateStructuredBuffer<FDW_SD::RuntimeOutput::OutputHeader>(device, 1u, true);
    m_pHardLow = FD3DW::StructuredBuffer::CreateStructuredBuffer<float>(device, static_cast<UINT>(lowPixels), false, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_pDescriptor = FD3DW::StructuredBuffer::CreateStructuredBuffer<float>(device, static_cast<UINT>(pixels * 18u), false, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_pBase = FD3DW::StructuredBuffer::CreateStructuredBuffer<float>(device, static_cast<UINT>(pixels * 9u), false, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_pHard = FD3DW::StructuredBuffer::CreateStructuredBuffer<float>(device, static_cast<UINT>(pixels), false, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_pFrameValidity = FD3DW::StructuredBuffer::CreateStructuredBuffer<std::uint32_t>(device, static_cast<UINT>(pixels), false, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_pDmlOutput = FD3DW::StructuredBuffer::CreateStructuredBuffer<float>(device, static_cast<UINT>(pixels), false, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_pVisibility = FD3DW::StructuredBuffer::CreateStructuredBuffer<float>(device, static_cast<UINT>(works * 4096u), false, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);                                                                      
    m_pValidity = FD3DW::StructuredBuffer::CreateStructuredBuffer<std::uint32_t>(device, static_cast<UINT>(works * 4096u), false, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
                                                                                 
    m_uAllocatedWidth = width;
    m_uAllocatedHeight = height;
    m_uAllocatedLights = lights;
    return RebuildSession(width, height);
}

std::shared_ptr<FD3DW::ExecutionHandle> NeuralShadowInferencePass::PrepareFrame(const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> &dependencies) {
    
    std::lock_guard lock(m_xMutex);
    if (m_xState != State::Ready) return nullptr;

    const auto width = static_cast<std::uint32_t>(std::max(1, m_pServices->GetSceneRenderWidth()));
    const auto height = static_cast<std::uint32_t>(std::max(1, m_pServices->GetSceneRenderHeight()));
    const auto lights = static_cast<std::uint32_t>(std::max(0, m_pServices->GetLightsCount()));
    if (!lights || !EnsureResources(width, height, lights)) return nullptr;

    const auto rx = FDW_SD::Spatial::RegionCountX(width);
    const auto ry = FDW_SD::Spatial::RegionCountY(height);
    
    m_vFrameWorkItems.clear();
    m_vFrameWorkItems.reserve(static_cast<std::size_t>(rx) * ry * lights);
    for (std::uint32_t region = 0; region < rx * ry; ++region) {
        for (std::uint32_t light = 0; light < lights; ++light) {
            FDW_SD::Spatial::WorkItem item{};
            const auto index = static_cast<std::uint32_t>(m_vFrameWorkItems.size());
            item.ScreenRegionIndex = region;
            item.LightIndex = light;
            item.ShadowMetaIndex = light;
            item.LowInputOffset = FDW_SD::Spatial::LowInputOffset(index);
            item.OutputOffset = FDW_SD::Spatial::OutputOffset(index);
            m_vFrameWorkItems.push_back(item);
        }
    }

    const auto generation = m_uNextGeneration++;
    m_xHeader = {};
    m_xHeader.GenerationLow = static_cast<std::uint32_t>(generation);
    m_xHeader.GenerationHigh = static_cast<std::uint32_t>(generation >> 32u);
    m_xHeader.ScreenWidth = width;
    m_xHeader.ScreenHeight = height;
    m_xHeader.RegionCountX = rx;
    m_xHeader.RegionCountY = ry;
    m_xHeader.WorkItemCount = static_cast<std::uint32_t>(m_vFrameWorkItems.size());
    m_xHeader.Completion = FDW_SD::RuntimeOutput::COMPLETION_INCOMPLETE;

    auto items = m_vFrameWorkItems;
    auto header = m_xHeader;
    auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>( D3D12_COMMAND_LIST_TYPE_DIRECT, [this, items = std::move(items), header](ID3D12GraphicsCommandList *list) {
        m_pWorkItems->UploadData(m_pServices->GetDevice(), list, items.data(), static_cast<UINT>(items.size()), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_pOutputHeader->UploadData(m_pServices->GetDevice(), list, &header, 1u, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    });

    return GlobalRenderThreadManager::GetInstance()->Submit(recipe, dependencies, true);
}

std::shared_ptr<FD3DW::ExecutionHandle> NeuralShadowInferencePass::ExecuteFrame(
    const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> &dependencies) {
    std::unique_lock lock(m_xMutex);
    if (m_xState != State::Ready || !m_pRuntime || !m_pNormal || !m_pVisibility) return nullptr;

    auto *depth = m_pServices->GetCurrentDepthResource();
    auto *tlas = m_pServices->GetTLASResource();
    if (!depth || !tlas) {
        SetFailure("neural-shadow depth/TLAS unavailable");
        return nullptr;
    }

    m_pTexturePack->AddResource(depth, 0u, m_pServices->GetDevice());

    NeuralRayParameters rayParameters{};
    rayParameters.ScreenWidth = m_uAllocatedWidth;
    rayParameters.ScreenHeight = m_uAllocatedHeight;
    rayParameters.LowWidth = FDW_SD::Model::LowExtent(m_uAllocatedWidth);
    rayParameters.LowHeight = FDW_SD::Model::LowExtent(m_uAllocatedHeight);
    rayParameters.LightCount = m_uAllocatedLights;
    
    TensorParams tensor{};
    tensor.ScreenWidth = rayParameters.ScreenWidth;
    tensor.ScreenHeight = rayParameters.ScreenHeight;
    tensor.LowWidth = rayParameters.LowWidth;
    tensor.LowHeight = rayParameters.LowHeight;
    tensor.RegionCountX = m_xHeader.RegionCountX;
    tensor.RegionCountY = m_xHeader.RegionCountY;
    tensor.LightCount = m_uAllocatedLights;
    
    const auto inverse = dx::XMMatrixInverse(nullptr, dx::XMMatrixTranspose(m_pServices->GetJitteredViewProjectionMatrix()));
    std::memcpy(rayParameters.InverseViewProjectionMatrix, &inverse, sizeof(rayParameters.InverseViewProjectionMatrix));
    std::memcpy(tensor.InverseViewProjectionMatrix, &inverse, sizeof(tensor.InverseViewProjectionMatrix));

    std::shared_ptr<FD3DW::ExecutionHandle> previous;
    for (std::uint32_t light = 0u; light < m_uAllocatedLights; ++light) {
        rayParameters.LightIndex = light;
        tensor.LightIndex = light;
        
        rayParameters.PassIndex = 0u;
        
        m_pNeuralRayParameters->CpyData(light * 2u, rayParameters);
        
        rayParameters.PassIndex = 1u;
        
        m_pNeuralRayParameters->CpyData(light * 2u + 1u, rayParameters);
        m_pTensorParams->CpyData(light, tensor);

        auto rt = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList4>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this, tlas, light](ID3D12GraphicsCommandList4 *list) {
            m_pNeuralRayPipeline->Bind(list);
                
            auto *currentDepth = m_pServices->GetCurrentDepthResource();
            currentDepth->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            m_pNormal->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            m_pHardLow->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            m_pDescriptor->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            m_pFrameValidity->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                
            ID3D12DescriptorHeap *heaps[]{m_pTexturePack->GetResult()->GetDescriptorPtr()};
            list->SetDescriptorHeaps(1u, heaps);
                
            list->SetComputeRootShaderResourceView(0u, tlas->GetGPUVirtualAddress());
            list->SetComputeRootShaderResourceView(1u, m_pServices->GetLightsBufferAddress());
            list->SetComputeRootDescriptorTable(2u, m_pTexturePack->GetResult()->GetGPUDescriptorHandle(0u));
            list->SetComputeRootUnorderedAccessView(3u, m_pHardLow->GetResource()->GetGPUVirtualAddress());
            list->SetComputeRootUnorderedAccessView(4u, m_pDescriptor->GetResource()->GetGPUVirtualAddress());
            list->SetComputeRootUnorderedAccessView(5u, m_pFrameValidity->GetResource()->GetGPUVirtualAddress());
            list->SetComputeRootConstantBufferView(6u, m_pNeuralRayParameters->GetGPULocation(light * 2u));
                
            list->DispatchRays(m_pNeuralRayBindingTable->GetDispatchRaysDesc(m_uAllocatedWidth, m_uAllocatedHeight, 1u));
                
            m_pDescriptor->ResourceUAVBarrier(list);
            m_pFrameValidity->ResourceUAVBarrier(list);
                
            list->SetComputeRootConstantBufferView(6u, m_pNeuralRayParameters->GetGPULocation(light * 2u + 1u));
                
            list->DispatchRays(m_pNeuralRayBindingTable->GetDispatchRaysDesc(m_uAllocatedWidth, m_uAllocatedHeight, 1u));
                
            m_pHardLow->ResourceUAVBarrier(list);
            m_pDescriptor->ResourceUAVBarrier(list);
            m_pFrameValidity->ResourceUAVBarrier(list);
        });
        
        auto rtDependencies = previous ? std::vector{previous} : dependencies;
        previous = GlobalRenderThreadManager::GetInstance()->Submit(rt, std::move(rtDependencies), true);
        
        auto feature = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this, light](ID3D12GraphicsCommandList *list) {
            m_pTensorPipeline->Bind(list);
               
            m_pHardLow->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            m_pDescriptor->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            m_pServices->GetLightsBuffer()->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            m_pDmlOutput->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_COMMON);
            m_pBase->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            m_pHard->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            m_pFrameValidity->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                
            ID3D12DescriptorHeap *heaps[]{m_pTexturePack->GetResult()->GetDescriptorPtr()};
            list->SetDescriptorHeaps(1u, heaps);
                
            list->SetComputeRootShaderResourceView(0u, m_pHardLow->GetResource()->GetGPUVirtualAddress());
            list->SetComputeRootShaderResourceView(1u, m_pDescriptor->GetResource()->GetGPUVirtualAddress());
            list->SetComputeRootShaderResourceView(2u, m_pServices->GetLightsBufferAddress());
            list->SetComputeRootShaderResourceView(3u, m_pDmlOutput->GetResource()->GetGPUVirtualAddress());
            list->SetComputeRootDescriptorTable(4u, m_pTexturePack->GetResult()->GetGPUDescriptorHandle(0u));
            list->SetComputeRootUnorderedAccessView(5u, m_pBase->GetResource()->GetGPUVirtualAddress());
            list->SetComputeRootUnorderedAccessView(6u, m_pHard->GetResource()->GetGPUVirtualAddress());
            list->SetComputeRootUnorderedAccessView(7u, m_pFrameValidity->GetResource()->GetGPUVirtualAddress());
            list->SetComputeRootUnorderedAccessView(8u, m_pVisibility->GetResource()->GetGPUVirtualAddress());
            list->SetComputeRootUnorderedAccessView(9u, m_pValidity->GetResource()->GetGPUVirtualAddress());
            list->SetComputeRootConstantBufferView(10u, m_pTensorParams->GetGPULocation(light));
            list->SetComputeRoot32BitConstant(11u, 0u, 0u);
                
            list->Dispatch((m_uAllocatedWidth + 7u) / 8u, (m_uAllocatedHeight + 7u) / 8u, 1u);
                
            m_pBase->ResourceUAVBarrier(list);
            m_pHard->ResourceUAVBarrier(list);
            m_pFrameValidity->ResourceUAVBarrier(list);
                
            m_pBase->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_COMMON);
            m_pHard->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_COMMON);
            m_pDescriptor->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_COMMON);
        });

        previous = GlobalRenderThreadManager::GetInstance()->Submit(feature, {previous}, true);
        
        previous = GlobalRenderThreadManager::GetInstance()->SubmitGpuSynchronizedLambda( [this]() {
            const char *inputNames[]{"base", "descriptor", "hard"};
            const OrtValue *inputs[]{m_pRuntime->m_pBaseTensor, m_pRuntime->m_pDescriptorTensor, m_pRuntime->m_pHardTensor};
            const char *outputNames[]{"visibility"};
            OrtValue *outputs[]{m_pRuntime->m_pOutputTensor};
            
            std::string error;
            if (!CheckRuntimeStatus(GetRuntimeApi().Run(m_pRuntime->m_pSession, nullptr, inputNames, inputs, _countof(inputs),
                                                        outputNames, _countof(outputs), outputs), error)) {
            
                SetFailure(std::move(error));
            }
        }, {previous}, true);
        
        auto assemble = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this, light](ID3D12GraphicsCommandList *list) {
            m_pTensorPipeline->Bind(list);
                
            m_pDmlOutput->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            m_pHardLow->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            m_pDescriptor->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            m_pBase->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            m_pHard->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            m_pFrameValidity->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            m_pVisibility->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            m_pValidity->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                
            ID3D12DescriptorHeap *heaps[]{m_pTexturePack->GetResult()->GetDescriptorPtr()};
            list->SetDescriptorHeaps(1u, heaps);
                
            list->SetComputeRootShaderResourceView(0u, m_pHardLow->GetResource()->GetGPUVirtualAddress());
            list->SetComputeRootShaderResourceView(1u, m_pDescriptor->GetResource()->GetGPUVirtualAddress());
            list->SetComputeRootShaderResourceView(2u, m_pServices->GetLightsBufferAddress());
            list->SetComputeRootShaderResourceView(3u, m_pDmlOutput->GetResource()->GetGPUVirtualAddress());
            list->SetComputeRootDescriptorTable(4u, m_pTexturePack->GetResult()->GetGPUDescriptorHandle(0u));
            list->SetComputeRootUnorderedAccessView(5u, m_pBase->GetResource()->GetGPUVirtualAddress());
            list->SetComputeRootUnorderedAccessView(6u, m_pHard->GetResource()->GetGPUVirtualAddress());
            list->SetComputeRootUnorderedAccessView(7u, m_pFrameValidity->GetResource()->GetGPUVirtualAddress());
            list->SetComputeRootUnorderedAccessView(8u, m_pVisibility->GetResource()->GetGPUVirtualAddress());
            list->SetComputeRootUnorderedAccessView(9u, m_pValidity->GetResource()->GetGPUVirtualAddress());
            list->SetComputeRootConstantBufferView(10u, m_pTensorParams->GetGPULocation(light));
            list->SetComputeRoot32BitConstant(11u, 1u, 0u);
                
            list->Dispatch((m_uAllocatedWidth + 7u) / 8u, (m_uAllocatedHeight + 7u) / 8u, 1u);
                
            m_pVisibility->ResourceUAVBarrier(list);
            m_pValidity->ResourceUAVBarrier(list);
            m_pVisibility->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            m_pValidity->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        });
        previous = GlobalRenderThreadManager::GetInstance()->Submit(assemble, {previous}, true);
    }

    auto header = m_xHeader;
    header.Completion = FDW_SD::RuntimeOutput::COMPLETION_COMPLETE;
    
    auto finalize = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this, header](ID3D12GraphicsCommandList *list) {
        m_pOutputHeader->UploadData(m_pServices->GetDevice(), list, &header, 1u, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    });

    previous = GlobalRenderThreadManager::GetInstance()->Submit(finalize, {previous}, true);
    
    m_xHeader = header;
    return previous;
}

void NeuralShadowInferencePass::SetFailure(std::string message) {
    m_xState = State::Error;
    m_xRuntimeState.Status = m_xState;
    m_xRuntimeState.LastError = std::move(message);
}

bool NeuralShadowInferencePass::IsReady() const {
    std::lock_guard lock(m_xMutex);
    return m_xState == State::Ready;
}

bool NeuralShadowInferencePass::AreResourcesAllocated() const {
    std::lock_guard lock(m_xMutex);
    return m_pBase && m_pDescriptor && m_pVisibility;
}

std::uint32_t NeuralShadowInferencePass::GetConsumerMode() const {
    return IsReady() ? 2u : 0u;
}

NeuralShadowInferencePass::RuntimeState NeuralShadowInferencePass::GetRuntimeState() const {
    std::lock_guard lock(m_xMutex);
    return m_xRuntimeState;
}

D3D12_GPU_VIRTUAL_ADDRESS NeuralShadowInferencePass::GetWorkItemsAddress() const {
    return m_pWorkItems ? m_pWorkItems->GetResource()->GetGPUVirtualAddress() : 0u;
}

D3D12_GPU_VIRTUAL_ADDRESS NeuralShadowInferencePass::GetOutputHeaderAddress() const {
    return m_pOutputHeader ? m_pOutputHeader->GetResource()->GetGPUVirtualAddress() : 0u;
}

D3D12_GPU_VIRTUAL_ADDRESS NeuralShadowInferencePass::GetVisibilityAddress() const {
    return m_pVisibility ? m_pVisibility->GetResource()->GetGPUVirtualAddress() : 0u;
}

D3D12_GPU_VIRTUAL_ADDRESS NeuralShadowInferencePass::GetValidityAddress() const {
    return m_pValidity ? m_pValidity->GetResource()->GetGPUVirtualAddress() : 0u;
}

void NeuralShadowInferencePass::Shutdown() {
    std::lock_guard lock(m_xMutex);
    if (m_xState == State::Uninitialized) return;

    GlobalRenderThreadManager::GetInstance()->WaitIdle();
    
    m_pRuntime.reset();
    m_pValidity.reset();
    m_pVisibility.reset();
    m_pDmlOutput.reset();
    m_pFrameValidity.reset();
    m_pHard.reset();
    m_pBase.reset();
    m_pDescriptor.reset();
    m_pHardLow.reset();
    m_pOutputHeader.reset();
    m_pWorkItems.reset();
    m_pTensorParams.reset();
    m_pNeuralRayParameters.reset();
    m_pTexturePack.reset();
    m_pTensorPipeline = nullptr;
    m_pNeuralRayBindingTable.reset();
    m_pNeuralRayPipeline = nullptr;

    m_xState = State::Uninitialized;
    m_xRuntimeState.Status = m_xState;
}

