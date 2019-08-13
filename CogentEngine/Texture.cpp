#include "Texture.h"
#include <WICTextureLoader.h>
#include "ResourceUploadBatch.h"
#include "DirectXHelpers.h"
using namespace DirectX;

void Texture::Create(ID3D12Device* device, const wchar_t* fileName, ID3D12CommandQueue* commandQueue, uint32_t index, const DescriptorHeap& heap, TextureType type)
{
	this->cpuHandle = heap.handleCPU(index);
	this->gpuHandle = heap.handleGPU(index);
	bool isCubeMap = false;
	ResourceUploadBatch resourceUpload(device);
	resourceUpload.Begin();
	switch (type)
	{
	case WIC:
		CreateWICTextureFromFile(device, resourceUpload, fileName, resource.GetAddressOf(), true);
		break;

	case DDS:
		CreateDDSTextureFromFile(device, resourceUpload, fileName, resource.GetAddressOf(), true, 0, nullptr, &isCubeMap);
		break;

	}
	
	auto uploadResourcesFinished = resourceUpload.End(commandQueue);
	uploadResourcesFinished.wait();

	CreateShaderResourceView(device, resource.Get(), cpuHandle, isCubeMap);
}

ID3D12Resource* Texture::GetResource()
{
	return resource.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetCPUHandle()
{
	return cpuHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE Texture::GetGPUHandle()
{
	return gpuHandle;
}
