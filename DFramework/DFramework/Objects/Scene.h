#pragma once
#include "../pch.h"
#include "Object.h"


namespace FDW
{
	struct Bone
	{
		std::string name = "";
		unsigned index = 0;
		dx::XMMATRIX offset = dx::XMMatrixIdentity();
		std::vector<Bone> children = {};
	};

	struct BoneTransformations {
		std::vector<float> positionTimestamps = {};
		std::vector<float> rotationTimestamps = {};
		std::vector<float> scaleTimestamps = {};

		std::vector<dx::XMVECTOR> positions = {};
		std::vector<dx::XMVECTOR> rotations = {};
		std::vector<dx::XMVECTOR> scales = {};
	};

	struct Animation {
		float duration = 0.0f;
		float ticksPerSecond = 1.0f;
		std::unordered_map<std::string, BoneTransformations> transformations = {};
	};


	class Scene
		: public Object
	{

	public:

		Scene(std::string path, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, bool neverUpdate);
		Scene() = default;
		virtual ~Scene();

		std::vector<dx::XMMATRIX> PlayAnimation(double time, std::string animationName);

		size_t GetBonesCount() const;

	protected:

		std::pair<unsigned, float> GetTimeKeyAndFrac(std::vector<float>& times, float& dt, const float& animTick, const float& duration);
		void GetPose(Animation& animation, Bone& skeletion, float dt, std::vector<dx::XMMATRIX>& output, dx::XMMATRIX& parentTransform);

		bool ReadSkeleton(Bone& boneOutput, aiNode* node, std::unordered_map<std::string, Bone>& bonesMap);
		
		void InitScene(std::string& path, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, bool neverUpdate);
		void ParseScene(std::string& path, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList);

		std::unique_ptr<FDW::UploadBuffer<FDW::SceneVertexFrameWork>> pVertexUploadBuffer;
		std::unique_ptr<FDW::UploadBuffer<std::uint16_t>> pIndexUploadBuffer;

		dx::XMMATRIX globalInverseTransform;
		Bone mainBone;
		unsigned bonesCount;

		std::unordered_map<std::string, Animation> animationsMap;
		std::vector<dx::XMMATRIX> resultBones;
	};

}