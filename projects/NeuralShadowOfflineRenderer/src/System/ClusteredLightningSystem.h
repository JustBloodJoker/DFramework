#pragma once

#include <Renderer/GlobalRenderThreadManager.h>
#include <Renderer/OfflineRendererComponent.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>


struct ClusterSystemCluster
{
    dx::XMFLOAT4 MinPoint;
    dx::XMFLOAT4  MaxPoint;
    UINT Count;
    UINT AffectingLightCount;
    UINT LightIndices[NSOfflineConfig::Lighting::MaxLightsPerTile];
};

struct ClusterSystemClusterViewParams
{
    dx::XMFLOAT4X4 ViewMatrix;
    int LightCount;
    UINT TileCount;
    UINT _pad0;
    UINT _pad1;
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

class ClusteredLightningSystem : public OfflineRendererComponent {
public:
	ClusteredLightningSystem() = default;
	virtual ~ClusteredLightningSystem() = default;

    virtual void AfterConstruction() override;
    std::shared_ptr<FD3DW::ExecutionHandle> OnStartRenderTick(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> syncs);
    std::shared_ptr<FD3DW::ExecutionHandle> AssignLightsToClusters(std::vector< std::shared_ptr<FD3DW::ExecutionHandle> > syncs);

    D3D12_GPU_VIRTUAL_ADDRESS GetClusteredStructuredBufferGPULocation();
protected:
	static constexpr UINT LIGHT_TILE_COUNT =
		NSOfflineConfig::Lighting::TileCountX * NSOfflineConfig::Lighting::TileCountY * NSOfflineConfig::Lighting::TileCountZ;

    std::unique_ptr<FD3DW::UploadBuffer<ClusterSystemClusterViewParams>> m_pClusterViewParamsBuffer;
	std::unique_ptr<FD3DW::UploadBuffer<ClusterSystemClusterParams>> m_pClusterParamsBuffer;
	std::unique_ptr<FD3DW::StructuredBuffer> m_pClustersStructuredBuffer;
};
