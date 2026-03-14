#include "RTObjectHelper.h"

namespace FD3DW {


    std::vector<AccelerationStructureBuffers> CreateBottomLevelAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, const std::vector<AccelerationStructureInput>& geometries, UINT hitGroupIndex, bool isStandaloneGeometries)
    {
        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;
        for (const auto& geom : geometries) {
            D3D12_RAYTRACING_GEOMETRY_DESC desc = {};
            desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            desc.Flags = geom.flags;

            desc.Triangles.VertexBuffer.StartAddress = geom.vertexBuffer->GetGPUVirtualAddress() + geom.vertexOffset * geom.vertexStride;
            desc.Triangles.VertexBuffer.StrideInBytes = geom.vertexStride;
            desc.Triangles.VertexCount = geom.vertexCount;
            desc.Triangles.VertexFormat = geom.vertexFormat;

            desc.Triangles.IndexBuffer = geom.indexBuffer->GetGPUVirtualAddress() + geom.indexOffset * geom.indexStride;
            desc.Triangles.IndexCount = geom.indexCount;
            desc.Triangles.IndexFormat = geom.indexFormat;



            geometryDescs.push_back(desc);
        }

        std::vector<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS> inputs;

        if (isStandaloneGeometries)
        {
            for (const auto& geom : geometryDescs) {
                D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS input;
                input.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
                input.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
                input.NumDescs = 1;
                input.pGeometryDescs = &geom;
                input.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
                inputs.push_back(input);

            }

        }
        else
        {
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS input;
            input.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            input.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            input.NumDescs = static_cast<UINT>(geometryDescs.size());
            input.pGeometryDescs = geometryDescs.data();
            input.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
            inputs.push_back(input);
        }


        std::vector<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO> prebuildInfos;
        for (int i = 0; i < inputs.size(); ++i) {
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
            device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs[i], &info);
            prebuildInfos.push_back(info);
        }

        std::vector<AccelerationStructureBuffers> buffers;

        for (int i = 0; i < inputs.size(); ++i) {
            AccelerationStructureBuffers buffer;
            CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
            CD3DX12_RESOURCE_DESC descScratch = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfos[i].ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &descScratch, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&buffer.pScratch));

            CD3DX12_RESOURCE_DESC descResult = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfos[i].ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &descResult, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&buffer.pResult));

            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
            buildDesc.Inputs = inputs[i];
            buildDesc.DestAccelerationStructureData = buffer.pResult->GetGPUVirtualAddress();
            buildDesc.ScratchAccelerationStructureData = buffer.pScratch->GetGPUVirtualAddress();

            cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

            CD3DX12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(buffer.pResult.Get());
            cmdList->ResourceBarrier(1, &uavBarrier);

            buffer.HitGroupIndex = hitGroupIndex;

            buffers.push_back(buffer);
        }

        return buffers;
    }

    AccelerationStructureBuffers CreateTopLevelAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, const std::vector<std::pair<AccelerationStructureBuffers, dx::XMMATRIX>>& instances)
    {
        auto outInstanceCount = static_cast<UINT64>(instances.size());

        std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs(outInstanceCount);
        for (UINT i = 0; i < outInstanceCount; ++i) {
            const auto& [blas, transform] = instances[i];
            D3D12_RAYTRACING_INSTANCE_DESC& desc = instanceDescs[i];
            XMMATRIXSetToDXRMatrix(desc.Transform, transform);
            desc.InstanceID = i;
            desc.InstanceMask = 1;
            desc.AccelerationStructure = blas.pResult->GetGPUVirtualAddress();
            desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
            desc.InstanceContributionToHitGroupIndex = blas.HitGroupIndex;
        }

        AccelerationStructureBuffers buffers;
        UINT64 descSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * outInstanceCount;

        CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC instBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(descSize);
        device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &instBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffers.pInstanceDesc));

        void* mapped = nullptr;
        buffers.pInstanceDesc->Map(0, nullptr, &mapped);
        memcpy(mapped, instanceDescs.data(), descSize);
        buffers.pInstanceDesc->Unmap(0, nullptr);

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
        inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        inputs.NumDescs = static_cast<UINT>(outInstanceCount);
        inputs.InstanceDescs = buffers.pInstanceDesc->GetGPUVirtualAddress();
        inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
        device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

        CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        CD3DX12_RESOURCE_DESC resultDesc = CD3DX12_RESOURCE_DESC::Buffer(info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &scratchDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&buffers.pScratch));
        device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &resultDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&buffers.pResult));

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
        buildDesc.Inputs = inputs;
        buildDesc.DestAccelerationStructureData = buffers.pResult->GetGPUVirtualAddress();
        buildDesc.ScratchAccelerationStructureData = buffers.pScratch->GetGPUVirtualAddress();

        cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
        CD3DX12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(buffers.pResult.Get());
        cmdList->ResourceBarrier(1, &uavBarrier);

        return buffers;
    }

    AccelerationStructureBuffers CreateTopLevelAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, std::vector<AccelerationStructureBuffers> blasList, dx::XMMATRIX defaultMatrix)
    {
        std::vector<std::pair<AccelerationStructureBuffers, dx::XMMATRIX>> instances;
        instances.reserve(blasList.size());

        for (const auto& blas : blasList) {
            instances.emplace_back(blas, defaultMatrix);
        }

        return CreateTopLevelAS(device, cmdList, instances);
    }


    void UpdateTopLevelAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, AccelerationStructureBuffers& tlasBuffers, const std::vector<std::pair<AccelerationStructureBuffers, dx::XMMATRIX>>& instances)
    {
        UINT64 instanceCount = static_cast<UINT64>(instances.size());

        std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs(instanceCount);
        for (UINT i = 0; i < instanceCount; ++i) {
            const auto& [blas, transform] = instances[i];
            D3D12_RAYTRACING_INSTANCE_DESC& desc = instanceDescs[i];
            XMMATRIXSetToDXRMatrix(desc.Transform, transform);
            desc.InstanceID = i;
            desc.InstanceMask = 1;
            desc.AccelerationStructure = blas.pResult->GetGPUVirtualAddress();
            desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
            desc.InstanceContributionToHitGroupIndex = blas.HitGroupIndex;
        }

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
        inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        inputs.NumDescs = static_cast<UINT>(instanceCount);
        inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
        device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

        UINT64 requiredInstanceDescSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceCount;

        bool recreate = false;

        D3D12_RESOURCE_DESC desc;

        if (tlasBuffers.pInstanceDesc) {
            desc = tlasBuffers.pInstanceDesc->GetDesc();
            if (desc.Width < requiredInstanceDescSize) recreate = true;
        }
        else recreate = true;

        if (tlasBuffers.pScratch) {
            desc = tlasBuffers.pScratch->GetDesc();
            if (desc.Width < info.ScratchDataSizeInBytes) recreate = true;
        }
        else recreate = true;

        if (tlasBuffers.pResult) {
            desc = tlasBuffers.pResult->GetDesc();
            if (desc.Width < info.ResultDataMaxSizeInBytes) recreate = true;
        }
        else recreate = true;

        if (recreate) {
            CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
            CD3DX12_RESOURCE_DESC instBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(requiredInstanceDescSize);
            device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &instBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&tlasBuffers.pInstanceDesc));

            CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
            CD3DX12_RESOURCE_DESC scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            CD3DX12_RESOURCE_DESC resultDesc = CD3DX12_RESOURCE_DESC::Buffer(info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

            device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &scratchDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&tlasBuffers.pScratch));
            device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &resultDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&tlasBuffers.pResult));
        }

        void* mapped = nullptr;
        tlasBuffers.pInstanceDesc->Map(0, nullptr, &mapped);
        memcpy(mapped, instanceDescs.data(), requiredInstanceDescSize);
        tlasBuffers.pInstanceDesc->Unmap(0, nullptr);

        inputs.InstanceDescs = tlasBuffers.pInstanceDesc->GetGPUVirtualAddress();

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
        buildDesc.Inputs = inputs;
        buildDesc.DestAccelerationStructureData = tlasBuffers.pResult->GetGPUVirtualAddress();
        buildDesc.ScratchAccelerationStructureData = tlasBuffers.pScratch->GetGPUVirtualAddress();

        cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

        CD3DX12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(tlasBuffers.pResult.Get());
        cmdList->ResourceBarrier(1, &uavBarrier);
    }


    std::vector<AccelerationStructureInput> CreateGeometriesForObject(Object* obj, ID3D12Resource* vb, ID3D12Resource* ib, UINT strideSize)
    {
        std::vector<AccelerationStructureInput> ret;

        auto objVectexBuffer = vb;
        auto objIndexBuffer = ib;

        auto count = obj->GetObjectBuffersCount();
        ret.resize(count);

        for (auto i = 0; i < count; ++i) {
            ret[i] = CreateGeometryForObject(obj, i, vb, ib, strideSize);
        }

        return ret;
    }

    AccelerationStructureInput CreateGeometryForObject(Object* obj, UINT objDataIdx, ID3D12Resource* vb, ID3D12Resource* ib, UINT strideSize)
    {
        auto objVectexBuffer = vb;
        auto objIndexBuffer = ib;

        AccelerationStructureInput ret;
        ret.indexBuffer = objIndexBuffer;
        ret.vertexBuffer = objVectexBuffer;
        auto params = obj->GetObjectParameters(objDataIdx);
        ret.indexCount = params.IndicesCount;
        ret.indexFormat = DEFAULT_INDEX_BUFFER_FORMAT;
        ret.indexOffset = params.IndicesOffset;
        ret.indexStride = GetFormatSizeInBytes(DEFAULT_INDEX_BUFFER_FORMAT);
        ret.vertexFormat = DEFAULT_RT_VERTEX_BUFFER_FORMAT;
        ret.vertexCount = params.VerticesCount;
        ret.vertexOffset = params.VerticesOffset;
        ret.vertexStride = strideSize;
        return ret;
    }

    AccelerationStructureBuffers CreateBLASForObjectInIndex(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, Object* obj, UINT objectParamsIndex, ID3D12Resource* vb, ID3D12Resource* ib, UINT strideSize, UINT hitGroupIndex) {
        auto geometry = CreateGeometryForObject(obj, objectParamsIndex, vb, ib, strideSize);

        return CreateBottomLevelAS(device, cmdList, { geometry }, hitGroupIndex, false).front();
    }



    std::vector<AccelerationStructureBuffers> CreateBLASForObject(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, Object* obj, ID3D12Resource* vb, ID3D12Resource* ib, UINT strideSize, UINT hitGroupIndex, bool isStandaloneGeometries)
    {
        auto geometries = CreateGeometriesForObject(obj, vb, ib, strideSize);

        return CreateBottomLevelAS(device, cmdList, geometries, hitGroupIndex, isStandaloneGeometries);
    }

    RTObjectData CreateRTDataForObject(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ID3D12Resource* vb, ID3D12Resource* ib, UINT strideSize, UINT hitGroupIndex, Object* obj)
    {
        RTObjectData data;
        auto blas = CreateBLASForObject(device, cmdList, obj, vb, ib, strideSize, hitGroupIndex,true);
        data.BLASBuffers = blas;
        data.Transforms.resize(blas.size(), dx::XMMatrixIdentity());

        auto instances = data.GetInstances();

        auto tlas = CreateTopLevelAS(device, cmdList, instances);
        data.TLASBuffers = tlas;

        return data;
    }

    std::vector<std::pair<AccelerationStructureBuffers, dx::XMMATRIX>> RTObjectData::GetInstances() const
    {
        std::vector<std::pair<AccelerationStructureBuffers, dx::XMMATRIX>> instances;
        for (size_t i = 0; i < BLASBuffers.size(); ++i) {
            instances.emplace_back(BLASBuffers[i], Transforms[i]);
        }
        return instances;
    }
    
    void RTObjectData::UpdateTLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* list)
    {
        UpdateTopLevelAS(device, list, TLASBuffers, GetInstances());
    }


    void UpdateBottomLevelAS(
        ID3D12Device5* device,
        ID3D12GraphicsCommandList4* cmdList,
        AccelerationStructureBuffers& blasBuffers,
        const std::vector<AccelerationStructureInput>& geometries)
    {
        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;
        geometryDescs.reserve(geometries.size());
        for (const auto& geom : geometries) {
            D3D12_RAYTRACING_GEOMETRY_DESC desc = {};
            desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            desc.Flags = geom.flags;

            desc.Triangles.VertexBuffer.StartAddress =
                geom.vertexBuffer->GetGPUVirtualAddress() + geom.vertexOffset * geom.vertexStride;
            desc.Triangles.VertexBuffer.StrideInBytes = geom.vertexStride;
            desc.Triangles.VertexCount = geom.vertexCount;
            desc.Triangles.VertexFormat = geom.vertexFormat;

            desc.Triangles.IndexBuffer =
                geom.indexBuffer->GetGPUVirtualAddress() + geom.indexOffset * geom.indexStride;
            desc.Triangles.IndexCount = geom.indexCount;
            desc.Triangles.IndexFormat = geom.indexFormat;

            geometryDescs.push_back(desc);
        }

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
        inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        inputs.NumDescs = static_cast<UINT>(geometryDescs.size());
        inputs.pGeometryDescs = geometryDescs.data();
        inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE |
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
        buildDesc.Inputs = inputs;
        buildDesc.DestAccelerationStructureData = blasBuffers.pResult->GetGPUVirtualAddress();
        buildDesc.SourceAccelerationStructureData = blasBuffers.pResult->GetGPUVirtualAddress(); 
        buildDesc.ScratchAccelerationStructureData = blasBuffers.pScratch->GetGPUVirtualAddress();

        cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

        D3D12_RESOURCE_BARRIER uavBarrier = {};
        uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        uavBarrier.UAV.pResource = blasBuffers.pResult.Get();
        cmdList->ResourceBarrier(1, &uavBarrier);
    }


}