#include "../pch.h"
#include "../GraphicUtilites/Materials.h"

namespace FDW
{

    class MaterialsManager
    {
   
    public:
        MaterialsManager() = default;
        ~MaterialsManager() = default;

        void AddMaterial();
        void SetTexture(std::string& texturePath, TEXTURE_TYPE type, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const std::size_t keyName = -1);
        void SetMaterialDesc(const MaterialFrameWork& materialDesc, const std::size_t keyName = -1);

        MaterialFrameWork GetMaterialDesc(size_t index) const;
        size_t GetMaterialSize() const;
        
        Material* GetMaterial(size_t index) const;

    private:
        void SetMaterialDesc(std::unique_ptr<Material>& pMaterial, const MaterialFrameWork& materialDesc);
        void SetTexture(std::unique_ptr<Material>& pMaterial, std::string& texturePath, TEXTURE_TYPE type, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList);

        std::vector<std::unique_ptr<Material>> materials;
    };



}