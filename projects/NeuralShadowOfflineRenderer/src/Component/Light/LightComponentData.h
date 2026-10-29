#pragma once

#include <WinWindow/Utils/Reflection/Reflection.h>
#include <DirectXMath.h>

namespace dx = DirectX;

enum class NLightComponentTypes {
    PointLight = 0,
    SpotLight = 1,
    DirectionalLight = 2,
    RectLight = 3,
    Size = 4
};

inline constexpr float LIGHT_MAX_SOURCE_RADIUS = 50000.0f;
inline constexpr float LIGHT_MAX_DIRECTIONAL_ANGULAR_RADIUS = 0.5f;
inline constexpr float LIGHT_MAX_SHADOW_SOFTNESS = 4.0f;

struct LightComponentData {
    int LightType = int(NLightComponentTypes::PointLight);
    dx::XMFLOAT3 Color = { 1.0f, 0.95f, 0.9f };
    float Intensity = 50.0f;

    dx::XMFLOAT3 Position = { 0.0f, 5.0f, 0.0f };
    float AttenuationRadius = 10.0f;

    // Point and spot lights: physical emitter-disk radius in world units.
    float SourceRadius = 0.0f;

    dx::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };
    float InnerConeAngle = dx::XMConvertToRadians(15.0f);
    float OuterConeAngle = dx::XMConvertToRadians(30.0f);

    dx::XMFLOAT2 RectSize = { 1.0f, 1.0f };
    dx::XMFLOAT3 Rotation = { 0.0f, dx::XMConvertToRadians(90.0f), 0.0f };

    // Directional lights: physical emitter half-angle in radians.
    float DirectionalAngularRadius = 0.0f;

    // Unitless artist multiplier for the physical emitter extent.
    float ShadowSoftness = 1.0f;

    REFLECT_STRUCT(LightComponentData)
    BEGIN_REFLECT(LightComponentData)
        REFLECT_PROPERTY(LightType)
        REFLECT_PROPERTY(Color)
        REFLECT_PROPERTY(Intensity)
        REFLECT_PROPERTY(Position)
        REFLECT_PROPERTY(AttenuationRadius)
        REFLECT_PROPERTY(SourceRadius)
        REFLECT_PROPERTY(Direction)
        REFLECT_PROPERTY(InnerConeAngle)
        REFLECT_PROPERTY(OuterConeAngle)
        REFLECT_PROPERTY(RectSize)
        REFLECT_PROPERTY(Rotation)
        REFLECT_PROPERTY(DirectionalAngularRadius)
        REFLECT_PROPERTY(ShadowSoftness)
    END_REFLECT(LightComponentData)
};
