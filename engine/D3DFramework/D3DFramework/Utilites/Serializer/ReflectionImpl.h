#pragma once

#include "../../pch.h"
#include <WinWindow/Utils/Reflection/Reflection.h>
#include <WinWindow/Utils/Serializer/Serializer.h>

using DX_XMFLOAT2 = dx::XMFLOAT2;
REGISTER_SERIALIZER(DX_XMFLOAT2, [](IArchive& ar, void* ptr) {
    DX_XMFLOAT2& obj = *static_cast<DX_XMFLOAT2*>(ptr);
    SerializeAny(ar, obj.x);
    SerializeAny(ar, obj.y);
});

using DX_XMFLOAT3 = dx::XMFLOAT3;
REGISTER_SERIALIZER(DX_XMFLOAT3, [](IArchive& ar, void* ptr) {
    DX_XMFLOAT3& obj = *static_cast<DX_XMFLOAT3*>(ptr);
    SerializeAny(ar, obj.x);
    SerializeAny(ar, obj.y);
    SerializeAny(ar, obj.z);
});

using DX_XMFLOAT4 = dx::XMFLOAT4;
REGISTER_SERIALIZER(DX_XMFLOAT4, [](IArchive& ar, void* ptr) {
    DX_XMFLOAT4& obj = *static_cast<DX_XMFLOAT4*>(ptr);
    SerializeAny(ar, obj.x);
    SerializeAny(ar, obj.y);
    SerializeAny(ar, obj.z);
    SerializeAny(ar, obj.w);
});

using DX_XMFLOAT4X4 = dx::XMFLOAT4X4;
REGISTER_SERIALIZER(DX_XMFLOAT4X4, [](IArchive& ar, void* ptr) {
    DX_XMFLOAT4X4& obj = *static_cast<DX_XMFLOAT4X4*>(ptr);
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            SerializeAny(ar, obj.m[i][j]);
});

using DX_XMMATRIX = dx::XMMATRIX;
REGISTER_SERIALIZER(DX_XMMATRIX, [](IArchive& ar, void* ptr) {
    DX_XMMATRIX& obj = *static_cast<DX_XMMATRIX*>(ptr);
    if (ar.IsOutput()) {
        dx::XMFLOAT4X4 mOut;
        dx::XMStoreFloat4x4(&mOut, obj);
        SerializeAny(ar, mOut);
    }
    else {
        dx::XMFLOAT4X4 mIn;
        SerializeAny(ar, mIn);
        obj = dx::XMLoadFloat4x4(&mIn);
    }
});

using DX_XMVECTOR = dx::XMVECTOR;
REGISTER_SERIALIZER(DX_XMVECTOR, [](IArchive& ar, void* ptr) {
    DX_XMVECTOR& obj = *static_cast<DX_XMVECTOR*>(ptr);
    if (ar.IsOutput()) {
        dx::XMFLOAT4 out;
        dx::XMStoreFloat4(&out, obj);
        SerializeAny(ar, out);
    }
    else {
        dx::XMFLOAT4 in;
        SerializeAny(ar, in);
        obj = dx::XMLoadFloat4(&in);
    }
});
