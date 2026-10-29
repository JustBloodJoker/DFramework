#include "pch.h"

#include <World/World.h>
#include <Renderer/OfflineRenderer.h>
#include <Renderer/GlobalRenderThreadManager.h>

void World::SetOfflineRenderer(OfflineRenderer* renderer) {
    m_pRender = renderer;
    InitializeLoadedEntities();
}

void World::AddNotifyToPull(NRenderSystemNotifyType type) {
    if (std::find(m_vRenderSystemNotifies.begin(), m_vRenderSystemNotifies.end(), type) == m_vRenderSystemNotifies.end()) {
        m_vRenderSystemNotifies.push_back(type);
    }
}

std::vector<NRenderSystemNotifyType> World::GetRenderSystemNotifies() const {
    return m_vRenderSystemNotifies;
}

void World::ClearNotifies() {
    m_vRenderSystemNotifies.clear();
}

TDefaultCamera* World::CreateDefaultCamera() {
    return CreateEntity<TDefaultCamera>();
}

TDirectionalLight* World::CreateDirectionalLight() {
    return CreateEntity<TDirectionalLight>();
}

TPointLight* World::CreatePointLight() {
    return CreateEntity<TPointLight>();
}

TSpotLight* World::CreateSpotLight() {
    return CreateEntity<TSpotLight>();
}

TRectLight* World::CreateRectLight() {
    return CreateEntity<TRectLight>();
}

TSimplePlane* World::CreateSimplePlane() {
    return CreateEntity<TSimplePlane>();
}

TSimpleCube* World::CreateSimpleCube() {
    return CreateEntity<TSimpleCube>();
}

TSimpleCone* World::CreateSimpleCone() {
    return CreateEntity<TSimpleCone>();
}

TSimpleSphere* World::CreateSimpleSphere() {
    return CreateEntity<TSimpleSphere>();
}

TSimpleCylinder* World::CreateSimpleCylinder() {
    return CreateEntity<TSimpleCylinder>();
}

TScene* World::CreateScene(std::string path) {
    return CreateEntity<TScene>(std::move(path));
}

void World::InitRenderEntity(const std::shared_ptr<TRender>& render) {
    GlobalRenderThreadManager::GetInstance()->WaitIdle();
    render->RenderInit(m_pRender->GetDevice(), nullptr)->WaitForExecute();
    if (m_pRender->IsRTSupported()) {
        render->RenderInitDXR(m_pRender->GetDXRDevice(), {})->WaitForExecute();
    }
}

void World::InitializeLoadedEntities() {
    for (auto& entity : m_vEntities) {
        entity->SetWorld(this);
        entity->Init();
        if (auto renderEntity = std::dynamic_pointer_cast<TRender>(entity)) {
            InitRenderEntity(renderEntity);
        }
    }
}
