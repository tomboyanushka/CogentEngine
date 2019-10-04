#pragma once
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <vector>
#include "Vertex.h"
#include "Mesh.h"

class ModelLoader
{
public:
	static Assimp::Importer importer;
	//static ModelLoader* Instance;

	static void ProcessMesh(
		aiMesh* mesh,
		const aiScene* scene,
		std::vector<Vertex>& vertices,
		std::vector<uint32_t>& indices);

	static Mesh* Load(std::string filename, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
};

