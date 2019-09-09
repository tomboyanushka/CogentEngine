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

}

Mesh* ModelLoader::Load(std::string filename, ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	const aiScene* pScene = importer.ReadFile(filename,
		aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded | aiProcess_ValidateDataStructure | aiProcess_JoinIdenticalVertices);

	if (pScene == NULL)
		return nullptr;

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
