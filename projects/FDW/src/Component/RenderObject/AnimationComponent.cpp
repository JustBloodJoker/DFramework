#include <Component/RenderObject/AnimationComponent.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>
#include <World/World.h>
#include <MainRenderer/PSOManager.h>

AnimationComponent::AnimationComponent() {
    m_sName = "AnimationComponent";
}

void AnimationComponent::Init() {
    IComponent::Init();
    m_bIsBonesActive = std::make_shared<bool>();
    *m_bIsBonesActive = false;
    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::SceneAnimationActivationDeactivation);
}

void AnimationComponent::Destroy() {
    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::SceneAnimationActivationDeactivation);
}

void AnimationComponent::Activate(bool b) {
    b &= ( m_pStructureBufferBones != nullptr );
    IComponent::Activate(b);
    *m_bIsBonesActive = b;
    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::SceneAnimationActivationDeactivation);
}

bool AnimationComponent::IsFreeze() {
	return m_bNeedFreezeBonesBuffer;
}

void AnimationComponent::Freeze(bool b) {
	m_bNeedFreezeBonesBuffer = b;
}


std::vector<std::string> AnimationComponent::GetAnimations() {
    static std::vector<std::string> s_sEmpty;
    return m_pScene ? m_pScene->GetAnimations() : s_sEmpty;
}

void AnimationComponent::Play(std::string animName) {
	if (m_sCurrentAnimation == animName) return;

	m_sCurrentAnimation = animName;
	m_fAnimationTime = 0.f;
	m_bNeedResetBonesBuffer = false;

    EnableAnimation(true);

    Activate(true);
}


void AnimationComponent::Stop() {
	if (m_sCurrentAnimation.empty()) return;

	m_sCurrentAnimation.clear();
	m_bNeedResetBonesBuffer = true;
    
    EnableAnimation(false);
}


bool AnimationComponent::IsPlaying() {
	return !m_sCurrentAnimation.empty();
}

void AnimationComponent::SetScene(ID3D12Device* device, ID3D12GraphicsCommandList* list, FD3DW::Scene* scene, FD3DW::IObjectVertexIndexDataCreator* sceneRenderData) {
	m_pScene = scene;
    m_pSceneCreatorData = sceneRenderData;
    if (m_pScene && m_pScene->GetBonesCount()) {
        m_pStructureBufferBones = FD3DW::StructuredBuffer::CreateStructuredBuffer<dx::XMMATRIX>(device, UINT(m_pScene->GetBonesCount()), false);

        const auto &vertices = m_pScene->GetVertices();
        auto verSize = UINT(vertices.size());
        m_pStructuredSceneVertexBuffer = FD3DW::StructuredBuffer::CreateStructuredBuffer<FD3DW::SceneVertexFrameWork>(device, verSize, false,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        m_pStructuredSceneVertexBuffer->UploadData(device, list, vertices.data(), verSize, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        m_pGPUSkinningConstantBuffer = FD3DW::UploadBuffer<AnimationComponentGPUConstantBufferData>::CreateConstantBuffer(device, 1);


        m_xData.NumBones = UINT(m_pScene->GetBonesCount());
        m_xData.VertexCount = verSize;
        m_xData.AnimationEnabled = 0;
        m_pGPUSkinningConstantBuffer->CpyData(0, m_xData);
    }
}

FD3DW::FResource* AnimationComponent::GetResource() {
    return m_pStructureBufferBones.get();
}

void AnimationComponent::OnAnimationUpdateTick(const AnimationComponentInputData& data) {
    if (!m_pScene || !m_pStructureBufferBones || m_bNeedFreezeBonesBuffer) return;

    std::vector<dx::XMMATRIX> dataVec;

    if (!m_sCurrentAnimation.empty()) {
        dataVec = m_pScene->PlayAnimation(m_fAnimationTime, m_sCurrentAnimation);
        m_fAnimationTime += data.DT;
    }
    else if (m_bNeedResetBonesBuffer) {
        dataVec.resize(m_pScene->GetBonesCount());
        Activate(false);
        m_bNeedResetBonesBuffer = false;
    }

    if (!dataVec.empty()) {
        m_pStructureBufferBones->UploadData(
            data.Device, data.CommandList,
            dataVec.data(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );



        m_bCallGPUCulling.store(true, std::memory_order_relaxed);
    }
}

void AnimationComponent::ProcessGPUSkinning(ID3D12GraphicsCommandList* computeList) {
    if (!IsPlaying() || !m_bCallGPUCulling.exchange(false, std::memory_order_acq_rel)) return;

    PSOManager::GetInstance()->GetPSOObject(PSOType::GPUSceneSkinning)->Bind(computeList);

    computeList->SetComputeRootShaderResourceView(GPU_SKINNING_PASS_BONES_TRANSFORM_INPUT_POS_IN_ROOT_SIG, m_pStructureBufferBones->GetResource()->GetGPUVirtualAddress());
    computeList->SetComputeRootShaderResourceView(GPU_SKINNING_PASS_VERTICES_SRV_INPUT_POS_IN_ROOT_SIG, m_pStructuredSceneVertexBuffer->GetResource()->GetGPUVirtualAddress());
    computeList->SetComputeRootUnorderedAccessView(GPU_SKINNING_PASS_VERTICES_UAV_OUTPUT_POS_IN_ROOT_SIG, m_pSceneCreatorData->GetVertexBufferResource()->GetGPUVirtualAddress());
    computeList->SetComputeRootConstantBufferView(GPU_SKINNING_PASS_SCENE_CBV_INPUT_POS_IN_ROOT_SIG, m_pGPUSkinningConstantBuffer->GetGPULocation(0));

    int dispatches = ( int( m_pScene->GetVertices().size() ) + GPU_SKINNING_DISPATCH_X_COUNT - 1) / GPU_SKINNING_DISPATCH_X_COUNT;
    computeList->Dispatch(dispatches,1,1);

    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::UpdateBLAS);
}

std::weak_ptr<bool> AnimationComponent::IsBonesActive() {
    return m_bIsBonesActive;
}

void AnimationComponent::EnableAnimation(bool b) {
    m_xData.AnimationEnabled = (UINT)b;
    m_pGPUSkinningConstantBuffer->CpyData(0, m_xData);
}
