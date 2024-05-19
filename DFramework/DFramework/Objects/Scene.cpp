#include "../pch.h"
#include "Scene.h"


namespace FDW
{


	Scene::Scene(std::string path, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, bool neverUpdate)
	{
		CONSOLE_MESSAGE(std::string("START PARSING SCENE WITH PATH: " + path));

		ParseScene(path, pDevice, pCommandList);

		CONSOLE_MESSAGE(std::string("END PARSING SCENE WITH PATH: " + path));

		FDW::BufferMananger::CreateDefaultBuffer(pDevice, pCommandList, vertices.data(), vertices.size(), vertexBuffer, pVertexUploadBuffer);

		vertexBufferView = std::make_unique<D3D12_VERTEX_BUFFER_VIEW>();
		vertexBufferView->BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexBufferView->SizeInBytes = vertices.size() * sizeof(FDW::VertexFrameWork);
		vertexBufferView->StrideInBytes = sizeof(FDW::VertexFrameWork);
		
		FDW::BufferMananger::CreateDefaultBuffer<std::uint16_t>(pDevice, pCommandList, indices.data(), indices.size(), indexBuffer, pIndexUploadBuffer);

		indexBufferView = std::make_unique<D3D12_INDEX_BUFFER_VIEW>();
		indexBufferView->BufferLocation = indexBuffer->GetGPUVirtualAddress();
		indexBufferView->Format = DXGI_FORMAT_R16_UNORM;
		indexBufferView->SizeInBytes = indices.size() * sizeof(std::uint16_t);

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

		CONSOLE_MESSAGE(std::string("SCENE WITH PATH: " + path + " INITED!"));
	}

	Scene::~Scene()
	{
	}



	void Scene::ParseScene(std::string& file, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList)
	{
		std::filesystem::path path(file);

		const aiScene* scene = nullptr;
		Assimp::Importer importer;

		scene = importer.ReadFile(file,
			aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_CalcTangentSpace);
		SAFE_ASSERT(scene, std::string("SCENE PATH: " + file + "  READ ERROR").c_str());

		int verticesSize = 0;
		int indicesSize = 0;
		for (int i = 0; i < scene->mNumMeshes; i++)
		{
			const aiMesh* mesh = scene->mMeshes[i];

			verticesSize = mesh->mNumVertices;
			for (int j = 0; j < mesh->mNumVertices; j++)
			{
				vertices.push_back({});

				
				(*(vertices.rbegin())).pos.x = mesh->mVertices[j].x;
				(*(vertices.rbegin())).pos.y = mesh->mVertices[j].y;
				(*(vertices.rbegin())).pos.z = mesh->mVertices[j].z;

				if (mesh->HasTextureCoords(0))
				{
					(*(vertices.rbegin())).texCoord.x = mesh->mTextureCoords[0][j].x;
					(*(vertices.rbegin())).texCoord.y = mesh->mTextureCoords[0][j].y;
				}

				(*(vertices.rbegin())).normal.x = mesh->mNormals[j].x;
				(*(vertices.rbegin())).normal.y = mesh->mNormals[j].y;
				(*(vertices.rbegin())).normal.z = mesh->mNormals[j].z;

				(*(vertices.rbegin())).tangent = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
				if (mesh->HasTangentsAndBitangents())
				{
					(*(vertices.rbegin())).tangent.x = mesh->mTangents[j].x;
					(*(vertices.rbegin())).tangent.y = mesh->mTangents[j].y;
					(*(vertices.rbegin())).tangent.z = mesh->mTangents[j].z;

					(*(vertices.rbegin())).bitangent.x = mesh->mBitangents[j].x;
					(*(vertices.rbegin())).bitangent.y = mesh->mBitangents[j].y;
					(*(vertices.rbegin())).bitangent.z = mesh->mBitangents[j].z;
				}

			}
			
			indicesSize = mesh->mNumFaces * 3;
			for (int j = 0; j < mesh->mNumFaces; j++)
			{
				indices.push_back(mesh->mFaces[j].mIndices[0]);
				indices.push_back(mesh->mFaces[j].mIndices[1]);
				indices.push_back(mesh->mFaces[j].mIndices[2]);
			}
			
			objectParameters.emplace_back(std::make_tuple<size_t, size_t, size_t, size_t, size_t>(verticesSize,
				vertices.size() - verticesSize,
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

			if (scene->mMaterials[i]->Get(AI_MATKEY_OPACITY, matDesc.diffuse.w) == AI_SUCCESS)
			{
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
			if (AI_SUCCESS == scene->mMaterials[i]->Get(AI_MATKEY_SPECULAR_FACTOR, matDesc.specular.w))
			{
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
				matMananger->SetTexture(name, TEXTURE_TYPE::BASE, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType_NORMALS, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				matMananger->SetTexture(name, TEXTURE_TYPE::NORMAL, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_DIFFUSE_ROUGHNESS, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				matMananger->SetTexture(name, TEXTURE_TYPE::ROUGHNESS, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_EMISSIVE, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				matMananger->SetTexture(name, TEXTURE_TYPE::EMISSIVE, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_SPECULAR, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				matMananger->SetTexture(name, TEXTURE_TYPE::SPECULAR, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_METALNESS, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				matMananger->SetTexture(name, TEXTURE_TYPE::METALNESS, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_HEIGHT, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				matMananger->SetTexture(name, TEXTURE_TYPE::HEIGHT, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_OPACITY, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				matMananger->SetTexture(name, TEXTURE_TYPE::OPACITY, pDevice, pCommandList);
			}
			if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_DISPLACEMENT, 0, &pathname))
			{
				name = prePathName + std::string(pathname.C_Str());
				matMananger->SetTexture(name, TEXTURE_TYPE::BUMP, pDevice, pCommandList);
			}
		}

		importer.FreeScene();
		scene = nullptr;
	}


}