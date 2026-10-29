#pragma once

#include <Config/NeuralShadowConfig.h>
#include <D3DFramework/GraphicUtilites/BufferManager.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>

class NeuralShadowInputCapture {
  public:
    struct Inputs {
        FD3DW::FResource *WorkItems = nullptr;
        FD3DW::FResource *WorkHeader = nullptr;
        FD3DW::FResource *ShadowMetadata = nullptr;
        FD3DW::FResource *BlockerDistanceAtlas = nullptr;
        FD3DW::FResource *BlockerValidityAtlas = nullptr;
        std::uint32_t ScreenWidth = 0u;
        std::uint32_t ScreenHeight = 0u;
        std::uint32_t ShadowMetadataCount = 0u;
    };

    bool Initialize(ID3D12Device *device, std::string &error);
    bool Record(ID3D12GraphicsCommandList *list, const Inputs &inputs, std::string &error);
    
    FD3DW::FResource *GetBlockerValues() const;
    FD3DW::FResource *GetInputValidity() const;

  private:
    struct Params {
        std::uint32_t ScreenWidth = 0u;
        std::uint32_t ScreenHeight = 0u;
        std::uint32_t AtlasWidth = 0u;
        std::uint32_t AtlasHeight = 0u;
        std::uint32_t ShadowMetaCount = 0u;
        std::uint32_t WorkCapacity = NSOfflineConfig::NeuralShadows::InputCaptureWorkCapacity;
        std::uint32_t LayoutVersion = NSOfflineConfig::NeuralShadows::InputCaptureLayout;
        std::uint32_t Padding0 = 0u;
    };

    ID3D12Device *m_pDevice = nullptr;
    
    std::unique_ptr<FD3DW::UploadBuffer<Params>> m_pParams;
    std::unique_ptr<FD3DW::StructuredBuffer> m_pBlockerValues;
    std::unique_ptr<FD3DW::StructuredBuffer> m_pInputValidity;
    std::unique_ptr<FD3DW::SRV_UAVPacker> m_pInputs;
};
