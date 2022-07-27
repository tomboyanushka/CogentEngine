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

	textureIndex = textureIndexTracker;
	return textureIndex;
}
 

uint32 Texture::CreateTextureFromResource(ID3D12Device* device, ID3D12CommandQueue* commandQueue, ID3D12Resource* resourceIn, DescriptorHeap* textureHeap, uint32 textureWidth, uint32 textureHeight, bool isDepthTexture, DXGI_FORMAT format)
{
	textureIndexTracker++;
	auto srvFormat = format;
	if (isDepthTexture)
	{
		srvFormat = DXGI_FORMAT_R32_FLOAT;
	}

	// Describe and create a SRV for the texture.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = srvFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	this->resource = resourceIn;
	this->textureCPUHandle = textureHeap->handleCPU(textureIndexTracker);
	device->CreateShaderResourceView(resourceIn, &srvDesc, textureCPUHandle);
	textureIndex = textureIndexTracker;
	return textureIndexTracker;
}

uint32 Texture::CreateUAVTextureFromResource(ID3D12Device* device, ID3D12CommandQueue* commandQueue, ID3D12Resource* resourceIn, DescriptorHeap* textureHeap, uint32 textureWidth, uint32 textureHeight, DXGI_FORMAT format)
{
	textureIndexTracker++;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	this->resource = resourceIn;
	this->textureCPUHandle = textureHeap->handleCPU(textureIndexTracker);
	device->CreateUnorderedAccessView(resourceIn, nullptr, &uavDesc, textureCPUHandle);
	textureIndex = textureIndexTracker;
	return textureIndexTracker;
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
