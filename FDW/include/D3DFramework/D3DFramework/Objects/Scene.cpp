#include "../pch.h"
#include "Scene.h"


namespace FD3DW
{
	static std::vector<SceneVertexFrameWork> vertices;
	static std::vector<std::uint32_t> indices;

	Scene::Scene(std::string path, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, bool neverUpdate)
	{
		m_sPath = path;
		InitScene(path, pDevice, pCommandList, neverUpdate);

		CONSOLE_MESSAGE(std::string("SCENE WITH PATH: " + path + " INITED!"));
	}

	std::vector<dx::XMMATRIX> Scene::PlayAnimation(float time, std::string animationName)
	{
		auto animation = m_mAnimationsMap.find(animationName);
		if (animation == m_mAnimationsMap.end())
		{
			CONSOLE_MESSAGE(std::string("ANIMATION MAP DOESN'T HAVE PARSED ANIMATION WITH NAME: ") + animationName);
		}
		else
		{
			auto identity = dx::XMMatrixIdentity();
			
			GetPose(animation->second, m_xMainBone, time, m_vResultBones, identity);
		}
		return m_vResultBones;
	}

	std::vector<std::string> Scene::GetAnimations()
	{
		std::vector<std::string> ret;
		ret.reserve(m_mAnimationsMap.size());
		for (const auto& [name, anim] : m_mAnimationsMap) {
			ret.push_back(name);

		}
		return ret;
	}

	size_t Scene::GetBonesCount() const
	{
		return m_uBonesCount;
	}

	std::string Scene::GetPath() const
	{
		return m_sPath;
	}

	size_t Scene::GetVertexStructSize() const
	{
		return sizeof(SceneVertexFrameWork);
	}

	void Scene::InitScene(std::string& path, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, bool neverUpdate)
	{
		ParseScene(path, pDevice, pCommandList);

		BufferManager::CreateDefaultBuffer(pDevice, pCommandList, vertices.data(), (UINT)vertices.size(), m_pVertexBuffer, m_pVertexUploadBuffer);

		m_pVertexBufferView = std::make_unique<D3D12_VERTEX_BUFFER_VIEW>();
		m_pVertexBufferView->BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
		m_pVertexBufferView->SizeInBytes =(UINT)(vertices.size() * sizeof(SceneVertexFrameWork));
		m_pVertexBufferView->StrideInBytes = sizeof(SceneVertexFrameWork);

		BufferManager::CreateDefaultBuffer(pDevice, pCommandList, indices.data(), (UINT)indices.size(), m_pIndexBuffer, m_pIndexUploadBuffer);

		m_pIndexBufferView = std::make_unique<D3D12_INDEX_BUFFER_VIEW>();
		m_pIndexBufferView->BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
		m_pIndexBufferView->Format =  DEFAULT_INDEX_BUFFER_FORMAT;
		m_pIndexBufferView->SizeInBytes = (UINT)(indices.size() * sizeof(decltype(indices[0])));

		if (neverUpdate)
		{
			m_pIndexUploadBuffer.release();
			m_pVertexUploadBuffer.release();
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

		size_t verticesSize = 0;
		size_t offsetVertices = 0;
		size_t indicesSize = 0;
		
		std::unordered_map<std::string, Bone> bonesMap;

		for (size_t i = 0; i < scene->mNumMeshes; i++)
		{
			const aiMesh* mesh = scene->mMeshes[i];
			
			offsetVertices += verticesSize;
			vertices.insert(vertices.end(), mesh->mNumVertices, SceneVertexFrameWork());
			for (size_t j = 0; j < mesh->mNumVertices; j++)
			{
				
				vertices[j + offsetVertices].Pos.x = mesh->mVertices[j].x;
				vertices[j + offsetVertices].Pos.y = mesh->mVertices[j].y;
				vertices[j + offsetVertices].Pos.z = mesh->mVertices[j].z;

				if (mesh->HasTextureCoords(0))
				{
					vertices[j + offsetVertices].TexCoord.x = mesh->mTextureCoords[0][j].x;
					vertices[j + offsetVertices].TexCoord.y = mesh->mTextureCoords[0][j].y;
				}

				vertices[j + offsetVertices].Normal.x = mesh->mNormals[j].x;
				vertices[j + offsetVertices].Normal.y = mesh->mNormals[j].y;
				vertices[j + offsetVertices].Normal.z = mesh->mNormals[j].z;

				vertices[j + offsetVertices].Tangent = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
				vertices[j + offsetVertices].Bitangent = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
				if (mesh->HasTangentsAndBitangents())
				{
					vertices[j + offsetVertices].Tangent.x = mesh->mTangents[j].x;
					vertices[j + offsetVertices].Tangent.y = mesh->mTangents[j].y;
					vertices[j + offsetVertices].Tangent.z = mesh->mTangents[j].z;

					vertices[j + offsetVertices].Bitangent.x = mesh->mBitangents[j].x;
					vertices[j + offsetVertices].Bitangent.y = mesh->mBitangents[j].y;
					vertices[j + offsetVertices].Bitangent.z = mesh->mBitangents[j].z;
				}

			}
			verticesSize = vertices.size() - offsetVertices;
			indicesSize = mesh->mNumFaces * 3;
			for (size_t j = 0; j < mesh->mNumFaces; j++)
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

					for (unsigned j = 0; j < bone->mNumWeights; j++)
					{
						auto& currvertexWeightIndex = weightsCurrIndex[bone->mWeights[j].mVertexId];
						if (bone->mWeights[j].mWeight == 0.0f || currvertexWeightIndex >= NUM_BONES_PER_VEREX) continue;
						vertices[bone->mWeights[j].mVertexId + offsetVertices].IDs[currvertexWeightIndex] = ind;
						vertices[bone->mWeights[j].mVertexId + offsetVertices].Weights[currvertexWeightIndex++] = bone->mWeights[j].mWeight;
					}
				}
			}

			m_vObjectParameters.push_back(ObjectDesc{ UINT(verticesSize), UINT(offsetVertices), UINT(indicesSize), UINT(indices.size() - indicesSize), UINT(mesh->mMaterialIndex) });
		}

		for (size_t i = 0; i < scene->mNumMaterials; i++)
		{
			MaterialFrameWork matDesc;
			float opacity = 1.0f;
			aiColor3D tColor = { 0.0f,0.0f,0.0f };
			std::string prePathName = path.parent_path().string();
			prePathName == "" ? prePathName : prePathName += '/';
			aiString pathname;

			m_pMaterialManager->AddMaterial();

			if (scene->mMaterials[i]->Get(AI_MATKEY_OPACITY, matDesc.Diffuse.w) != AI_SUCCESS)
			{
				matDesc.Diffuse.w = 1.0f;
			}
			if (AI_SUCCESS == scene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, tColor))
			{
				matDesc.Diffuse.x = tColor.r;
				matDesc.Diffuse.y = tColor.g;
				matDesc.Diffuse.z = tColor.b;
			}
			if (AI_SUCCESS == scene->mMaterials[i]->Get(AI_MATKEY_COLOR_SPECULAR, tColor))
			{
				matDesc.Specular.x = tColor.r;
				matDesc.Specular.y = tColor.g;
				matDesc.Specular.z = tColor.b;
			}
			if (AI_SUCCESS != scene->mMaterials[i]->Get(AI_MATKEY_SPECULAR_FACTOR, matDesc.Specular.w))
			{
				matDesc.Specular.w = 1.0f;
			}
			if (AI_SUCCESS == scene->mMaterials[i]->Get(AI_MATKEY_COLOR_EMISSIVE, tColor))
			{
				matDesc.Emissive.x = tColor.r;
				matDesc.Emissive.y = tColor.g;
				matDesc.Emissive.z = tColor.b;
			}
			if (AI_SUCCESS == scene->mMaterials[i]->Get(AI_MATKEY_COLOR_AMBIENT, tColor))
			{
				matDesc.Ambient.x = tColor.r;
				matDesc.Ambient.y = tColor.g;
				matDesc.Ambient.z = tColor.b;
			}
			if (AI_SUCCESS != scene->mMaterials[i]->Get(AI_MATKEY_SHININESS, matDesc.SpecularPower) ){
				matDesc.SpecularPower = _DEFAULT_MATERIAL_NOT_LOADED_DATA;
			}
			if (AI_SUCCESS != scene->mMaterials[i]->Get(AI_MATKEY_ROUGHNESS_FACTOR, matDesc.Roughness)) {
				matDesc.Roughness = _DEFAULT_MATERIAL_NOT_LOADED_DATA;
			}
			if (AI_SUCCESS != scene->mMaterials[i]->Get(AI_MATKEY_METALLIC_FACTOR, matDesc.Metalness)) {
				matDesc.Metalness = _DEFAULT_MATERIAL_NOT_LOADED_DATA;
			}
			if (AI_SUCCESS != scene->mMaterials[i]->Get(AI_MATKEY_BUMPSCALING, matDesc.HeightScale)) {
				matDesc.HeightScale = _DEFAULT_MATERIAL_NOT_LOADED_DATA;
			}


			m_pMaterialManager->SetMaterialDesc(matDesc);
			//////////////////////////////////////////////////////////////////////////////////////
			//									   TEXTURES
			/////////////////////////////////////////////////////////////////////////////////////
			std::string name;
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				m_pMaterialManager->SetTexture(name, TextureType::BASE, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType_NORMALS, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				m_pMaterialManager->SetTexture(name, TextureType::NORMAL, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_DIFFUSE_ROUGHNESS, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				m_pMaterialManager->SetTexture(name, TextureType::ROUGHNESS, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_EMISSIVE, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				m_pMaterialManager->SetTexture(name, TextureType::EMISSIVE, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_SPECULAR, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				m_pMaterialManager->SetTexture(name, TextureType::SPECULAR, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_METALNESS, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				m_pMaterialManager->SetTexture(name, TextureType::METALNESS, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_HEIGHT, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				m_pMaterialManager->SetTexture(name, TextureType::HEIGHT, pDevice, pCommandList);
			}	
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_OPACITY, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				m_pMaterialManager->SetTexture(name, TextureType::OPACITY, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_AMBIENT, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				m_pMaterialManager->SetTexture(name, TextureType::AMBIENT, pDevice, pCommandList);
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

				if (m_mAnimationsMap.find(anim->mName.C_Str()) != m_mAnimationsMap.end())
					continue;

				Animation animation;

				if (anim->mTicksPerSecond != 0.0)
					animation.TicksPerSecond = anim->mTicksPerSecond;
				else
					animation.TicksPerSecond = 1.0;

				animation.Duration = anim->mDuration * anim->mTicksPerSecond;
				animation.Transformations = {};

				for (size_t i = 0; i < anim->mNumChannels; i++)
				{	
					aiNodeAnim* channel = anim->mChannels[i];
					BoneTransformations transformation;
					for (size_t j = 0; j < channel->mNumPositionKeys; j++)
					{
						transformation.PositionTimestamps.push_back(channel->mPositionKeys[j].mTime);
						transformation.Positions.push_back(ConvertFromAIVector3D(channel->mPositionKeys[j].mValue));
					}
					for (size_t j = 0; j < channel->mNumRotationKeys; j++)
					{
						transformation.RotationTimestamps.push_back(channel->mRotationKeys[j].mTime);
						transformation.Rotations.push_back(ConvertFromAIQuaternion(channel->mRotationKeys[j].mValue));
					}
					for (size_t j = 0; j < channel->mNumScalingKeys; j++)
					{
						transformation.ScaleTimestamps.push_back(channel->mScalingKeys[j].mTime);
						transformation.Scales.push_back(ConvertFromAIVector3D(channel->mScalingKeys[j].mValue));
					}
					animation.Transformations.emplace(channel->mNodeName.C_Str(), transformation);
				}

				m_mAnimationsMap.emplace(anim->mName.C_Str(), animation);
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

		m_uBonesCount = bonesMap.size();
		m_vResultBones.assign(m_uBonesCount, dx::XMMatrixIdentity());
		ReadSkeleton(m_xMainBone, scene->mRootNode, bonesMap);
		
		m_xGlobalInverseTransform = ConvertFromAIMatrix4x4(scene->mRootNode->mTransformation);

		importer.FreeScene();
		scene = nullptr;

		CONSOLE_MESSAGE(std::string("END PARSING SCENE WITH PATH: " + file));
	}

	void Scene::GetPose(Animation& animation, Bone& skeletion, double dt, std::vector<dx::XMMATRIX>& output, dx::XMMATRIX& parentTransform)
	{
		auto iter = animation.Transformations.find(skeletion.Name);
		dx::XMMATRIX globalTransform = dx::XMMatrixIdentity();
		if (iter != animation.Transformations.end())
		{
			BoneTransformations& bt = animation.Transformations[skeletion.Name];

			std::pair<unsigned, float> fp;

			dx::XMVECTOR position;
			dx::XMVECTOR rotation;
			dx::XMVECTOR scale;

			fp = GetTimeKeyAndFrac(bt.PositionTimestamps, dt, animation.TicksPerSecond, animation.Duration);
			if (bt.PositionTimestamps.size() > 1)
			{
				dx::XMVECTOR position1 = bt.Positions[fp.first - 1];
				dx::XMVECTOR position2 = bt.Positions[fp.first];

				position = dx::XMVectorLerp(position1, position2, fp.second);
			}
			else
			{
				position = bt.Positions[fp.first];
			}

			fp = GetTimeKeyAndFrac(bt.RotationTimestamps, dt, animation.TicksPerSecond, animation.Duration);
			if (bt.RotationTimestamps.size() > 1)
			{
				dx::XMVECTOR rotation1 = bt.Rotations[fp.first - 1];
				dx::XMVECTOR rotation2 = bt.Rotations[fp.first];
				
				rotation = dx::XMQuaternionSlerp(rotation1, rotation2, fp.second);
			}
			else
			{
				rotation = bt.Rotations[fp.first];
			}

			fp = GetTimeKeyAndFrac(bt.ScaleTimestamps, dt, animation.TicksPerSecond, animation.Duration);
			if (bt.ScaleTimestamps.size() > 1)
			{
				dx::XMVECTOR scale1 = bt.Scales[fp.first - 1];
				dx::XMVECTOR scale2 = bt.Scales[fp.first];

				scale = dx::XMVectorLerp(scale1, scale2, fp.second);
			}
			else
			{
				scale = bt.Scales[fp.first];
			}
		
			dx::XMMATRIX positionMat = dx::XMMatrixTranslation(position.vector4_f32[0], position.vector4_f32[1], position.vector4_f32[2]);
			dx::XMMATRIX rotationMat = dx::XMMatrixRotationQuaternion(rotation);
			dx::XMMATRIX scaleMat = dx::XMMatrixScaling(scale.vector4_f32[0], scale.vector4_f32[1], scale.vector4_f32[2]);
			dx::XMMATRIX localTransform = positionMat * rotationMat * scaleMat;
			globalTransform = localTransform * parentTransform;
		}
		output[skeletion.Index] =  dx::XMMatrixTranspose(skeletion.Offset * globalTransform ); // * globalInverseTransform  why this doesn't need here? ((
																							   // and why some animations incorrect (
		for (Bone& child : skeletion.Children) 
		{
			GetPose(animation, child, dt, output, globalTransform);
		}
	}

	std::pair<unsigned, float> Scene::GetTimeKeyAndFrac(std::vector<double>& times, double& dt, const double& animTick, const double& Duration)
	{
		double TicksPerSecond = animTick != 0.0 ? animTick : 25.0;
		double timeInTicks = dt * TicksPerSecond;
		double animationTime = fmod(timeInTicks, Duration);

		double frac = 1.0;
		unsigned segment = 0;
		
		double mod = animationTime / times[times.size() - 1];
		mod = mod - std::floor(mod);
		double tempDT = times[times.size() - 1] * mod;

		while (tempDT > times[segment])
			segment++;
		
		if (!segment) segment++;
		if (times.size() > 1)
		{
			double start = times[segment - 1];
			double end = times[segment];
			frac = (tempDT - start) / (end - start);
		}
		else
		{
			segment = 0;
		}
		return { segment, (float)frac };
	}

	bool Scene::ReadSkeleton(Bone& boneOutput, aiNode* node, std::unordered_map<std::string, Bone>& bonesMap) 
	{
		if (bonesMap.find(node->mName.C_Str()) != bonesMap.end()) {
			boneOutput.Name = node->mName.C_Str();
			boneOutput.Index = bonesMap[boneOutput.Name].Index;
			boneOutput.Offset = bonesMap[boneOutput.Name].Offset;

			for (unsigned i = 0; i < node->mNumChildren; i++) {
				Bone child;
				ReadSkeleton(child, node->mChildren[i], bonesMap);
				boneOutput.Children.push_back(child);
			}
			return true;
		}
		else {
			for (unsigned i = 0; i < node->mNumChildren; i++) {
				if (ReadSkeleton(boneOutput, node->mChildren[i], bonesMap)) {
					return true;
				}

			}
		}
		return false;
	}

}