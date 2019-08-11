#include "Material.h"

uint32_t Material::Create(ID3D12Device* device, 
	const wchar_t* diffuseTextureFileName, 
	const wchar_t* normalTextureFileName, 
	ID3D12CommandQueue* commandQueue, 
	uint32_t index, 
	const DescriptorHeap& heap)
{
	diffuseTexture.Create(device, diffuseTextureFileName, commandQueue, index, heap);
	normalTexture.Create(device, normalTextureFileName, commandQueue, index + 1, heap);

	return index + 2; //returning next index to be used for a new material
}

D3D12_GPU_DESCRIPTOR_HANDLE Material::GetFirstGPUHandle()
{
	return diffuseTexture.GetGPUHandle();
}

