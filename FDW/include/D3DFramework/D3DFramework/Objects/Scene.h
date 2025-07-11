#pragma once
#include "../pch.h"
#include "Object.h"


namespace FD3DW
{
	struct Bone
	{
		std::string Name = "";
		unsigned Index = 0;
		dx::XMMATRIX Offset = dx::XMMatrixIdentity();
		std::vector<Bone> Children = {};
	};

	struct BoneTransformations {
		std::vector<double> PositionTimestamps = {};
		std::vector<double> RotationTimestamps = {};
		std::vector<double> ScaleTimestamps = {};

		std::vector<dx::XMVECTOR> Positions = {};
		std::vector<dx::XMVECTOR> Rotations = {};
		std::vector<dx::XMVECTOR> Scales = {};
	};

	struct Animation {
		double Duration = 0.0;
		double TicksPerSecond = 1.0;
		std::unordered_map<std::string, BoneTransformations> Transformations = {};
	};

	class Scene
		: public Object
	{

	public:

		Scene(std::string path, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, bool neverUpdate);
		Scene() = default;
		virtual ~Scene() = default;;

		std::vector<dx::XMMATRIX> PlayAnimation(float time, std::string animationName);
		
		std::vector<std::string> GetAnimations();

		size_t GetBonesCount() const;
		std::string GetPath() const;

	protected:

		std::pair<unsigned, float> GetTimeKeyAndFrac(std::vector<double>& times, double& dt, const double& animTick, const double& duration);
		void GetPose(Animation& animation, Bone& skeletion, double dt, std::vector<dx::XMMATRIX>& output, dx::XMMATRIX& parentTransform);

		bool ReadSkeleton(Bone& boneOutput, aiNode* node, std::unordered_map<std::string, Bone>& bonesMap);
		
		void InitScene(std::string& path, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, bool neverUpdate);
		void ParseScene(std::string& path, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList);

		std::unique_ptr<FD3DW::UploadBuffer<FD3DW::SceneVertexFrameWork>> m_pVertexUploadBuffer;
		std::unique_ptr<FD3DW::UploadBuffer<std::uint32_t>> m_pIndexUploadBuffer;

		dx::XMMATRIX m_xGlobalInverseTransform;
		Bone m_xMainBone;
		size_t m_uBonesCount;

		std::unordered_map<std::string, Animation> m_mAnimationsMap;
		std::vector<dx::XMMATRIX> m_vResultBones;

		std::string m_sPath;
	};

}