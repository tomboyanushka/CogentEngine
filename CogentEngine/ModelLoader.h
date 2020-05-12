#pragma once
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include "Constants.h"
#include "Mesh.h"
#include <vector>
#include "Vertex.h"

struct MeshMaterial
{
	std::string Diffuse;
	std::string Normal;
	std::string Roughness;
	std::string Metalness;
};

struct ModelData
{
	Mesh*						Mesh;
	std::vector<MeshMaterial>	Materials;
};

class ModelLoader
{
public:
	static Assimp::Importer importer;

	static void ProcessMesh(aiMesh* mesh, const aiScene* scene, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

	static Mesh* Load(std::string filename, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

	// Loading a model with multiple meshes
	static ModelData LoadComplexModel(std::string filename, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

};

