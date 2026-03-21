#include <MainRenderer/MainRenderer.h>
#include <System/LightSystem.h>
#include <Component/Light/LightComponent.h>
#include <D3DFramework/GraphicUtilites/FResource.h>


void LightSystem::AfterConstruction() {
    MainRendererComponent::AfterConstruction();

    m_pLightsHelperConstantBuffer = FD3DW::UploadBuffer< LightSystemBuffer>::CreateConstantBuffer(m_pOwner->GetDevice(), 1u);
    m_pLightsStructuredBuffer = FD3DW::StructuredBuffer::CreateStructuredBuffer<LightComponentData>(m_pOwner->GetDevice(), 1u, true);

    GlobalRenderThreadManager::GetInstance()->Submit(
        std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, 
            [this](ID3D12GraphicsCommandList* list) mutable {
                m_pIBLBrdfLUTResource = CreateIBLBrdfLutResource(m_pOwner->GetDevice(), list);

                m_pOwner->UpdateIBL_LUT_Resource();
        } )
    );
}

int LightSystem::GetLightsCount() {
    return m_xLightBuffer.LightCount;
}

FD3DW::StructuredBuffer* LightSystem::GetLightsBuffer() {
    return m_pLightsStructuredBuffer.get();
}

D3D12_GPU_VIRTUAL_ADDRESS LightSystem::GetLightsStructuredBufferGPULocation() {
    return m_pLightsStructuredBuffer->GetResource()->GetGPUVirtualAddress();
}

D3D12_GPU_VIRTUAL_ADDRESS LightSystem::GetLightsConstantBufferGPULocation() {
    return m_pLightsHelperConstantBuffer->GetGPULocation(0);
}

FD3DW::FResource* LightSystem::GetIBLBrdfLUTResource() {
    return m_pIBLBrdfLUTResource.get();
}

const std::vector<LightComponentData>& LightSystem::GetLightComponentsData() const { 
    return m_vLightComponentsData; 
}

bool LightSystem::IsEnabledIBL() {
    return m_bIsIBLEnabled;
}

void LightSystem::EnableIBL(bool b) {
    m_bIsIBLEnabled = b;
}

float LightSystem::GetIBLDiffuseIntensity() const {
    return m_fIBLDiffuseIntensity;
}

void LightSystem::SetIBLDiffuseIntensity(float value) {
    m_fIBLDiffuseIntensity = std::max(0.0f, value);
}

float LightSystem::GetIBLSpecularIntensity() const {
    return m_fIBLSpecularIntensity;
}

void LightSystem::SetIBLSpecularIntensity(float value) {
    m_fIBLSpecularIntensity = std::max(0.0f, value);
}

float LightSystem::GetIBLMaxReflectionMip() const {
    return m_fIBLMaxReflectionMip;
}

void LightSystem::SetIBLMaxReflectionMip(float value) {
    m_fIBLMaxReflectionMip = std::clamp(value, 0.0f, 16.0f);
}

std::shared_ptr<FD3DW::ExecutionHandle> LightSystem::OnStartRenderTick(std::shared_ptr<FD3DW::ExecutionHandle> syncHandle) {
    auto updateRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, 
        [this](ID3D12GraphicsCommandList* list) mutable {
            auto components = GetWorld()->GetAllComponentsOfType<LightComponent>();

            m_xLightBuffer.CameraPos = m_pOwner->GetCurrentCameraPosition();
            m_xLightBuffer.IsShadowImpl = (int)m_pOwner->IsShadowEnabled();
            m_xLightBuffer.ZNear = m_pOwner->GetCameraFrustum().GetZNear();
			m_xLightBuffer.ZFar = m_pOwner->GetCameraFrustum().GetZFar();
			m_xLightBuffer.FrameIndex = m_pOwner->GetFrameIndex();
			m_xLightBuffer.IsIBLEnabled = int(m_pOwner->IsEnabledIBL());
			m_xLightBuffer.IBLDiffuseIntensity = m_pOwner->GetIBLDiffuseIntensity();
			m_xLightBuffer.IBLSpecularIntensity = m_pOwner->GetIBLSpecularIntensity();
			m_xLightBuffer.IBLMaxReflectionMip = m_pOwner->GetIBLMaxReflectionMip();
			m_xLightBuffer.InverseViewProjectionMatrix = dx::XMMatrixInverse( nullptr, dx::XMMatrixTranspose( m_pOwner->GetJitteredViewProjectionMatrix() ) );

            if (m_bIsNeedUpdateDataInBuffer.exchange(false, std::memory_order_acq_rel)) {
                auto device = m_pOwner->GetDevice();
                m_vLightComponentsData = GetDataFromLightComponents(components);
                m_xLightBuffer.LightCount = int(m_vLightComponentsData.size());
                m_pLightsStructuredBuffer->UploadData(device, list, m_vLightComponentsData.data(), m_xLightBuffer.LightCount, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            }

            m_pLightsHelperConstantBuffer->CpyData(0, m_xLightBuffer);
    });
    
    return GlobalRenderThreadManager::GetInstance()->Submit(updateRecipe, { syncHandle }, true);
}

void LightSystem::ProcessNotify(NRenderSystemNotifyType type) {
    if (type == NRenderSystemNotifyType::Light || type == NRenderSystemNotifyType::LightUpdateData) {
        m_bIsNeedUpdateDataInBuffer.store(true, std::memory_order_relaxed);
    }
}

float LightSystem::RadicalInverseVdC(UINT bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f;
}

dx::XMFLOAT2 LightSystem::Hammersley(UINT i, UINT n) {
    return dx::XMFLOAT2(float(i) / float(n), RadicalInverseVdC(i));
}

dx::XMFLOAT3 LightSystem::ImportanceSampleGGX(const dx::XMFLOAT2& xi, float roughness) {
    auto a = roughness * roughness;

    auto phi = 2.0f * M_PI_F * xi.x;
    auto cosTheta = std::sqrt((1.0f - xi.y) / (1.0f + (a * a - 1.0f) * xi.y));
    auto sinTheta = std::sqrt(std::max(1.0f - cosTheta * cosTheta, 0.0f));

    return dx::XMFLOAT3(std::cos(phi) * sinTheta, std::sin(phi) * sinTheta, cosTheta);
}

float LightSystem::GeometrySchlickGGX_IBL(float nDotV, float roughness) {
    auto a = roughness * roughness;
    auto k = 0.5f * a;
    auto denom = nDotV * (1.0f - k) + k;
    return nDotV / std::max(denom, 1e-5f);
}

float LightSystem::GeometrySmith_IBL(float nDotV, float nDotL, float roughness) {
    return GeometrySchlickGGX_IBL(nDotV, roughness) * GeometrySchlickGGX_IBL(nDotL, roughness);
}

dx::XMFLOAT2 LightSystem::IntegrateBRDF(float nDotV, float roughness) {
    dx::XMFLOAT3 V(std::sqrt(std::max(1.0f - nDotV * nDotV, 0.0f)), 0.0f, nDotV);

    float A = 0.0f;
    float B = 0.0f;
    for (UINT i = 0; i < IBL_BRDF_LUT_SAMPLES; ++i) {
        auto xi = Hammersley(i, IBL_BRDF_LUT_SAMPLES);
        auto H = ImportanceSampleGGX(xi, roughness);
        auto vDotH = FD3DW::Clamp01(FD3DW::Dot3(V, H));

        auto L = FD3DW::Normalize3(FD3DW::Add3(FD3DW::Scale3(H, 2.0f * vDotH), FD3DW::Scale3(V, -1.0f)));
        auto nDotL = FD3DW::Clamp01(L.z);
        auto nDotH = FD3DW::Clamp01(H.z);

        if (nDotL > 0.0f) {
            auto G = GeometrySmith_IBL(nDotV, nDotL, roughness);
            auto G_Vis = (G * vDotH) / std::max(nDotH * nDotV, 1e-5f);
            auto Fc = std::pow(1.0f - vDotH, 5.0f);

            A += (1.0f - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    auto invSamples = 1.0f / float(IBL_BRDF_LUT_SAMPLES);
    return dx::XMFLOAT2(A * invSamples, B * invSamples);
}

std::shared_ptr<FD3DW::FResource> LightSystem::CreateIBLBrdfLutResource(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
    auto texture = FD3DW::FResource::CreateAnonimTexture(
        device,
        1,
        DXGI_FORMAT_R32G32_FLOAT,
        IBL_BRDF_LUT_SIZE,
        IBL_BRDF_LUT_SIZE,
        DXGI_SAMPLE_DESC({ 1, 0 }),
        D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        D3D12_RESOURCE_FLAG_NONE,
        D3D12_TEXTURE_LAYOUT_UNKNOWN,
        D3D12_HEAP_FLAG_NONE,
        &FD3DW::keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
        1
    );

    std::vector<float> pixels(size_t(IBL_BRDF_LUT_SIZE) * size_t(IBL_BRDF_LUT_SIZE) * 2u, 0.0f);
    for (UINT y = 0; y < IBL_BRDF_LUT_SIZE; ++y) {
        auto roughness = (float(y) + 0.5f) / float(IBL_BRDF_LUT_SIZE);
        for (UINT x = 0; x < IBL_BRDF_LUT_SIZE; ++x) {
            auto nDotV = (float(x) + 0.5f) / float(IBL_BRDF_LUT_SIZE);
            auto brdf = IntegrateBRDF(FD3DW::Clamp01(nDotV), FD3DW::Clamp01(roughness));
            auto index = (size_t(y) * size_t(IBL_BRDF_LUT_SIZE) + size_t(x)) * 2u;
            pixels[index + 0] = brdf.x;
            pixels[index + 1] = brdf.y;
        }
    }

    texture->UploadData(device, list, pixels.data());
    return std::shared_ptr<FD3DW::FResource>(texture.release());
}

std::vector<LightComponentData> LightSystem::GetDataFromLightComponents(std::vector<LightComponent*> cmps) {
    std::vector<LightComponentData> ret;
    for (const auto& cmp : cmps) {
        if( cmp->IsActive() ) ret.push_back(cmp->GetLightComponentData());
    }
    return ret;
}
