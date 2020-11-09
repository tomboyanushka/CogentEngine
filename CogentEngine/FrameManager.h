#pragma once

#include "Constants.h"
#include "ConstantBuffer.h"
#include "ConstantBufferView.h"
#include "DXUtility.h"
#include "Entity.h"
#include "Material.h"
#include "Texture.h"
#include <map>


class FrameManager
{
public:
	void Initialize(ID3D12Device* device);
	ConstantBufferView CreateConstantBufferView(uint32_t bufferSize);
	Material CreateMaterial(
		const std::string& diffuseTextureFileName,
		const std::string& normalTextureFileName,
		const std::string& metalTextureFileName,
		const std::string& roughTextureFileName,
		ID3D12CommandQueue* commandQueue,
		TextureType type = WIC);

	Texture CreateTexture(
		const std::string& textureFileName,
		ID3D12CommandQueue* commandQueue,
		TextureType type = WIC);

	ID3D12Resource* CreateResource(
		ID3D12CommandQueue* commandQueue, D3D12_RESOURCE_FLAGS flags);

	Texture CreateTextureFromResource(
		ID3D12CommandQueue* commandQueue,
		ID3D12Resource* resource);

	Entity* CreateEntity(Mesh* mesh, Material* material);
	Entity* CreateTransparentEntity(Mesh* mesh, Material* material);
	void CopyData(void* data, uint32_t size, ConstantBufferView cbv, uint32_t backBufferIndex);

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t index, uint32_t backBufferIndex);
	DescriptorHeap& GetGPUDescriptorHeap(uint32_t backBufferIndex);
	DescriptorHeap GetGPUDescriptorHeap();


	

private:
	uint32_t frameHeapCounter = 0;
	uint32_t frameIndex;
	uint32_t constantBufferIndex = 0;
	uint32_t cbOffset = 0;

	// This heap will store the descriptor to the constant buffer
	GPUConstantBuffer gpuConstantBuffer[cFrameBufferCount];

	// This is the memory on the GPU where the constant buffer will be placed.
	DescriptorHeap gpuHeap;
	DescriptorHeap materialHeap;
	DescriptorHeap textureHeap;
	ID3D12Device* device;

	std::map<std::string, Texture> textureMap;
	std::map<std::string, Material> materialMap;
	std::vector<std::string> materialID;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resources;

	int count = 0;
};

