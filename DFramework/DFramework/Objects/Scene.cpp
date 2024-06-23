#include "../pch.h"
#include "Scene.h"


namespace FDW
{
	static std::vector<SceneVertexFrameWork> vertices;
	static std::vector<std::uint16_t> indices;

	Scene::Scene(std::string path, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, bool neverUpdate)
	{
		InitScene(path, pDevice, pCommandList, neverUpdate);

		CONSOLE_MESSAGE(std::string("SCENE WITH PATH: " + path + " INITED!"));
	}

	Scene::~Scene()
	{
	}

	std::vector<dx::XMMATRIX> Scene::PlayAnimation(double time, std::string animationName)
	{
		auto animation = animationsMap.find(animationName);
		if (animation == animationsMap.end())
		{
			CONSOLE_MESSAGE(std::string("ANIMATION MAP DOESN'T HAVE PARSED ANIMATION WITH NAME: ") + animationName);
		}
		else
		{
			auto identity = dx::XMMatrixIdentity();
			
			GetPose(animation->second, mainBone, time, resultBones, identity);
		}
		return resultBones;
	}

	size_t Scene::GetBonesCount() const
	{
		return bonesCount;
	}

	void Scene::InitScene(std::string& path, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, bool neverUpdate)
	{
		ParseScene(path, pDevice, pCommandList);

		BufferMananger::CreateDefaultBuffer(pDevice, pCommandList, vertices.data(), vertices.size(), vertexBuffer, pVertexUploadBuffer);

		vertexBufferView = std::make_unique<D3D12_VERTEX_BUFFER_VIEW>();
		vertexBufferView->BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexBufferView->SizeInBytes = vertices.size() * sizeof(SceneVertexFrameWork);
		vertexBufferView->StrideInBytes = sizeof(SceneVertexFrameWork);

		BufferMananger::CreateDefaultBuffer(pDevice, pCommandList, indices.data(), indices.size(), indexBuffer, pIndexUploadBuffer);

		indexBufferView = std::make_unique<D3D12_INDEX_BUFFER_VIEW>();
		indexBufferView->BufferLocation = indexBuffer->GetGPUVirtualAddress();
		indexBufferView->Format = DXGI_FORMAT_R16_UNORM;
		indexBufferView->SizeInBytes = indices.size() * sizeof(decltype(indices[0]));

		if (neverUpdate)
		{
			pIndexUploadBuffer.release();
			pVertexUploadBuffer.release();
			CONSOLE_MESSAGE(std::string("SCENE WITH PATH: " + path + " CLEARING UPLOAD BUFFERS!"));
		}

		vertices.clear();
		vertices.shrink_to_fit();
		indices.clear();
		indices.shrink_to_fit();
	}

	void Scene::ParseScene(std::string& file, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList)
	{
		CONSOLE_MESSAGE(std::string("START PARSING SCENE WITH PATH: " + file));

		std::filesystem::path path(file);
		
		const aiScene* scene = nullptr;
		Assimp::Importer importer;
		
		scene = importer.ReadFile(file,
			aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_CalcTangentSpace );
		SAFE_ASSERT(scene, std::string("SCENE PATH: " + file + "  READ ERROR || ASSIMP ERROR INFO: " + importer.GetErrorString()).c_str());

		int verticesSize = 0;
		int offsetVertices = 0;
		int indicesSize = 0;
		
		std::unordered_map<std::string, Bone> bonesMap;

		for (int i = 0; i < scene->mNumMeshes; i++)
		{
			const aiMesh* mesh = scene->mMeshes[i];
			
			offsetVertices += verticesSize;
			vertices.insert(vertices.end(), mesh->mNumVertices, SceneVertexFrameWork());
			for (int j = 0; j < mesh->mNumVertices; j++)
			{
				
				vertices[j + offsetVertices].pos.x = mesh->mVertices[j].x;
				vertices[j + offsetVertices].pos.y = mesh->mVertices[j].y;
				vertices[j + offsetVertices].pos.z = mesh->mVertices[j].z;

				if (mesh->HasTextureCoords(0))
				{
					vertices[j + offsetVertices].texCoord.x = mesh->mTextureCoords[0][j].x;
					vertices[j + offsetVertices].texCoord.y = mesh->mTextureCoords[0][j].y;
				}

				vertices[j + offsetVertices].normal.x = mesh->mNormals[j].x;
				vertices[j + offsetVertices].normal.y = mesh->mNormals[j].y;
				vertices[j + offsetVertices].normal.z = mesh->mNormals[j].z;

				vertices[j + offsetVertices].tangent = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
				vertices[j + offsetVertices].bitangent = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
				if (mesh->HasTangentsAndBitangents())
				{
					vertices[j + offsetVertices].tangent.x = mesh->mTangents[j].x;
					vertices[j + offsetVertices].tangent.y = mesh->mTangents[j].y;
					vertices[j + offsetVertices].tangent.z = mesh->mTangents[j].z;

					vertices[j + offsetVertices].bitangent.x = mesh->mBitangents[j].x;
					vertices[j + offsetVertices].bitangent.y = mesh->mBitangents[j].y;
					vertices[j + offsetVertices].bitangent.z = mesh->mBitangents[j].z;
				}

			}
			verticesSize = vertices.size() - offsetVertices;
			indicesSize = mesh->mNumFaces * 3;
			for (int j = 0; j < mesh->mNumFaces; j++)
			{
				indices.push_back(mesh->mFaces[j].mIndices[0]);
				indices.push_back(mesh->mFaces[j].mIndices[1]);
				indices.push_back(mesh->mFaces[j].mIndices[2]);
			}
			
			if (mesh->HasBones())
			{
				std::vector<int>weightsCurrIndex(verticesSize, 0);
				for (unsigned ind = 0; ind < mesh->mNumBones; ind++)
				{
					auto bone = mesh->mBones[ind];

					if (bonesMap.find(bone->mName.C_Str()) == bonesMap.end())
					{
						bonesMap.emplace(bone->mName.C_Str(), Bone{"", ind, ConvertFromAIMatrix4x4(bone->mOffsetMatrix)});
					}

					for (int j = 0; j < bone->mNumWeights; j++)
					{
						auto& currvertexWeightIndex = weightsCurrIndex[bone->mWeights[j].mVertexId];
						if (bone->mWeights[j].mWeight == 0.0f || currvertexWeightIndex >= NUM_BONES_PER_VEREX) continue;
						vertices[bone->mWeights[j].mVertexId + offsetVertices].IDs[currvertexWeightIndex] = ind;
						vertices[bone->mWeights[j].mVertexId + offsetVertices].Weights[currvertexWeightIndex++] = bone->mWeights[j].mWeight;
					}
				}
			}

			objectParameters.push_back(std::make_tuple<size_t, size_t, size_t, size_t, size_t>(verticesSize,
				offsetVertices,
				indicesSize,
				indices.size() - indicesSize,
				mesh->mMaterialIndex));
			
		}

		for (int i = 0; i < scene->mNumMaterials; i++)
		{
			MaterialFrameWork matDesc;
			float opacity = 1.0f;
			aiColor3D tColor = { 0.0f,0.0f,0.0f };
			std::string prePathName = path.parent_path().string();
			prePathName == "" ? prePathName : prePathName += '/';
			aiString pathname;

			matMananger->AddMaterial();

			if (scene->mMaterials[i]->Get(AI_MATKEY_OPACITY, matDesc.diffuse.w) != AI_SUCCESS)
			{
				matDesc.diffuse.w = 1.0f;
			}
			if (AI_SUCCESS == scene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, tColor))
			{
				matDesc.diffuse.x = tColor.r;
				matDesc.diffuse.y = tColor.g;
				matDesc.diffuse.z = tColor.b;
			}
			if (AI_SUCCESS == scene->mMaterials[i]->Get(AI_MATKEY_COLOR_SPECULAR, tColor))
			{
				matDesc.specular.x = tColor.r;
				matDesc.specular.y = tColor.g;
				matDesc.specular.z = tColor.b;
			}
			if (AI_SUCCESS != scene->mMaterials[i]->Get(AI_MATKEY_SPECULAR_FACTOR, matDesc.specular.w))
			{
				matDesc.specular.w != 1.0f;
			}
			if (AI_SUCCESS == scene->mMaterials[i]->Get(AI_MATKEY_COLOR_EMISSIVE, tColor))
			{
				matDesc.emissive.x = tColor.r;
				matDesc.emissive.y = tColor.g;
				matDesc.emissive.z = tColor.b;
			}
			if (AI_SUCCESS == scene->mMaterials[i]->Get(AI_MATKEY_COLOR_AMBIENT, tColor))
			{
				matDesc.ambient.x = tColor.r;
				matDesc.ambient.y = tColor.g;
				matDesc.ambient.z = tColor.b;
			}
			
			matMananger->SetMaterialDesc(matDesc);
			//////////////////////////////////////////////////////////////////////////////////////
			//									   TEXTURES
			/////////////////////////////////////////////////////////////////////////////////////
			std::string name;
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				matMananger->SetTexture(name, TextureType::BASE, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType_NORMALS, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				matMananger->SetTexture(name, TextureType::NORMAL, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_DIFFUSE_ROUGHNESS, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				matMananger->SetTexture(name, TextureType::ROUGHNESS, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_EMISSIVE, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				matMananger->SetTexture(name, TextureType::EMISSIVE, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_SPECULAR, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				matMananger->SetTexture(name, TextureType::SPECULAR, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_METALNESS, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				matMananger->SetTexture(name, TextureType::METALNESS, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_HEIGHT, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				matMananger->SetTexture(name, TextureType::HEIGHT, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_OPACITY, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				matMananger->SetTexture(name, TextureType::OPACITY, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_DISPLACEMENT, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				matMananger->SetTexture(name, TextureType::BUMP, pDevice, pCommandList);
			}
		}
		//////////////////////////////////////////////////////////////////////////////////////
		//									  ANIMATIONS
		/////////////////////////////////////////////////////////////////////////////////////

		if (scene->HasAnimations())
		{
			CONSOLE_MESSAGE(std::string("SCENE WITH PATH: " + file + " HAS ANIMATIONS!!!"));
			
			for (unsigned ind = 0; ind < scene->mNumAnimations; ind++)
			{
				auto anim = scene->mAnimations[ind];

				if (animationsMap.find(anim->mName.C_Str()) != animationsMap.end())
					continue;

				Animation animation;

				if (anim->mTicksPerSecond != 0.0f)
					animation.ticksPerSecond = anim->mTicksPerSecond;
				else
					animation.ticksPerSecond = 1;

				animation.duration = anim->mDuration * anim->mTicksPerSecond;
				animation.transformations = {};

				for (int i = 0; i < anim->mNumChannels; i++) 
				{	
					aiNodeAnim* channel = anim->mChannels[i];
					BoneTransformations transformation;
					for (int j = 0; j < channel->mNumPositionKeys; j++) 
					{
						transformation.positionTimestamps.push_back(channel->mPositionKeys[j].mTime);
						transformation.positions.push_back(ConvertFromAIVector3D(channel->mPositionKeys[j].mValue));
					}
					for (int j = 0; j < channel->mNumRotationKeys; j++) 
					{
						transformation.rotationTimestamps.push_back(channel->mRotationKeys[j].mTime);
						transformation.rotations.push_back(ConvertFromAIQuaternion(channel->mRotationKeys[j].mValue));
					}
					for (int j = 0; j < channel->mNumScalingKeys; j++) 
					{
						transformation.scaleTimestamps.push_back(channel->mScalingKeys[j].mTime);
						transformation.scales.push_back(ConvertFromAIVector3D(channel->mScalingKeys[j].mValue));
					}
					animation.transformations.emplace(channel->mNodeName.C_Str(), transformation);
				}

				animationsMap.emplace(anim->mName.C_Str(), animation);
			}
		}

		for (auto& el : vertices)
		{
			float totalWeight = 0;
			for (unsigned ind = 0; ind < NUM_BONES_PER_VEREX; ind++)
			{
				totalWeight += el.Weights[ind];
			}

			if (totalWeight > 0.0f)
			{
				for (unsigned ind = 0; ind < NUM_BONES_PER_VEREX; ind++)
				{
					el.Weights[ind] /= totalWeight;
				}
			}
		}

		bonesCount = bonesMap.size();
		resultBones.assign(bonesCount, dx::XMMatrixIdentity());
		ReadSkeleton(mainBone, scene->mRootNode, bonesMap);
		
		globalInverseTransform = ConvertFromAIMatrix4x4(scene->mRootNode->mTransformation);

		importer.FreeScene();
		scene = nullptr;

		CONSOLE_MESSAGE(std::string("END PARSING SCENE WITH PATH: " + file));
	}

	void Scene::GetPose(Animation& animation, Bone& skeletion, float dt, std::vector<dx::XMMATRIX>& output, dx::XMMATRIX& parentTransform)
	{
		auto iter = animation.transformations.find(skeletion.name);
		dx::XMMATRIX globalTransform = dx::XMMatrixIdentity();
		if (iter != animation.transformations.end())
		{
			BoneTransformations& bt = animation.transformations[skeletion.name];

			std::pair<unsigned, float> fp;
			fp = GetTimeKeyAndFrac(bt.positionTimestamps, dt, animation.ticksPerSecond, animation.duration);

			dx::XMVECTOR position1 = bt.positions[fp.first - 1];
			dx::XMVECTOR position2 = bt.positions[fp.first];
			
			dx::XMVECTOR position = dx::XMVectorLerp(position1, position2, fp.second);

			fp = GetTimeKeyAndFrac(bt.rotationTimestamps, dt, animation.ticksPerSecond, animation.duration);

			dx::XMVECTOR rotation1 = bt.rotations[fp.first - 1];
			dx::XMVECTOR rotation2 = bt.rotations[fp.first];

			dx::XMVECTOR rotation = dx::XMQuaternionSlerp(rotation1, rotation2, fp.second);

			fp = GetTimeKeyAndFrac(bt.scaleTimestamps, dt, animation.ticksPerSecond, animation.duration);
			
			dx::XMVECTOR scale1 = bt.scales[fp.first - 1];
			dx::XMVECTOR scale2 = bt.scales[fp.first];

			dx::XMVECTOR scale = dx::XMVectorLerp(scale1, scale2, fp.second);

			dx::XMMATRIX positionMat = dx::XMMatrixTranslation(position.vector4_f32[0], position.vector4_f32[1], position.vector4_f32[2]);
			dx::XMMATRIX rotationMat = dx::XMMatrixRotationQuaternion(rotation);
			dx::XMMATRIX scaleMat = dx::XMMatrixScaling(scale.vector4_f32[0], scale.vector4_f32[1], scale.vector4_f32[2]);
			dx::XMMATRIX localTransform = positionMat * rotationMat * scaleMat;
			globalTransform = localTransform * parentTransform;
		}
		output[skeletion.index] =  dx::XMMatrixTranspose(skeletion.offset * globalTransform * globalInverseTransform );
		
		for (Bone& child : skeletion.children) 
		{
			GetPose(animation, child, dt, output, globalTransform);
		}
	}

	std::pair<unsigned, float> Scene::GetTimeKeyAndFrac(std::vector<float>& times, float& dt, const float& animTick, const float& duration)
	{
		double ticksPerSecond = animTick != 0.0f ? animTick : 25.0f;
		double timeInTicks = dt * ticksPerSecond;
		double animationTime = fmod(timeInTicks, duration);

		unsigned segment = 0;
		float mod = animationTime / times[times.size() - 1];
		mod = mod - std::floor(mod);
		float tempDT = times[times.size() - 1] * mod;

		while (tempDT > times[segment])
			segment++;
		
		if (!segment) segment++;

		float start = times[segment - 1];
		float end = times[segment];
		float frac = (tempDT - start) / (end - start);
		return { segment, frac };
	}

	bool Scene::ReadSkeleton(Bone& boneOutput, aiNode* node, std::unordered_map<std::string, Bone>& bonesMap) 
	{
		if (bonesMap.find(node->mName.C_Str()) != bonesMap.end()) {
			boneOutput.name = node->mName.C_Str();
			boneOutput.index = bonesMap[boneOutput.name].index;
			boneOutput.offset = bonesMap[boneOutput.name].offset;

			for (int i = 0; i < node->mNumChildren; i++) {
				Bone child;
				ReadSkeleton(child, node->mChildren[i], bonesMap);
				boneOutput.children.push_back(child);
			}
			return true;
		}
		else {
			for (int i = 0; i < node->mNumChildren; i++) {
				if (ReadSkeleton(boneOutput, node->mChildren[i], bonesMap)) {
					return true;
				}

			}
		}
		return false;
	}

}