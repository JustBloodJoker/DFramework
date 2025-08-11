#pragma once

#include <pch.h>
#include <D3DFramework/Utilites/Serializer/ReflectionImpl.h>

enum LightTypes {
    PointLight = 0,
    SpotLight = 1,
    DirectionalLight = 2,
    RectLight = 3,


    Size = 4
};


struct LightStruct {
    int LightType = LightTypes::PointLight;
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

    BEGIN_FIELD_REGISTRATION(LightStruct)
        REGISTER_FIELD(LightType)
        REGISTER_FIELD(Color)
        REGISTER_FIELD(Intensity)
        REGISTER_FIELD(Position)
        REGISTER_FIELD(AttenuationRadius)
        REGISTER_FIELD(SourceRadius)
        REGISTER_FIELD(Direction)
        REGISTER_FIELD(InnerConeAngle)
        REGISTER_FIELD(OuterConeAngle)
        REGISTER_FIELD(RectSize)
        REGISTER_FIELD(Rotation)
    END_FIELD_REGISTRATION()
};

struct LightBuffer {
    int LightCount;
    dx::XMFLOAT3 CameraPos;
    int IsShadowImpl;
    dx::XMFLOAT3 margin;
};

