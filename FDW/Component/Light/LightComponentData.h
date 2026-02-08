#pragma once

#include <pch.h>
#include <WinWindow/Utils/Reflection/Reflection.h>

enum class NLightComponentTypes {
    PointLight = 0,
    SpotLight = 1,
    DirectionalLight = 2,
    RectLight = 3,


    Size = 4
};


struct LightComponentData {
    int LightType = int( NLightComponentTypes::PointLight );
    dx::XMFLOAT3 Color = { 1.0f, 0.95f, 0.9f };
    float Intensity = 50.0f;

    dx::XMFLOAT3 Position = { 0.0f, 5.0f, 0.0f };

    float AttenuationRadius = 10.0f;

    //point light only
    float SourceRadius = 0.0f;

    //spot light only
    dx::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f }; //directional light using this
    float InnerConeAngle = dx::XMConvertToRadians(15.0f);
    float OuterConeAngle = dx::XMConvertToRadians(30.0f);

    //LTC only
    dx::XMFLOAT2 RectSize = { 1.0f, 1.0f };
    dx::XMFLOAT3 Rotation = { 0.0f, dx::XMConvertToRadians(90.0f), 0.0f };

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
    END_REFLECT(LightComponentData)
};
