#pragma once
#include <pch.h>


struct IndirectMeshRenderableData {
    D3D12_GPU_VIRTUAL_ADDRESS CBMatrices;
    D3D12_GPU_VIRTUAL_ADDRESS CBMaterials;
    D3D12_GPU_VIRTUAL_ADDRESS SRVBones;
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
    D3D12_INDEX_BUFFER_VIEW IndexBufferView;
    D3D12_DRAW_INDEXED_ARGUMENTS DrawArguments;
};