#pragma once

#include <pch.h>

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

    //ûpot light only
    dx::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };
    float InnerConeAngle = dx::XMConvertToRadians(15.0f);
    float OuterConeAngle = dx::XMConvertToRadians(30.0f);


    dx::XMFLOAT2 RectSize = { 1.0f, 1.0f };
    dx::XMFLOAT3 Up = { 0.0f, 1.0f, 0.0f };
    dx::XMFLOAT3 Right = { 1.0f, 0.0f, 0.0f };
};

struct LightBuffer {
    int LightCount;
    dx::XMFLOAT3 CameraPos;
};
