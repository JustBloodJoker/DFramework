#include <World/World.h>
#include <MainRenderer/MainRenderer.h>
#include <WinWindow/Utils/Scripting/ScriptManager.h>
#include <WinWindow/Utils/Scripting/ScriptValue.h>
#include <Utilities/HelperFunctions.h>
#include <Component/Light/LightComponent.h>
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
	if (name.empty()) return nullptr;

	auto cachedIt = m_mEntityByName.find(name);
	if (cachedIt != m_mEntityByName.end()) {
		auto cached = cachedIt->second;
		if (cached && cached->GetName() == name) {
			return cached;
		}

		m_mEntityByName.erase(cachedIt);
	}

	for (auto& entity : m_vEntities) {
		if (entity && entity->GetName() == name) {
			m_mEntityByName[name] = entity.get();
			return entity.get();
		}
	}
	return nullptr;
}

void World::SetEntityName(ComponentHolder* entity, std::string name) {
	if (!entity) return;

	RemoveEntityNameFromCache(entity);
	entity->SetName(name);
	AddEntityNameToCache(entity);
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

namespace {
    ScriptArray* AsScriptArray(void* ptr) {
        return reinterpret_cast<ScriptArray*>(ptr);
    }

    const ScriptArray* AsScriptArrayConst(void* ptr) {
        return reinterpret_cast<const ScriptArray*>(ptr);
    }

    bool TryReadFloatAt(const ScriptArray* arr, int idx, float& outValue) {
        if (!arr) return false;
        if (idx < 0 || idx >= static_cast<int>(arr->Values.size())) return false;
        outValue = arr->Values[static_cast<size_t>(idx)].AsFloat();
        return true;
    }

    bool TryReadIntAt(const ScriptArray* arr, int idx, int& outValue) {
        if (!arr) return false;
        if (idx < 0 || idx >= static_cast<int>(arr->Values.size())) return false;
        outValue = arr->Values[static_cast<size_t>(idx)].AsInt();
        return true;
    }

    ComponentHolder* ReadEntityAt(const ScriptArray* arr, int idx) {
        if (!arr) return nullptr;
        if (idx < 0 || idx >= static_cast<int>(arr->Values.size())) return nullptr;
        auto* obj = arr->Values[static_cast<size_t>(idx)].AsObject();
        return static_cast<ComponentHolder*>(obj);
    }

    void WriteFloatAt(ScriptArray* arr, int idx, float value) {
        if (!arr) return;
        if (idx < 0) return;
        if (idx >= static_cast<int>(arr->Values.size())) {
            arr->Values.resize(static_cast<size_t>(idx) + 1);
        }
        arr->Values[static_cast<size_t>(idx)] = value;
    }

    float RandRangeNative(float minValue, float maxValue) {
        static thread_local uint32_t seed = 0x9E3779B9u;
        seed = seed * 1664525u + 1013904223u;
        const float t = static_cast<float>((seed >> 8) & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
        return minValue + (maxValue - minValue) * t;
    }
}

void World::ApplyScriptLightsState(void* lightsArray,void* xArray,void* yArray,void* zArray,void* intensityArray,void* kindArray,void* dirXArray,void* dirYArray,void* dirZArray,int beginIndex,int endIndex) {
    const auto* lights = AsScriptArrayConst(lightsArray);
    const auto* xs = AsScriptArrayConst(xArray);
    const auto* ys = AsScriptArrayConst(yArray);
    const auto* zs = AsScriptArrayConst(zArray);
    const auto* intensities = AsScriptArrayConst(intensityArray);
    const auto* kinds = AsScriptArrayConst(kindArray);
    const auto* dirXs = AsScriptArrayConst(dirXArray);
    const auto* dirYs = AsScriptArrayConst(dirYArray);
    const auto* dirZs = AsScriptArrayConst(dirZArray);

    if (!lights || !xs || !ys || !zs || !intensities) return;

    const int minIndex = std::max(beginIndex, 0);
    const int maxLightIndex = static_cast<int>(lights->Values.size()) - 1;
    const int maxIndex = std::min(endIndex, maxLightIndex);
    if (minIndex > maxIndex) return;

    bool anyUpdated = false;

    for (int i = minIndex; i <= maxIndex; ++i) {
        auto* entity = ReadEntityAt(lights, i);
        if (!entity) continue;

        auto* lightCmp = entity->GetComponent<LightComponent>();
        if (!lightCmp || !lightCmp->IsActive()) continue;

        float px = 0.0f;
        float py = 0.0f;
        float pz = 0.0f;
        float intensity = 0.0f;
        if (!TryReadFloatAt(xs, i, px) || !TryReadFloatAt(ys, i, py) || !TryReadFloatAt(zs, i, pz) || !TryReadFloatAt(intensities, i, intensity)) {
            continue;
        }

        auto data = lightCmp->GetLightComponentData();
        data.Position = { px, py, pz };
        data.Intensity = intensity;

        int kind = 0;
        if (TryReadIntAt(kinds, i, kind) && kind == 1) {
            float dx = 0.0f;
            float dy = 0.0f;
            float dz = 0.0f;
            if (TryReadFloatAt(dirXs, i, dx) && TryReadFloatAt(dirYs, i, dy) && TryReadFloatAt(dirZs, i, dz)) {
                data.Direction = { dx, dy, dz };
            }
        }

        lightCmp->SetLightComponentDataSilent(data);
        anyUpdated = true;
    }

    if (anyUpdated) {
        AddNotifyToPull(NRenderSystemNotifyType::LightUpdateData);
    }
}

void World::UpdateScriptLightsNative(
    void* lightsArray,
    void* kindArray,
    void* xArray,
    void* yArray,
    void* zArray,
    void* txArray,
    void* tyArray,
    void* tzArray,
    void* spdArray,
    void* distLimArray,
    void* yMinLimArray,
    void* yMaxLimArray,
    void* intenArray,
    void* idirArray,
    void* heatArray,
    void* iminArray,
    void* imaxArray,
    void* dirXArray,
    void* dirYArray,
    void* dirZArray,
    void* dirLerpArray,
    float dt,
    int beginIndex,
    int endIndex
) {
    if (dt <= 0.0f) return;

    auto* lights = AsScriptArray(lightsArray);
    auto* kinds = AsScriptArray(kindArray);
    auto* xs = AsScriptArray(xArray);
    auto* ys = AsScriptArray(yArray);
    auto* zs = AsScriptArray(zArray);
    auto* txs = AsScriptArray(txArray);
    auto* tys = AsScriptArray(tyArray);
    auto* tzs = AsScriptArray(tzArray);
    auto* spds = AsScriptArray(spdArray);
    auto* distLims = AsScriptArray(distLimArray);
    auto* yMinLims = AsScriptArray(yMinLimArray);
    auto* yMaxLims = AsScriptArray(yMaxLimArray);
    auto* intensities = AsScriptArray(intenArray);
    auto* idirs = AsScriptArray(idirArray);
    auto* heats = AsScriptArray(heatArray);
    auto* imins = AsScriptArray(iminArray);
    auto* imaxs = AsScriptArray(imaxArray);
    auto* dirXs = AsScriptArray(dirXArray);
    auto* dirYs = AsScriptArray(dirYArray);
    auto* dirZs = AsScriptArray(dirZArray);
    auto* dirLerps = AsScriptArray(dirLerpArray);

    if (!lights || !xs || !ys || !zs || !txs || !tys || !tzs || !spds || !distLims || !yMinLims || !yMaxLims || !intensities || !idirs || !heats || !imins || !imaxs) {
        return;
    }

    const int maxLightIndex = static_cast<int>(lights->Values.size()) - 1;
    const int minIndex = std::max(beginIndex, 0);
    const int maxIndex = std::min(endIndex, maxLightIndex);
    if (minIndex > maxIndex) return;

    bool anyUpdated = false;

    for (int i = minIndex; i <= maxIndex; ++i) {
        auto* entity = ReadEntityAt(lights, i);
        if (!entity) continue;

        auto* lightCmp = entity->GetComponent<LightComponent>();
        if (!lightCmp || !lightCmp->IsActive()) continue;

        float x = 0.0f, y = 0.0f, z = 0.0f;
        float tx = 0.0f, ty = 0.0f, tz = 0.0f;
        float spd = 0.0f, distLim = 0.0f;
        float yMinLim = 0.0f, yMaxLim = 0.0f;
        float intensity = 0.0f, idir = 0.0f, heat = 0.0f;
        float imin = 0.0f, imax = 0.0f;

        if (!TryReadFloatAt(xs, i, x) || !TryReadFloatAt(ys, i, y) || !TryReadFloatAt(zs, i, z) ||
            !TryReadFloatAt(txs, i, tx) || !TryReadFloatAt(tys, i, ty) || !TryReadFloatAt(tzs, i, tz) ||
            !TryReadFloatAt(spds, i, spd) || !TryReadFloatAt(distLims, i, distLim) ||
            !TryReadFloatAt(yMinLims, i, yMinLim) || !TryReadFloatAt(yMaxLims, i, yMaxLim) ||
            !TryReadFloatAt(intensities, i, intensity) || !TryReadFloatAt(idirs, i, idir) || !TryReadFloatAt(heats, i, heat) ||
            !TryReadFloatAt(imins, i, imin) || !TryReadFloatAt(imaxs, i, imax)) {
            continue;
        }

        const float dx = tx - x;
        const float dy = ty - y;
        const float dz = tz - z;

        x = x + dx * dt * spd;
        y = y + dy * dt * spd;
        z = z + dz * dt * spd;

        const float dist2 = dx * dx + dy * dy + dz * dz;
        if (dist2 < distLim) {
            tx = RandRangeNative(-800.0f, 800.0f);
            ty = RandRangeNative(yMinLim, yMaxLim);
            tz = RandRangeNative(-800.0f, 800.0f);
            spd = RandRangeNative(0.09f, 0.34f);
        }

        intensity = intensity + idir * dt * heat;
        if (intensity < imin) { intensity = imin; idir = 1.0f; }
        if (intensity > imax) { intensity = imax; idir = -1.0f; }

        int kind = 0;
        TryReadIntAt(kinds, i, kind);

        auto data = lightCmp->GetLightComponentData();
        data.Position = { x, y, z };
        data.Intensity = intensity;

        if (kind == 1 && dirXs && dirYs && dirZs && dirLerps) {
            float dirX = 0.0f, dirY = 0.0f, dirZ = 0.0f, dirLerp = 0.0f;
            if (TryReadFloatAt(dirXs, i, dirX) && TryReadFloatAt(dirYs, i, dirY) && TryReadFloatAt(dirZs, i, dirZ) && TryReadFloatAt(dirLerps, i, dirLerp)) {
                const float ndx = dx * 0.010f;
                const float ndy = (-0.78f) + dy * 0.002f;
                const float ndz = dz * 0.010f;

                dirX = dirX + (ndx - dirX) * dt * dirLerp;
                dirY = dirY + (ndy - dirY) * dt * dirLerp;
                dirZ = dirZ + (ndz - dirZ) * dt * dirLerp;

                data.Direction = { dirX, dirY, dirZ };

                WriteFloatAt(dirXs, i, dirX);
                WriteFloatAt(dirYs, i, dirY);
                WriteFloatAt(dirZs, i, dirZ);
            }
        }

        lightCmp->SetLightComponentDataSilent(data);
        anyUpdated = true;

        WriteFloatAt(xs, i, x);
        WriteFloatAt(ys, i, y);
        WriteFloatAt(zs, i, z);
        WriteFloatAt(txs, i, tx);
        WriteFloatAt(tys, i, ty);
        WriteFloatAt(tzs, i, tz);
        WriteFloatAt(spds, i, spd);
        WriteFloatAt(intensities, i, intensity);
        WriteFloatAt(idirs, i, idir);
        WriteFloatAt(imins, i, imin);
        WriteFloatAt(imaxs, i, imax);
    }

    if (anyUpdated) {
        AddNotifyToPull(NRenderSystemNotifyType::LightUpdateData);
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
	if (dt <= 0.0f) {
		return;
	}

	m_fScriptUpdateAccumulator = std::min(m_fScriptUpdateAccumulator + dt, m_fScriptUpdateMaxAccumulator);

	if (m_fScriptUpdateAccumulator < m_fScriptUpdateInterval) {
		return;
	}

	m_fScriptUpdateAccumulator -= m_fScriptUpdateInterval;
	ScriptManager::GetInstance()->Update(m_fScriptUpdateInterval);
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

	std::erase_if(m_vEntities, [this, holder](const std::shared_ptr<ComponentHolder>& ent) {
		if (ent.get() == holder) {

			RemoveEntityNameFromCache(ent.get());
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
	RebuildEntityNameCache();
}

void World::SetupScriptingBindings() {
	auto* scriptManager = ScriptManager::GetInstance();
	scriptManager->StopAllScripts();
	scriptManager->ClearRegisteredObjects();
	scriptManager->RegisterObject("world", this, "World");
	m_fScriptUpdateAccumulator = 0.0f;
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

void World::AddEntityNameToCache(ComponentHolder* entity) {
	if (!entity) return;

	auto name = entity->GetName();

	if (name.empty()) return;

	m_mEntityByName[name] = entity;
}

void World::RemoveEntityNameFromCache(ComponentHolder* entity) {
	if (!entity) return;

	std::erase_if(m_mEntityByName, [entity](const auto& kv) {
		return kv.second == entity;
	});
}

void World::RebuildEntityNameCache() {
	m_mEntityByName.clear();

	for (auto& entity : m_vEntities) {
		AddEntityNameToCache(entity.get());
	}
}

