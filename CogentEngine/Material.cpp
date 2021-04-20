#include "Material.h"
#include "FrameManager.h"

uint32_t Material::materialIndexTracker = 0;

uint32_t Material::Create(ID3D12Device* device, 
	const std::string& diffuseTextureFileName,
	const std::string& normalTextureFileName,
	const std::string& metalnessTextureFileName,
	const std::string& roughnessTextureFileName,
	ID3D12CommandQueue* commandQueue, 
	const DescriptorHeap* textureHeap,
	const DescriptorHeap* materialHeap,
	FrameManager* frameManager,
	uint32_t heapCount,
	TextureType type)
{
	diffuseTexture = frameManager->CreateTexture(diffuseTextureFileName, commandQueue, type);
	normalTexture = frameManager->CreateTexture(normalTextureFileName, commandQueue, type);
	metalnessTexture = frameManager->CreateTexture(metalnessTextureFileName, commandQueue, type);
	roughnessTexture = frameManager->CreateTexture(roughnessTextureFileName, commandQueue, type);
	
	device->CopyDescriptorsSimple(1, materialHeap->handleCPU(materialIndexTracker), diffuseTexture.GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	device->CopyDescriptorsSimple(1, materialHeap->handleCPU(materialIndexTracker + 1), normalTexture.GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	device->CopyDescriptorsSimple(1, materialHeap->handleCPU(materialIndexTracker + 2), metalnessTexture.GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	device->CopyDescriptorsSimple(1, materialHeap->handleCPU(materialIndexTracker + 3), roughnessTexture.GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	this->materialCPUHandle = materialHeap->handleCPU(materialIndexTracker);

	materialIndexTracker += 4;
	// Returning next index to be used for a new material
	return materialIndexTracker;
}

D3D12_GPU_DESCRIPTOR_HANDLE Material::GetGPUHandle()
{
	return materialGPUHandle;
}

void Material::SetGPUHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle)
{
	materialGPUHandle = handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE Material::GetCPUHandle()
{
	return materialCPUHandle;
}