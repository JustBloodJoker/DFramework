#pragma once

#include <D3DFramework/GraphicUtilites/BufferManager.h>
#include <D3DFramework/GraphicUtilites/ComputePipelineObject.h>
#include <D3DFramework/GraphicUtilites/RTPipelineObject.h>
#include <D3DFramework/GraphicUtilites/RTShaderBindingTable.h>
#include <D3DFramework/GraphicUtilites/RenderThreadUtils/ExecutionHandle.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>
#include <ShadowData/NeuralShadowData.h>
#include <System/Shadows/Neural/NeuralShadowModelPackage.h>
#include <System/Shadows/ShadowRenderServices.h>

struct OrtStatus;
struct OrtApi;
struct OrtSession;

class NeuralShadowInferencePass {
    public:
    explicit NeuralShadowInferencePass(IShadowRenderServices &services);
    ~NeuralShadowInferencePass();
    
    public:
    enum class State : std::uint32_t {
        Uninitialized,
        Ready,
        Unavailable,
        Error
    };
    struct RuntimeState {
        State Status = State::Uninitialized;
        std::string LastError;
    };

    public:
    State Initialize(const std::filesystem::path &graph);
    void SetInputResources(FD3DW::FResource *normal, ID3D12Device *device);
    
    std::shared_ptr<FD3DW::ExecutionHandle> PrepareFrame(const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> &dependencies = {});
    std::shared_ptr<FD3DW::ExecutionHandle> ExecuteFrame(const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> &dependencies);

    public:
    bool IsReady() const;
    
    std::uint32_t GetConsumerMode() const;
    RuntimeState GetRuntimeState() const;
    D3D12_GPU_VIRTUAL_ADDRESS GetWorkItemsAddress() const;
    D3D12_GPU_VIRTUAL_ADDRESS GetOutputHeaderAddress() const;
    D3D12_GPU_VIRTUAL_ADDRESS GetVisibilityAddress() const;
    D3D12_GPU_VIRTUAL_ADDRESS GetValidityAddress() const;
    
    public:
    void Shutdown();

    protected:
    struct Runtime; // in .cpp file

    struct NeuralRayParameters {
        std::uint32_t ScreenWidth;
        std::uint32_t ScreenHeight;
        std::uint32_t LowWidth;
        std::uint32_t LowHeight;
        std::uint32_t LightIndex;
        std::uint32_t LightCount;
        std::uint32_t PassIndex;
        std::uint32_t Padding;
        float InverseViewProjectionMatrix[16];
    };

    protected:
    struct TensorParams {
        std::uint32_t ScreenWidth;
        std::uint32_t ScreenHeight;
        std::uint32_t LowWidth;
        std::uint32_t LowHeight;
        std::uint32_t RegionCountX;
        std::uint32_t RegionCountY;
        std::uint32_t LightIndex;
        std::uint32_t LightCount;
        float InverseViewProjectionMatrix[16];
    };

    protected:
    bool EnsureResources(std::uint32_t width, std::uint32_t height, std::uint32_t lights);
    bool RebuildSession(std::uint32_t width, std::uint32_t height);
    void BindTensorResources(ID3D12GraphicsCommandList* list, std::uint32_t light, std::uint32_t pass);
    void SetFailure(std::string message);
    static const OrtApi &GetRuntimeApi();
    static bool CheckRuntimeStatus(OrtStatus *status, std::string &error);
    static bool RequireTensor(OrtSession *session, std::size_t index, bool input, const char *expectedName,
                              const std::vector<std::int64_t> &expectedShape, std::string &error);

    protected:
    IShadowRenderServices *m_pServices = nullptr;
    mutable std::mutex m_xMutex;

    State m_xState = State::Uninitialized;
    RuntimeState m_xRuntimeState;

    NeuralShadowModel::Package m_xPackage;

    std::unique_ptr<Runtime> m_pRuntime;

    FD3DW::RTPipelineObject* m_pNeuralRayPipeline = nullptr;
    std::unique_ptr<FD3DW::RTShaderBindingTable> m_pNeuralRayBindingTable;
    FD3DW::ComputePipelineObject* m_pTensorPipeline = nullptr;
    std::unique_ptr<FD3DW::SRV_UAVPacker> m_pTexturePack;

    std::unique_ptr<FD3DW::UploadBuffer<NeuralRayParameters>> m_pNeuralRayParameters;
    std::unique_ptr<FD3DW::UploadBuffer<TensorParams>> m_pTensorParams;

    std::unique_ptr<FD3DW::StructuredBuffer> m_pWorkItems;
    std::unique_ptr<FD3DW::StructuredBuffer> m_pOutputHeader;
    std::unique_ptr<FD3DW::StructuredBuffer> m_pHardLow;
    std::unique_ptr<FD3DW::StructuredBuffer> m_pDescriptor;
    std::unique_ptr<FD3DW::StructuredBuffer> m_pBase;
    std::unique_ptr<FD3DW::StructuredBuffer> m_pHard;
    std::unique_ptr<FD3DW::StructuredBuffer> m_pFrameValidity;
    std::unique_ptr<FD3DW::StructuredBuffer> m_pDmlOutput;
    std::unique_ptr<FD3DW::StructuredBuffer> m_pVisibility;
    std::unique_ptr<FD3DW::StructuredBuffer> m_pValidity;

    FD3DW::FResource *m_pNormal = nullptr;

    std::vector<FDW_SD::Spatial::WorkItem> m_vFrameWorkItems;
    FDW_SD::RuntimeOutput::OutputHeader m_xHeader{};

    std::uint64_t m_uNextGeneration = 1u;
    std::uint32_t m_uAllocatedWidth = 0u;
    std::uint32_t m_uAllocatedHeight = 0u;
    std::uint32_t m_uAllocatedLights = 0u;
};
