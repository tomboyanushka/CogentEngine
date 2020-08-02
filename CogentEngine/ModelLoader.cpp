#include "ModelLoader.h"

//ModelLoader* ModelLoader::Instance = nullptr;
Assimp::Importer ModelLoader::importer;

void ModelLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
{
	for (uint32_t i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;

		vertex.Position.x = mesh->mVertices[i].x;
		vertex.Position.y = mesh->mVertices[i].y;
		vertex.Position.z = mesh->mVertices[i].z;

		vertex.Normal = DirectX::XMFLOAT3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

		if (mesh->mTextureCoords[0])
		{
			vertex.UV.x = (float)mesh->mTextureCoords[0][i].x;
			vertex.UV.y = (float)mesh->mTextureCoords[0][i].y; 
		}

		vertices.push_back(vertex);
	}

	for (uint32_t i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];

		for (uint32_t j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	if (!mesh->HasNormals() || !mesh->HasTangentsAndBitangents())
	{
		std::vector<XMFLOAT3> pos;
		std::vector<XMFLOAT3> normals;
		std::vector<XMFLOAT3> tangents;
		std::vector<XMFLOAT2> uv;
		for (auto& v : vertices)
		{
			pos.push_back(v.Position);
			normals.push_back(v.Normal);
			tangents.push_back(v.Tangent);
			uv.push_back(v.UV);
		}

		if (!mesh->HasNormals())
			DirectX::ComputeNormals(indices.data(), mesh->mNumFaces, pos.data(), pos.size(), CNORM_DEFAULT, normals.data());

		if (!mesh->HasTangentsAndBitangents())
			DirectX::ComputeTangentFrame(indices.data(), mesh->mNumFaces, pos.data(), normals.data(), uv.data(), pos.size(), tangents.data(), nullptr);
		for (int i = 0; i < vertices.size(); ++i)
		{
			vertices[i].Normal = normals[i];
			vertices[i].Tangent = tangents[i];
		}
	}
}

Mesh* ModelLoader::Load(std::string filename, ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	const aiScene* pScene = importer.ReadFile(filename,
		aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded | aiProcess_ValidateDataStructure | aiProcess_JoinIdenticalVertices);

	if (pScene == NULL)
		return nullptr;

	uint32_t NumVertices = 0;
	uint32_t NumIndices = 0;
	std::vector<MeshEntry> meshEntries(pScene->mNumMeshes);

	for (uint32_t i = 0; i < meshEntries.size(); i++)
	{
		meshEntries[i].NumIndices = pScene->mMeshes[i]->mNumFaces * 3;
		meshEntries[i].BaseVertex = NumVertices;
		meshEntries[i].BaseIndex = NumIndices;

		NumVertices += pScene->mMeshes[i]->mNumVertices;
		NumIndices += meshEntries[i].NumIndices;
	}

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	for (uint32_t i = 0; i < pScene->mNumMeshes; ++i)
	{
		ProcessMesh(pScene->mMeshes[i], pScene, vertices, indices);
	}

	uint32_t vertexCount = (uint32_t)vertices.size();
	uint32_t indexCount = (uint32_t)indices.size();

	Mesh* mesh = new Mesh(vertices.data(), vertexCount, indices.data(), indexCount, device, commandList);

	return mesh;
}

// Function for loading models with sub-meshes and materials
ModelData ModelLoader::LoadComplexModel(std::string filename, ID3D12Device * device, ID3D12GraphicsCommandList * commandList)
{
	const aiScene* pScene = importer.ReadFile(filename,
		aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded | aiProcess_ValidateDataStructure | aiProcess_JoinIdenticalVertices);

	if (pScene == NULL)
	{
		return ModelData{};
	}

	uint32_t NumVertices = 0;
	uint32_t NumIndices = 0;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<MeshEntry> meshEntries(pScene->mNumMeshes);
	std::vector<MeshMaterial> materials(pScene->mNumMeshes);
	Mesh* mesh = nullptr;

	for (uint32_t i = 0; i < meshEntries.size(); ++i)
	{
		meshEntries[i].NumIndices = pScene->mMeshes[i]->mNumFaces * 3;
		meshEntries[i].BaseVertex = NumVertices;
		meshEntries[i].BaseIndex = NumIndices;

		NumVertices += pScene->mMeshes[i]->mNumVertices;
		NumIndices += meshEntries[i].NumIndices;
	}

	std::vector<Vertex> meshVertices;
	std::vector<uint32_t> meshIndices;

	for (uint32_t i = 0; i < pScene->mNumMeshes; i++)
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		ProcessMesh(pScene->mMeshes[i], pScene, vertices, indices);

		meshVertices.insert(meshVertices.end(), vertices.begin(), vertices.end());
		meshIndices.insert(meshIndices.end(), indices.begin(), indices.end());
	}

	mesh = new Mesh(meshVertices.data(), (uint32_t)meshVertices.size(), meshIndices.data(), (uint32_t)meshIndices.size(), device, commandList);

	mesh->MeshEntries = meshEntries;


	if (pScene->HasMaterials())
	{
		for (uint32_t i = 0; i < pScene->mNumMeshes; i++)
		{
			auto matId = pScene->mMeshes[i]->mMaterialIndex;
			auto mat = pScene->mMaterials[matId];
			auto c = mat->mNumProperties;
			aiString diffuseTexture;
			aiString normalTexture;
			aiString roughnessTexture;
			aiString metalnessTexture;
			MeshMaterial mMat;
			if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseTexture) == aiReturn_SUCCESS)
			{
				mMat.Diffuse = diffuseTexture.C_Str();
			}

			if (mat->GetTexture(aiTextureType_NORMALS, 0, &normalTexture) == aiReturn_SUCCESS || mat->GetTexture(aiTextureType_HEIGHT, 0, &normalTexture) == aiReturn_SUCCESS)
			{
				mMat.Normal = normalTexture.C_Str();
			}

			if (mat->GetTexture(aiTextureType_SPECULAR, 0, &roughnessTexture) == aiReturn_SUCCESS)
			{
				mMat.Roughness = roughnessTexture.C_Str();
			}

			if (mat->GetTexture(aiTextureType_AMBIENT, 0, &metalnessTexture) == aiReturn_SUCCESS)
			{
				mMat.Metalness = metalnessTexture.C_Str();
			}


			materials[i] = mMat;
		}

	}

	return ModelData{mesh, materials};
}
