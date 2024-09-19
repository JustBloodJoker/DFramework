#pragma once
#include "../pch.h"

void InverseTexture(ID3D12Resource* texture, ID3D12Device* device);
void GreyEffect(ID3D12Resource* texture, ID3D12Device* device);
void SharpnessEffect(ID3D12Resource* texture, ID3D12Device* device);
void BlurEffect(ID3D12Resource* texture, ID3D12Device* device);

/////////////////////////
// SURFACES MAP CONTROL

void ClearFromMap(ID3D12Resource* texture);
void ClearAllFromMap();
void InitToMap(ID3D12Resource* texture, ID3D12Device* device);
bool FindInMap(ID3D12Resource* texture);