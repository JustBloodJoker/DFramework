#pragma once

#include <pch.h>
#include <MainRenderer/GlobalRenderThreadManager.h>
#include <MainRenderer/MainRendererComponent.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>


struct ClusterSystemCluster
{
    dx::XMFLOAT4 MinPoint;
    dx::XMFLOAT4  MaxPoint;
    UINT Count;
    UINT LightIndices[CLUSTERED_MAX_LIGHTS_PER_CLUSTER];
};

struct ClusterSystemClusterViewParams
{
    dx::XMFLOAT4X4 ViewMatrix;
    int LightCount;
};

struct ClusterSystemClusterParams
{
    float ZNear;
    float ZFar;
    uint32_t GridSize0;
    uint32_t GridSize1;

    uint32_t GridSize2;
    uint32_t ScreenWidth;
    uint32_t ScreenHeight;
    uint32_t _pad0;

    dx::XMFLOAT4X4 InverseProjection;
};

struct ClusterSystemClusterParamsPS {
    float ZNear;
    float ZFar;
    uint32_t GridSize0;
    uint32_t GridSize1;

    uint32_t GridSize2;
    uint32_t ScreenWidth;
    uint32_t ScreenHeight;
    uint32_t _pad0;

    dx::XMFLOAT4X4 ViewMatrix;
};

class ClusteredLightningSystem : public MainRendererComponent {
public:
	ClusteredLightningSystem() = default;
	virtual ~ClusteredLightningSystem() = default;

    virtual void AfterConstruction() override;
    std::shared_ptr<FD3DW::ExecutionHandle> OnStartRenderTick(std::shared_ptr<FD3DW::ExecutionHandle> sync);
    std::shared_ptr<FD3DW::ExecutionHandle> AssignLightsToClusters(std::vector< std::shared_ptr<FD3DW::ExecutionHandle> > syncs);

    D3D12_GPU_VIRTUAL_ADDRESS GetClusteredStructuredBufferGPULocation();
    D3D12_GPU_VIRTUAL_ADDRESS GetClusteredConstantBufferGPULocation();

protected:
	
    std::unique_ptr<FD3DW::UploadBuffer<ClusterSystemClusterViewParams>> m_pClusterViewParamsBuffer;
	std::unique_ptr<FD3DW::UploadBuffer<ClusterSystemClusterParams>> m_pClusterParamsBuffer;
	std::unique_ptr<FD3DW::UploadBuffer<ClusterSystemClusterParamsPS>> m_pClusterParamsPSBuffer;
	std::unique_ptr<FD3DW::StructuredBuffer> m_pClustersStructuredBuffer;
};