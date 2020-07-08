#include "Material.h"

uint32_t Material::Create(ID3D12Device* device, 
	const std::string& diffuseTextureFileName,
	const std::string& normalTextureFileName,
	const std::string& metalTextureFileName,
	const std::string& roughTextureFileName,
	ID3D12CommandQueue* commandQueue, 
	uint32_t index, 
	const DescriptorHeap* heap,
	uint32_t heapCount,
	TextureType type)
{

	diffuseTexture.CreateTexture(device, diffuseTextureFileName, commandQueue, index, heap, type);
	normalTexture.CreateTexture(device, normalTextureFileName, commandQueue, index + 1, heap, type);
	metalTexture.CreateTexture(device, metalTextureFileName, commandQueue, index + 2, heap, type);
	roughTexture.CreateTexture(device, roughTextureFileName, commandQueue, index + 3, heap, type);

	index += 4;
	this->materialIndex = index; 
	// Returning next index to be used for a new material
	return materialIndex;
}

D3D12_GPU_DESCRIPTOR_HANDLE Material::GetFirstGPUHandle(const DescriptorHeap* heap, uint32_t backBufferIndex)
{
	return diffuseTexture.GetGPUHandle(heap, backBufferIndex);
}

