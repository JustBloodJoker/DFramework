#include <World/World.h>
#include <MainRenderer/MainRenderer.h>
#include <WinWindow/Utils/Scripting/ScriptManager.h>
#include <Utilities/HelperFunctions.h>
#include <algorithm>

void World::SetMainRenderer(MainRenderer* mr) {
	m_pRender = mr;
	AfterSetMainRenderer();
	SetupScriptingBindings();
	RestoreScriptPredicatesAfterLoad();
}

MainRenderer* World::GetMainRenderer() {
	return m_pRender;
}

void World::AddNotifyToPull(NRenderSystemNotifyType type) {
	if (std::find(m_vRenderSystemNotifies.begin(), m_vRenderSystemNotifies.end(), type) != m_vRenderSystemNotifies.end()) return;

	m_vRenderSystemNotifies.push_back(type);
}

std::vector<NRenderSystemNotifyType> World::GetRenderSystemNotifies() const {
	return m_vRenderSystemNotifies;
}

void World::ClearNotifies() {
	m_vRenderSystemNotifies.clear();
}

ComponentHolder* World::CreateEmptyEntity() {
	return CreateEntity<ComponentHolder>();
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

TSkybox* World::CreateSkybox(std::string path) {
	return CreateEntity<TSkybox>(path);
}

TScene* World::CreateScene(std::string path) {
	return CreateEntity<TScene>(path);
}

TAudio* World::CreateAudio(std::string path) {
	return CreateEntity<TAudio>(path);
}

ComponentHolder* World::GetEntityByName(std::string name) {
	for (auto& entity : m_vEntities) {
		if (entity && entity->GetName() == name) {
			return entity.get();
		}
	}
	return nullptr;
}

void World::SetEntityName(ComponentHolder* entity, std::string name) {
	if (!entity) return;
	entity->SetName(name);
}

void World::SetEntityActive(ComponentHolder* entity, int active) {
	if (!entity) return;
	entity->Activate(active != 0);
}

void World::SetMeshPosition(ComponentHolder* entity, float x, float y, float z) {
	if (auto* mesh = dynamic_cast<TMesh*>(entity)) {
		mesh->SetPosition({ x, y, z });
	}
}

void World::SetMeshRotation(ComponentHolder* entity, float x, float y, float z) {
	if (auto* mesh = dynamic_cast<TMesh*>(entity)) {
		mesh->SetRotation({ x, y, z });
	}
}

void World::SetMeshScale(ComponentHolder* entity, float x, float y, float z) {
	if (auto* mesh = dynamic_cast<TMesh*>(entity)) {
		mesh->SetScale({ x, y, z });
	}
}

void World::TranslateMesh(ComponentHolder* entity, float dx, float dy, float dz) {
	if (auto* mesh = dynamic_cast<TMesh*>(entity)) {
		auto pos = mesh->GetPosition();
		mesh->SetPosition({ pos.x + dx, pos.y + dy, pos.z + dz });
	}
}

void World::SetLightColor(ComponentHolder* entity, float r, float g, float b) {
	if (auto* light = dynamic_cast<TLight*>(entity)) {
		light->SetLightColor({ r, g, b });
	}
}

void World::SetLightIntensity(ComponentHolder* entity, float intensity) {
	if (auto* light = dynamic_cast<TLight*>(entity)) {
		light->SetLightIntensity(intensity);
	}
}

void World::SetLightPosition(ComponentHolder* entity, float x, float y, float z) {
	if (auto* pointLight = dynamic_cast<TPointLight*>(entity)) {
		pointLight->SetLightPosition({ x, y, z });
		return;
	}
	if (auto* spotLight = dynamic_cast<TSpotLight*>(entity)) {
		spotLight->SetLightPosition({ x, y, z });
		return;
	}
	if (auto* rectLight = dynamic_cast<TRectLight*>(entity)) {
		rectLight->SetLightPosition({ x, y, z });
	}
}

void World::SetLightDirection(ComponentHolder* entity, float x, float y, float z) {
	if (auto* directionalLight = dynamic_cast<TDirectionalLight*>(entity)) {
		directionalLight->SetLightDirection({ x, y, z });
		return;
	}
	if (auto* spotLight = dynamic_cast<TSpotLight*>(entity)) {
		spotLight->SetLightDirection({ x, y, z });
	}
}

void World::SetLightAttenuationRadius(ComponentHolder* entity, float radius) {
	if (auto* pointLight = dynamic_cast<TPointLight*>(entity)) {
		pointLight->SetLightAttenuationRadius(radius);
		return;
	}
	if (auto* spotLight = dynamic_cast<TSpotLight*>(entity)) {
		spotLight->SetLightAttenuationRadius(radius);
	}
}

void World::SetLightSourceRadius(ComponentHolder* entity, float radius) {
	if (auto* pointLight = dynamic_cast<TPointLight*>(entity)) {
		pointLight->SetLightSourceRadius(radius);
	}
}

void World::SetLightConeAngles(ComponentHolder* entity, float inner, float outer) {
	if (auto* spotLight = dynamic_cast<TSpotLight*>(entity)) {
		spotLight->SetLightInnerConeAngle(inner);
		spotLight->SetLightOuterConeAngle(outer);
	}
}

void World::SetRectLightSize(ComponentHolder* entity, float x, float y) {
	if (auto* rectLight = dynamic_cast<TRectLight*>(entity)) {
		rectLight->SetLightRectSize({ x, y });
	}
}

void World::SetRectLightRotation(ComponentHolder* entity, float x, float y, float z) {
	if (auto* rectLight = dynamic_cast<TRectLight*>(entity)) {
		rectLight->SetLightRotation({ x, y, z });
	}
}

void World::SetDefaultCameraEye(ComponentHolder* entity, float x, float y, float z) {
	if (auto* camera = dynamic_cast<TDefaultCamera*>(entity)) {
		camera->SetEyePosition(x, y, z);
	}
}

void World::SetDefaultCameraYawPitchRoll(ComponentHolder* entity, float yaw, float pitch, float roll) {
	if (auto* camera = dynamic_cast<TDefaultCamera*>(entity)) {
		camera->SetYawPitchRoll(yaw, pitch, roll);
	}
}

void World::SetCameraSpeed(ComponentHolder* entity, float speed) {
	if (auto* camera = dynamic_cast<TBaseCamera*>(entity)) {
		camera->SetCameraSpeed(speed);
	}
}

void World::ResetCamera(ComponentHolder* entity) {
	if (auto* camera = dynamic_cast<TBaseCamera*>(entity)) {
		camera->ResetPosition();
	}
}

void World::MoveDefaultCameraForward(ComponentHolder* entity, float dt) {
	if (auto* camera = dynamic_cast<TDefaultCamera*>(entity)) {
		camera->MoveForward(dt);
	}
}

void World::MoveDefaultCameraBackward(ComponentHolder* entity, float dt) {
	if (auto* camera = dynamic_cast<TDefaultCamera*>(entity)) {
		camera->MoveBackward(dt);
	}
}

void World::MoveDefaultCameraLeft(ComponentHolder* entity, float dt) {
	if (auto* camera = dynamic_cast<TDefaultCamera*>(entity)) {
		camera->StrafeLeft(dt);
	}
}

void World::MoveDefaultCameraRight(ComponentHolder* entity, float dt) {
	if (auto* camera = dynamic_cast<TDefaultCamera*>(entity)) {
		camera->StrafeRight(dt);
	}
}

void World::PlayAudio(ComponentHolder* entity) {
	if (auto* audio = dynamic_cast<TAudio*>(entity)) {
		audio->Play();
	}
}

void World::StopAudio(ComponentHolder* entity) {
	if (auto* audio = dynamic_cast<TAudio*>(entity)) {
		audio->Stop();
	}
}

void World::RestartAudio(ComponentHolder* entity) {
	if (auto* audio = dynamic_cast<TAudio*>(entity)) {
		audio->Restart();
	}
}

void World::SetAudioVolume(ComponentHolder* entity, float volume) {
	if (auto* audio = dynamic_cast<TAudio*>(entity)) {
		audio->SetVolume(volume);
	}
}

void World::SetAudioLoop(ComponentHolder* entity, int loop) {
	if (auto* audio = dynamic_cast<TAudio*>(entity)) {
		audio->Loop(loop != 0);
	}
}

void World::SetPreDepthEnabled(int enabled) {
	if (!m_pRender) return;
	m_pRender->EnablePreDepth(enabled != 0);
}

void World::SetBloomEnabled(int enabled) {
	if (!m_pRender) return;
	m_pRender->EnableBloom(enabled != 0);
}

void World::SetBloomThreshold(float threshold) {
	if (!m_pRender) return;
	auto data = m_pRender->GetBrightPassData();
	data.Threshold = threshold;
	m_pRender->SetBrightPassData(data);
}

void World::SetBloomIntensity(float intensity) {
	if (!m_pRender) return;
	auto data = m_pRender->GetCompositeData();
	data.BloomIntensity = intensity;
	m_pRender->SetCompositeData(data);
}

void World::SetBloomBlurType(int blurType) {
	if (!m_pRender) return;
	auto clamped = std::clamp(blurType, int(BloomBlurType::Horizontal), int(BloomBlurType::Both));
	m_pRender->SetBloomBlurType(BloomBlurType(clamped));
}

void World::LoadWorldFile(std::string path) {
	if (!m_pRender) return;
	m_pRender->LoadWorld(path);
}

void World::SaveWorldFile(std::string path) {
	if (!m_pRender) return;
	m_pRender->SaveActiveWorld(path);
}

int World::ExecuteScriptFile(std::string path) {
	auto normalized = FDWUtils::NormalizePath(path);
	auto id = ScriptManager::GetInstance()->ExecuteFile(normalized);
	if (id != 0) {
		RememberExecutedScript(normalized);
	}
	return int(id);
}

void World::UpdateScripts(float dt) {
	ScriptManager::GetInstance()->Update(dt);
}

const std::vector<WorldScriptRecord>& World::GetLoadedScripts() const {
	return m_vLoadedScripts;
}

std::vector<ComponentHolder*> World::GetEntities() {
	std::vector<ComponentHolder*> result;
	result.reserve(m_vEntities.size());
	for (auto& e : m_vEntities) {
		result.push_back(e.get());
	}
	return result;
}

void World::DestroyEntity(ComponentHolder* holder) {
	if (auto* renderEntity = dynamic_cast<TRender*>(holder)) {
		GlobalRenderThreadManager::GetInstance()->WaitIdle();
	}

	std::erase_if(m_vEntities, [&](const std::shared_ptr<ComponentHolder>& ent) {
		if (ent.get() == holder) {
			ent->Destroy();
			ent->SetWorld(nullptr);
			return true;
		}
		return false;
	});
}

void World::InitRenderEntity(std::shared_ptr<TRender> render) {
	GlobalRenderThreadManager::GetInstance()->WaitIdle();
	render->RenderInit(m_pRender->GetDevice(), nullptr)->WaitForExecute();
	if (m_pRender->IsRTSupported()) render->RenderInitDXR(m_pRender->GetDXRDevice(), {})->WaitForExecute();
}

void World::AfterSetMainRenderer() {
	for (auto entity : m_vEntities) {
		entity->Init();
		if (auto renderEntity = std::dynamic_pointer_cast<TRender>(entity)) {
			InitRenderEntity(renderEntity);
		}
	}
}

void World::SetupScriptingBindings() {
	auto* scriptManager = ScriptManager::GetInstance();
	scriptManager->StopAllScripts();
	scriptManager->ClearRegisteredObjects();
	scriptManager->RegisterObject("world", this, "World");
}

void World::RestoreScriptPredicatesAfterLoad() {
	for (auto& script : m_vLoadedScripts) {
		if (!script.WasExecuted) continue;
		auto normalized = FDWUtils::NormalizePath(script.Path);
		ScriptManager::GetInstance()->ExecuteFile(normalized, ScriptExecutionMode::PredicatesOnly);
		script.Path = normalized;
	}
}

void World::RememberExecutedScript(const std::string& path) {
	auto normalized = FDWUtils::NormalizePath(path);
	if (normalized.empty()) return;

	auto it = std::find_if(m_vLoadedScripts.begin(), m_vLoadedScripts.end(),
		[&normalized](const WorldScriptRecord& script) {
			return script.Path == normalized;
		}
	);
	if (it != m_vLoadedScripts.end()) {
		it->WasExecuted = true;
	}
	else {
		m_vLoadedScripts.push_back(WorldScriptRecord{ normalized, true });
	}
}

