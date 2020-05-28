#pragma once
#include "d3dx12.h"
#include "Texture.h"

struct DefaultMaterial
{
	static Texture DefaultDiffuse;
	static Texture DefaultNormal;
	static Texture DefaultRoughness;
	static Texture DefaultMetal;
};

class Material
{
public:
	uint32_t Create(ID3D12Device* device, 
		const std::string& diffuseTextureFileName,
		const std::string& normalTextureFileName,
		const std::string& metalTextureFileName,
		const std::string& roughTextureFileName,
		ID3D12CommandQueue* commandQueue, 
		uint32_t index, 
		const DescriptorHeap* heap,
		uint32_t heapCount,
		TextureType type = WIC);

	D3D12_GPU_DESCRIPTOR_HANDLE GetFirstGPUHandle(const DescriptorHeap* heap, uint32_t backBufferIndex);

	Texture diffuseTexture;
	Texture normalTexture;
	Texture metalTexture;
	Texture roughTexture;

	uint32_t materialIndex;

private:

};

