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

class FrameManager;

class Material
{
public:
	uint32_t Create(ID3D12Device* device, 
		const std::string& diffuseTextureFileName,
		const std::string& normalTextureFileName,
		const std::string& metalTextureFileName,
		const std::string& roughTextureFileName,
		ID3D12CommandQueue* commandQueue, 
		const DescriptorHeap* textureHeap,
		const DescriptorHeap* materialHeap,
		FrameManager* frameManager,
		uint32_t heapCount,
		TextureType type = WIC);

	uint32_t Create(ID3D12Device* device,
		Texture& diffuse,
		Texture& normal,
		Texture& metalness,
		Texture& roughness,
		ID3D12CommandQueue* commandQueue,
		const DescriptorHeap* textureHeap,
		const DescriptorHeap* materialHeap,
		FrameManager* frameManager,
		uint32_t heapCount,
		TextureType type = WIC);

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle();
	void SetGPUHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle);

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle();

	Texture diffuseTexture;
	Texture normalTexture;
	Texture metalnessTexture;
	Texture roughnessTexture;

private:
	static uint32_t materialIndexTracker;
	D3D12_GPU_DESCRIPTOR_HANDLE materialGPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE materialCPUHandle;
};

