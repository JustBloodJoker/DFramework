#include <Component/RenderObject/AnimationComponent.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>
#include <World/World.h>

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
    Activate(true);
}


void AnimationComponent::Stop() {
	if (m_sCurrentAnimation.empty()) return;

	m_sCurrentAnimation.clear();
	m_bNeedResetBonesBuffer = true;
}


bool AnimationComponent::IsPlaying() {
	return !m_sCurrentAnimation.empty();
}

void AnimationComponent::SetScene(ID3D12Device* device, FD3DW::Scene* scene) {
	m_pScene = scene;
    if (m_pScene && m_pScene->GetBonesCount()) {
        m_pStructureBufferBones = FD3DW::StructuredBuffer::CreateStructuredBuffer<dx::XMMATRIX>(device, UINT(m_pScene->GetBonesCount()), false);
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
    }

}

std::weak_ptr<bool> AnimationComponent::IsBonesActive() {
    return m_bIsBonesActive;
}
