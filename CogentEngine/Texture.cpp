#include "Texture.h"
#include <WICTextureLoader.h>
#include "ResourceUploadBatch.h"
#include "DirectXHelpers.h"

using namespace DirectX;

uint32_t Texture::textureIndexTracker = 0;

uint32 Texture::CreateTexture(ID3D12Device* device, const std::string& fileName, ID3D12CommandQueue* commandQueue, const DescriptorHeap* textureHeap, TextureType type)
{
	Texture texture;
	textureIndexTracker++;
	bool isCubeMap = false;
	ResourceUploadBatch resourceUpload(device);
	resourceUpload.Begin();
	this->fileName = fileName;
	auto fName = ToWString(fileName);
	switch (type)
	{
	case WIC:
		CreateWICTextureFromFile(device, resourceUpload, fName.c_str(), resource.GetAddressOf(), true);
		break;

	case DDS:
		CreateDDSTextureFromFile(device, resourceUpload, fName.c_str(), resource.GetAddressOf(), false, 0, nullptr, &isCubeMap);
		break;

	}
	
	auto uploadResourcesFinished = resourceUpload.End(commandQueue);
	uploadResourcesFinished.wait();

	this->textureCPUHandle = textureHeap->handleCPU(textureIndexTracker);
	CreateShaderResourceView(device, resource.Get(), textureCPUHandle, isCubeMap);

	return textureIndex;
}
 
ID3D12Resource* Texture::GetResource()
{
	return resource.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetCPUHandle()
{
	return textureCPUHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE Texture::GetGPUHandle()
{
	return textureGPUHandle;
}

void Texture::SetGPUHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle)
{
	textureGPUHandle = handle;
}

std::string Texture::GetName()
{
	return fileName;
}
