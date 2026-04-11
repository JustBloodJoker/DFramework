#include <World/World.h>
#include <MainRenderer/MainRenderer.h>
#include <WinWindow/Utils/Scripting/ScriptManager.h>
#include <WinWindow/Utils/Scripting/ScriptValue.h>
#include <Utilities/HelperFunctions.h>
#include <Component/Light/LightComponent.h>
#include <Component/RenderObject/MeshComponent.h>
#include <System/RenderMeshesSystem.h>

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

void World::ActivateCamera(ComponentHolder* entity) {
	auto target = dynamic_cast<TBaseCamera*>(entity);
	if (!target) return;

	for (auto& worldEntity : m_vEntities) {
		auto camera = dynamic_cast<TBaseCamera*>(worldEntity.get());
		if (!camera) continue;

		camera->Activate(camera==target);
	}
}

int World::FillEntityBounds(ComponentHolder* entity, ScriptBoundsInfo* outBounds) {
	if (!outBounds) return 0;

	auto meshes = entity ? entity->GetComponents<MeshComponent>() : GetAllComponentsOfType<MeshComponent>();

	ScriptBoundsInfo computed;
	if (!BuildBoundsFromMeshes(meshes, computed)) return 0;

	*outBounds = computed;
	return 1;
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
		if (!std::isfinite(r) || !std::isfinite(g) || !std::isfinite(b)) return;
		light->SetLightColor({
			std::clamp(r, 0.0f, 100.0f),
			std::clamp(g, 0.0f, 100.0f),
			std::clamp(b, 0.0f, 100.0f)
		});
	}
}

void World::SetLightIntensity(ComponentHolder* entity, float intensity) {
	if (auto* light = dynamic_cast<TLight*>(entity)) {
		if ( !std::isfinite(intensity) ) return;

		light->SetLightIntensity(std::clamp(intensity, 0.0f, 200000.0f));
	}
}

void World::SetLightPosition(ComponentHolder* entity, float x, float y, float z) {
	if ( !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) ) return;

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
	if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return;

	dx::XMFLOAT3 direction = { x, y, z };
	auto lenSq = direction.x * direction.x + direction.y * direction.y + direction.z * direction.z;
	if (lenSq <= 1e-6f) {
		direction = { 0.0f, -1.0f, 0.0f };
	}
	else {
		auto invLen = 1.0f / std::sqrt(lenSq);
		direction.x *= invLen;
		direction.y *= invLen;
		direction.z *= invLen;
	}

	if (auto* directionalLight = dynamic_cast<TDirectionalLight*>(entity)) {
		directionalLight->SetLightDirection(direction);
		return;
	}
	if (auto* spotLight = dynamic_cast<TSpotLight*>(entity)) {
		spotLight->SetLightDirection(direction);
	}
}

void World::SetLightAttenuationRadius(ComponentHolder* entity, float radius) {
	if (!std::isfinite(radius)) return;
	auto safeRadius = std::clamp(radius, 1.0f, 100000.0f);

	if (auto* pointLight = dynamic_cast<TPointLight*>(entity)) {
		pointLight->SetLightAttenuationRadius(safeRadius);
		return;
	}
	if (auto* spotLight = dynamic_cast<TSpotLight*>(entity)) {
		spotLight->SetLightAttenuationRadius(safeRadius);
	}
}

void World::SetLightSourceRadius(ComponentHolder* entity, float radius) {
	if (!std::isfinite(radius)) return;
	auto safeRadius = std::clamp(radius, 0.0f, 50000.0f);

	if (auto* pointLight = dynamic_cast<TPointLight*>(entity)) {
		auto attenuation = std::max(1.0f, pointLight->GetLightAttenuationRadius());
		pointLight->SetLightSourceRadius(std::min(safeRadius, attenuation * 0.95f));
	}
}

void World::SetLightConeAngles(ComponentHolder* entity, float inner, float outer) {
	if (auto* spotLight = dynamic_cast<TSpotLight*>(entity)) {
		if (!std::isfinite(inner) || !std::isfinite(outer)) return;

		auto safeInner = std::clamp(inner, 0.01f, dx::XM_PI - 0.02f);
		auto safeOuter = std::clamp(outer, safeInner + 0.01f, dx::XM_PI - 0.001f);

		spotLight->SetLightInnerConeAngle(safeInner);
		spotLight->SetLightOuterConeAngle(safeOuter);
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

struct ScriptLightNativeState {
    bool Valid = false;
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 0.0f;
    float TX = 0.0f;
    float TY = 0.0f;
    float TZ = 0.0f;
    float Speed = 0.0f;
    float Intensity = 0.0f;
    float IntensityDirection = 0.0f;
    float IntensityMin = 0.0f;
    float IntensityMax = 0.0f;
    float DirectionX = 0.0f;
    float DirectionY = 0.0f;
    float DirectionZ = 0.0f;
    int Kind = 0;
};

ScriptLightNativeState ComputeScriptLightNativeState(
    const ScriptArray* kinds,
    const ScriptArray* xs,
    const ScriptArray* ys,
    const ScriptArray* zs,
    const ScriptArray* txs,
    const ScriptArray* tys,
    const ScriptArray* tzs,
    const ScriptArray* spds,
    const ScriptArray* distLims,
    const ScriptArray* yMinLims,
    const ScriptArray* yMaxLims,
    const ScriptArray* intensities,
    const ScriptArray* idirs,
    const ScriptArray* heats,
    const ScriptArray* imins,
    const ScriptArray* imaxs,
    const ScriptArray* dirXs,
    const ScriptArray* dirYs,
    const ScriptArray* dirZs,
    const ScriptArray* dirLerps,
    float dt,
    int index
) {
    ScriptLightNativeState state{};

    float x = 0.0f, y = 0.0f, z = 0.0f;
    float tx = 0.0f, ty = 0.0f, tz = 0.0f;
    float spd = 0.0f, distLim = 0.0f;
    float yMinLim = 0.0f, yMaxLim = 0.0f;
    float intensity = 0.0f, idir = 0.0f, heat = 0.0f;
    float imin = 0.0f, imax = 0.0f;

    if (!TryReadFloatAt(xs, index, x) || !TryReadFloatAt(ys, index, y) || !TryReadFloatAt(zs, index, z) ||
        !TryReadFloatAt(txs, index, tx) || !TryReadFloatAt(tys, index, ty) || !TryReadFloatAt(tzs, index, tz) ||
        !TryReadFloatAt(spds, index, spd) || !TryReadFloatAt(distLims, index, distLim) ||
        !TryReadFloatAt(yMinLims, index, yMinLim) || !TryReadFloatAt(yMaxLims, index, yMaxLim) ||
        !TryReadFloatAt(intensities, index, intensity) || !TryReadFloatAt(idirs, index, idir) || !TryReadFloatAt(heats, index, heat) ||
        !TryReadFloatAt(imins, index, imin) || !TryReadFloatAt(imaxs, index, imax)) {
        return state;
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
    TryReadIntAt(kinds, index, kind);

    float dirX = 0.0f;
    float dirY = -1.0f;
    float dirZ = 0.0f;
    if (kind == 1 && dirXs && dirYs && dirZs && dirLerps) {
        float dirLerp = 0.0f;
        if (TryReadFloatAt(dirXs, index, dirX) && TryReadFloatAt(dirYs, index, dirY) && TryReadFloatAt(dirZs, index, dirZ) && TryReadFloatAt(dirLerps, index, dirLerp)) {
            const float ndx = dx * 0.010f;
            const float ndy = (-0.78f) + dy * 0.002f;
            const float ndz = dz * 0.010f;

            dirX = dirX + (ndx - dirX) * dt * dirLerp;
            dirY = dirY + (ndy - dirY) * dt * dirLerp;
            dirZ = dirZ + (ndz - dirZ) * dt * dirLerp;
        }
    }

    state.Valid = true;
    state.X = x;
    state.Y = y;
    state.Z = z;
    state.TX = tx;
    state.TY = ty;
    state.TZ = tz;
    state.Speed = spd;
    state.Intensity = intensity;
    state.IntensityDirection = idir;
    state.IntensityMin = imin;
    state.IntensityMax = imax;
    state.DirectionX = dirX;
    state.DirectionY = dirY;
    state.DirectionZ = dirZ;
    state.Kind = kind;

    return state;
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

    const int lightCount = maxIndex - minIndex + 1;
    std::vector<ScriptLightNativeState> states(static_cast<size_t>(lightCount));

    auto computeRange = [&](int beginI, int endI) {
        for (int i = beginI; i <= endI; ++i) {
            states[static_cast<size_t>(i - minIndex)] = ComputeScriptLightNativeState(
                kinds,
                xs,
                ys,
                zs,
                txs,
                tys,
                tzs,
                spds,
                distLims,
                yMinLims,
                yMaxLims,
                intensities,
                idirs,
                heats,
                imins,
                imaxs,
                dirXs,
                dirYs,
                dirZs,
                dirLerps,
                dt,
                i
            );
        }
    };

    bool useParallel = m_bScriptNativeParallelUpdateEnabled && lightCount >= std::max(1, m_iScriptNativeParallelChunkSize);
    if (useParallel) {
        if (!m_bIsScriptNativeWorkerPoolInitialized) {
            m_xScriptNativeWorkerPool.Init(std::max(1, m_iScriptNativeParallelWorkerCount));
            m_bIsScriptNativeWorkerPoolInitialized = true;
        }
        else {
            m_xScriptNativeWorkerPool.Init(std::max(1, m_iScriptNativeParallelWorkerCount));
        }

        const int chunkSize = std::max(1, m_iScriptNativeParallelChunkSize);
        for (int chunkBegin = minIndex; chunkBegin <= maxIndex; chunkBegin += chunkSize) {
            const int chunkEnd = std::min(chunkBegin + chunkSize - 1, maxIndex);
            m_xScriptNativeWorkerPool.PostTask([&, chunkBegin, chunkEnd]() {
                computeRange(chunkBegin, chunkEnd);
            });
        }

        m_xScriptNativeWorkerPool.WaitIdle();
    }
    else {
        computeRange(minIndex, maxIndex);
    }

    bool anyUpdated = false;
    for (int i = minIndex; i <= maxIndex; ++i) {
        const auto& state = states[static_cast<size_t>(i - minIndex)];
        if (!state.Valid) continue;

        auto* entity = ReadEntityAt(lights, i);
        if (!entity) continue;

        auto* lightCmp = entity->GetComponent<LightComponent>();
        if (!lightCmp || !lightCmp->IsActive()) continue;

        auto data = lightCmp->GetLightComponentData();
        data.Position = { state.X, state.Y, state.Z };
        data.Intensity = state.Intensity;

        if (state.Kind == 1) {
            dx::XMFLOAT3 direction = { state.DirectionX, state.DirectionY, state.DirectionZ };
            auto lenSq = direction.x * direction.x + direction.y * direction.y + direction.z * direction.z;
            if (lenSq <= 1e-6f) {
                direction = { 0.0f, -1.0f, 0.0f };
            }
            else {
                auto invLen = 1.0f / std::sqrt(lenSq);
                direction.x *= invLen;
                direction.y *= invLen;
                direction.z *= invLen;
            }
            data.Direction = direction;
        }

        lightCmp->SetLightComponentDataSilent(data);
        anyUpdated = true;

        WriteFloatAt(xs, i, state.X);
        WriteFloatAt(ys, i, state.Y);
        WriteFloatAt(zs, i, state.Z);
        WriteFloatAt(txs, i, state.TX);
        WriteFloatAt(tys, i, state.TY);
        WriteFloatAt(tzs, i, state.TZ);
        WriteFloatAt(spds, i, state.Speed);
        WriteFloatAt(intensities, i, state.Intensity);
        WriteFloatAt(idirs, i, state.IntensityDirection);
        WriteFloatAt(imins, i, state.IntensityMin);
        WriteFloatAt(imaxs, i, state.IntensityMax);
        if (dirXs && dirYs && dirZs) {
            WriteFloatAt(dirXs, i, state.DirectionX);
            WriteFloatAt(dirYs, i, state.DirectionY);
            WriteFloatAt(dirZs, i, state.DirectionZ);
        }
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

void World::SetDefaultCameraEyeAndLookAt(ComponentHolder* entity, float eyeX, float eyeY, float eyeZ, float lookX, float lookY, float lookZ) {
	if (auto* camera = dynamic_cast<TDefaultCamera*>(entity)) {
		camera->SetEyeAndLookAt(eyeX, eyeY, eyeZ, lookX, lookY, lookZ);
	}
}

void World::SetDefaultCameraYawPitchRoll(ComponentHolder* entity, float yaw, float pitch, float roll) {
	if (auto* camera = dynamic_cast<TDefaultCamera*>(entity)) {
		camera->SetYawPitchRoll(yaw, pitch, roll);
	}
}

void World::SetDefaultCameraInputEnabled(ComponentHolder* entity, int enabled) {
	if (auto* camera = dynamic_cast<TDefaultCamera*>(entity)) {
		camera->SetInputEnabled(enabled);
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

void World::SetIBLEnabled(int enabled) {
	if (!m_pRender) return;

	m_pRender->EnableIBL(enabled != 0);
}

void World::SetIBLDiffuseIntensity(float intensity) {
	if (!m_pRender) return;

	m_pRender->SetIBLDiffuseIntensity(intensity);
}

void World::SetIBLSpecularIntensity(float intensity) {
	if (!m_pRender) return;

	m_pRender->SetIBLSpecularIntensity(intensity);
}

void World::SetIBLMaxReflectionMip(float value) {
	if (!m_pRender) return;

	m_pRender->SetIBLMaxReflectionMip(value);
}

void World::SetTAAEnabled(int enabled) {
	if (!m_pRender) return;

	m_pRender->EnableTAA(enabled != 0);
}

void World::SetTAABlendWeight(float value) {
	if (!m_pRender) return;

	m_pRender->SetTAABlendWeight(value);
}

void World::SetJitterEnabled(int enabled) {
	if (!m_pRender) return;

	m_pRender->EnableJitter(enabled != 0);
}

void World::SetLinkJitterToTAA(int enabled) {
	if (!m_pRender) return;

	m_pRender->EnableLinkJitterToTAA(enabled != 0);
}

void World::SetUnlitScene(int enabled) {
	if (!m_pRender) return;

	m_pRender->EnableUnlitScene(enabled != 0);
}

void World::SetMeshCullingType(int type) {
	if (!m_pRender) return;

	auto clamped = std::clamp(type, int(MeshCullingType::None), int(MeshCullingType::GPU));
	m_pRender->SetMeshCullingType(MeshCullingType(clamped));
}

void World::SetScriptUpdateInterval(float seconds) {
	if (!std::isfinite(seconds)) return;

	m_fScriptUpdateInterval = std::clamp(seconds, 1.0f / 240.0f, 1.0f);
	m_fScriptUpdateMaxAccumulator = std::max(m_fScriptUpdateMaxAccumulator, m_fScriptUpdateInterval);
}

void World::SetScriptUpdateMaxAccumulator(float seconds) {
	if (!std::isfinite(seconds)) return;

	m_fScriptUpdateMaxAccumulator = std::clamp(seconds, m_fScriptUpdateInterval, 1.0f);
	m_fScriptUpdateAccumulator = std::min(m_fScriptUpdateAccumulator, m_fScriptUpdateMaxAccumulator);
}

void World::SetScriptNativeParallelUpdateEnabled(int enabled) {
	m_bScriptNativeParallelUpdateEnabled = enabled != 0;
}

void World::SetScriptNativeParallelWorkerCount(int workerCount) {
	m_iScriptNativeParallelWorkerCount = std::clamp(workerCount, 1, 32);
	if (m_bIsScriptNativeWorkerPoolInitialized) {
		m_xScriptNativeWorkerPool.Init(m_iScriptNativeParallelWorkerCount);
	}
}

void World::SetScriptNativeParallelChunkSize(int chunkSize) {
	m_iScriptNativeParallelChunkSize = std::clamp(chunkSize, 1, 4096);
}

void World::CinematicClearPath() {
	CinematicStop();
	m_vCinematicPoints.clear();
}

void World::CinematicAddPoint(float eyeX, float eyeY, float eyeZ, float lookX, float lookY, float lookZ) {
	if (!std::isfinite(eyeX) || !std::isfinite(eyeY) || !std::isfinite(eyeZ) || !std::isfinite(lookX) || !std::isfinite(lookY) || !std::isfinite(lookZ)) {
		return;
	}

	m_vCinematicPoints.push_back({
		{ eyeX, eyeY, eyeZ },
		{ lookX, lookY, lookZ }
	});
}

void World::CinematicSetDuration(float durationSeconds) {
	if (!std::isfinite(durationSeconds)) return;

	m_fCinematicDuration = std::clamp(durationSeconds, 0.1f, 600.0f);
}

void World::CinematicSetLoop(int loop) {
	m_bCinematicLoop = loop != 0;
}

void World::CinematicSetUseSmoothStep(int enabled) {
	m_bCinematicUseSmoothStep = enabled != 0;
}

int World::CinematicPlay(ComponentHolder* cameraEntity, int lockInput) {
	auto* camera = dynamic_cast<TDefaultCamera*>(cameraEntity);
	if (!camera) return 0;
	if (m_vCinematicPoints.size() < 2) return 0;

	CinematicStop();
	ActivateCamera(camera);

	m_pCinematicCamera = camera;
	m_bCinematicLockInput = lockInput != 0;
	m_bCinematicPlaying = true;
	m_fCinematicTime = 0.0f;
	m_iCinematicPrevInputEnabled = camera->IsInputEnabled();

	SetCinematicInputLock(m_bCinematicLockInput);

	const auto startEye = EvaluateCinematicSpline(m_vCinematicPoints, 0.0f, false);
	const auto startLook = EvaluateCinematicSpline(m_vCinematicPoints, 0.0f, true);
	m_pCinematicCamera->SetEyeAndLookAt(startEye.x, startEye.y, startEye.z, startLook.x, startLook.y, startLook.z);

	return 1;
}

void World::CinematicStop() {
	if (!m_bCinematicPlaying && !m_pCinematicCamera) {
		return;
	}

	SetCinematicInputLock(false);

	m_bCinematicPlaying = false;
	m_fCinematicTime = 0.0f;
	m_pCinematicCamera = nullptr;
}

int World::CinematicIsPlaying() {
	return m_bCinematicPlaying ? 1 : 0;
}

float World::CinematicGetProgress() {
	if (!m_bCinematicPlaying || m_fCinematicDuration <= 0.0f) return 0.0f;

	return std::clamp(m_fCinematicTime / m_fCinematicDuration, 0.0f, 1.0f);
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
	UpdateCinematic(dt);

	if (dt <= 0.0f) {
		return;
	}

	m_fScriptUpdateAccumulator = std::min(m_fScriptUpdateAccumulator + dt, m_fScriptUpdateMaxAccumulator);

	if (m_fScriptUpdateAccumulator < m_fScriptUpdateInterval) {
		return;
	}

	int steps = 0;
	constexpr int kMaxScriptStepsPerFrame = 8;
	while (m_fScriptUpdateAccumulator >= m_fScriptUpdateInterval && steps < kMaxScriptStepsPerFrame) {
		m_fScriptUpdateAccumulator -= m_fScriptUpdateInterval;
		ScriptManager::GetInstance()->Update(m_fScriptUpdateInterval);
		++steps;
	}

	if (steps >= kMaxScriptStepsPerFrame && m_fScriptUpdateAccumulator > m_fScriptUpdateInterval) {
		m_fScriptUpdateAccumulator = std::fmod(m_fScriptUpdateAccumulator, m_fScriptUpdateInterval);
	}
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
	CinematicStop();
	m_vCinematicPoints.clear();
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

bool World::BuildBoundsFromMeshes(const std::vector<MeshComponent*>& meshes, ScriptBoundsInfo& outBounds) const {
	if (meshes.empty()) return false;

	dx::XMFLOAT3 minP = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
	dx::XMFLOAT3 maxP = { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };
	bool hasBounds = false;

	for (auto* mesh : meshes) {
		if (!mesh || !mesh->IsActive()) continue;

		auto [localMin, localMax] = mesh->GetWorldBounds();
		if (!std::isfinite(localMin.x) || !std::isfinite(localMin.y) || !std::isfinite(localMin.z) ||
			!std::isfinite(localMax.x) || !std::isfinite(localMax.y) || !std::isfinite(localMax.z)) {
			continue;
		}

		minP.x = std::min(minP.x, localMin.x);
		minP.y = std::min(minP.y, localMin.y);
		minP.z = std::min(minP.z, localMin.z);

		maxP.x = std::max(maxP.x, localMax.x);
		maxP.y = std::max(maxP.y, localMax.y);
		maxP.z = std::max(maxP.z, localMax.z);
		hasBounds = true;
	}

	if (!hasBounds) return false;

	outBounds.MinX = minP.x;
	outBounds.MinY = minP.y;
	outBounds.MinZ = minP.z;
	outBounds.MaxX = maxP.x;
	outBounds.MaxY = maxP.y;
	outBounds.MaxZ = maxP.z;

	outBounds.SizeX = std::max(0.0f, maxP.x - minP.x);
	outBounds.SizeY = std::max(0.0f, maxP.y - minP.y);
	outBounds.SizeZ = std::max(0.0f, maxP.z - minP.z);

	outBounds.CenterX = (minP.x + maxP.x) * 0.5f;
	outBounds.CenterY = (minP.y + maxP.y) * 0.5f;
	outBounds.CenterZ = (minP.z + maxP.z) * 0.5f;
	return true;
}

void World::SetCinematicInputLock(bool lockInput) {
	if (!m_pCinematicCamera) return;

	if (lockInput && m_bCinematicLockInput) {
		m_pCinematicCamera->SetInputEnabled(0);
	}
	else {
		m_pCinematicCamera->SetInputEnabled(m_iCinematicPrevInputEnabled);
	}
}

float World::ApplyCinematicSmoothStep(float t) {
	auto clamped = std::clamp(t, 0.0f, 1.0f);
	return clamped * clamped * (3.0f - 2.0f * clamped);
}

dx::XMFLOAT3 World::CatmullRom(const dx::XMFLOAT3& p0, const dx::XMFLOAT3& p1, const dx::XMFLOAT3& p2, const dx::XMFLOAT3& p3, float t) {
	auto t2 = t * t;
	auto t3 = t2 * t;

	dx::XMFLOAT3 out{};
	out.x = 0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 + (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);
	out.y = 0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 + (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);
	out.z = 0.5f * ((2.0f * p1.z) + (-p0.z + p2.z) * t + (2.0f * p0.z - 5.0f * p1.z + 4.0f * p2.z - p3.z) * t2 + (-p0.z + 3.0f * p1.z - 3.0f * p2.z + p3.z) * t3);
	return out;
}

dx::XMFLOAT3 World::EvaluateCinematicSpline(const std::vector<CinematicPoint>& points, float t01, bool sampleLookAt) {
	if (points.empty()) return { 0.0f, 0.0f, 0.0f };
	if (points.size() == 1) return sampleLookAt ? points.front().LookAt : points.front().Eye;

	auto clampedT = std::clamp(t01, 0.0f, 1.0f);
	auto segmentFloat = clampedT * float(points.size() - 1);
	auto segmentIndex = std::min(int(segmentFloat), int(points.size()) - 2);
	auto localT = segmentFloat - float(segmentIndex);

	auto p0Idx = std::max(segmentIndex - 1, 0);
	auto p1Idx = segmentIndex;
	auto p2Idx = segmentIndex + 1;
	auto p3Idx = std::min(segmentIndex + 2, int(points.size()) - 1);

	const auto& p0 = sampleLookAt ? points[static_cast<size_t>(p0Idx)].LookAt : points[static_cast<size_t>(p0Idx)].Eye;
	const auto& p1 = sampleLookAt ? points[static_cast<size_t>(p1Idx)].LookAt : points[static_cast<size_t>(p1Idx)].Eye;
	const auto& p2 = sampleLookAt ? points[static_cast<size_t>(p2Idx)].LookAt : points[static_cast<size_t>(p2Idx)].Eye;
	const auto& p3 = sampleLookAt ? points[static_cast<size_t>(p3Idx)].LookAt : points[static_cast<size_t>(p3Idx)].Eye;

	return CatmullRom(p0, p1, p2, p3, localT);
}

void World::UpdateCinematic(float dt) {
	if (!m_bCinematicPlaying) return;
	if (!m_pCinematicCamera) {
		m_bCinematicPlaying = false;
		return;
	}
	if (m_vCinematicPoints.size() < 2) {
		CinematicStop();
		return;
	}
	if (dt <= 0.0f) return;

	m_fCinematicTime += dt;
	if (m_fCinematicDuration <= 0.0f) m_fCinematicDuration = 0.1f;

	auto t01 = m_fCinematicTime / m_fCinematicDuration;
	if (m_bCinematicLoop) {
		t01 = std::fmod(t01, 1.0f);
		if (t01 < 0.0f) t01 += 1.0f;
	}
	else if (t01 >= 1.0f) {
		t01 = 1.0f;
	}

	if (m_bCinematicUseSmoothStep) {
		t01 = ApplyCinematicSmoothStep(t01);
	}

	auto eye = EvaluateCinematicSpline(m_vCinematicPoints, t01, false);
	auto look = EvaluateCinematicSpline(m_vCinematicPoints, t01, true);
	m_pCinematicCamera->SetEyeAndLookAt(eye.x, eye.y, eye.z, look.x, look.y, look.z);

	if (!m_bCinematicLoop && m_fCinematicTime >= m_fCinematicDuration) {
		CinematicStop();
	}
}

