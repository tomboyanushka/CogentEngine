#include "FrameManager.h"

//To Do: Make it simpler by sending index in and calling a for loop only once

void FrameManager::Initialize(ID3D12Device* device)
{
	this->device = device;
	uint32_t bufferSize = sizeof(PixelShaderExternalData);
	bufferSize = (bufferSize + 255);
	bufferSize = bufferSize & ~255;
	for (int i = 0; i < FRAME_BUFFER_COUNT; ++i)
	{
		gpuConstantBuffer[i].Create(device, MAX_CBUFFER_SIZE, bufferSize);
	}
	gpuHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, HEAPSIZE, true);
	materialHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, HEAPSIZE, false);
	textureHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, HEAPSIZE, false);

}

ConstantBufferView FrameManager::CreateConstantBufferView(uint32_t bufferSize)
{
	bufferSize = (bufferSize + 255);
	bufferSize = bufferSize & ~255;
	ConstantBufferView cbv;
	cbv.cbOffset = cbOffset;
	cbv.heapIndex = frameHeapCounter;
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	for (int i = 0; i < FRAME_BUFFER_COUNT; ++i)
	{
		cbvDesc.BufferLocation = gpuConstantBuffer[i].GetAddress(cbOffset);
		cbvDesc.SizeInBytes = bufferSize;

	}
	device->CreateConstantBufferView(&cbvDesc, gpuHeap.handleCPU(frameHeapCounter));
	cbOffset += bufferSize; //(bufferSize / 256);
	frameHeapCounter++;
	constantBufferIndex++;

	return cbv;
}

Material FrameManager::CreateMaterial(
	const std::string& diffuseTextureFileName,
	const std::string& normalTextureFileName,
	const std::string& metalTextureFileName,
	const std::string& roughnessTextureFileName,
	ID3D12CommandQueue* commandQueue,
	TextureType type)
{
	Material material;
	std::string materialName = diffuseTextureFileName + normalTextureFileName + metalTextureFileName + roughnessTextureFileName;

	if (materialMap.find(materialName) != materialMap.end())
	{
		material = materialMap[materialName];
	}
	else
	{
		material.Create(device,
			diffuseTextureFileName,
			normalTextureFileName,
			metalTextureFileName,
			roughnessTextureFileName,
			commandQueue,
			&materialHeap,
			&textureHeap,
			this,
			FRAME_BUFFER_COUNT,
			type);

		device->CopyDescriptorsSimple(4, gpuHeap.handleCPU(frameHeapCounter), material.GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_GPU_DESCRIPTOR_HANDLE handle = gpuHeap.handleGPU(frameHeapCounter);
		material.SetGPUHandle(handle);
		frameHeapCounter += 4;
		materialMap.insert({ materialName, material });
	}

	return material;
}

Texture FrameManager::CreateTexture(const std::string& textureFileName, ID3D12CommandQueue* commandQueue, TextureType type)
{
	Texture texture;

	if (textureMap.find(textureFileName) != textureMap.end())
	{
		texture = textureMap[textureFileName];
	}
	else
	{
		texture.CreateTexture(device, textureFileName, commandQueue, &textureHeap, type);

		device->CopyDescriptorsSimple(1, gpuHeap.handleCPU(frameHeapCounter), texture.GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_GPU_DESCRIPTOR_HANDLE handle = gpuHeap.handleGPU(frameHeapCounter);
		texture.SetGPUHandle(handle);
		frameHeapCounter++;
		textureMap.insert({ textureFileName, texture });
	}

	return texture;
}

ID3D12Resource* FrameManager::CreateResource(ID3D12CommandQueue* commandQueue, D3D12_RESOURCE_FLAGS flags, LPCWSTR resourceName, DXGI_FORMAT format)
{
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	auto desc = CD3DX12_RESOURCE_DESC::Tex2D(format, SCREEN_WIDTH, SCREEN_HEIGHT, 1, 0, 1, 0, flags);
	auto clearVal = CD3DX12_CLEAR_VALUE(format, 1.f, 0);
	{
		device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearVal,
			IID_PPV_ARGS(resource.GetAddressOf()));
	}

	resource->SetName(resourceName);
	resources.push_back(resource);
	return resource.Get();
}

ID3D12Resource* FrameManager::CreateUAVResource(ID3D12CommandQueue* commandQueue, D3D12_RESOURCE_FLAGS flags, LPCWSTR resourceName, DXGI_FORMAT format)
{
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	auto desc = CD3DX12_RESOURCE_DESC::Tex2D(format, SCREEN_WIDTH, SCREEN_HEIGHT, 1, 0, 1, 0, flags);
	auto clearVal = CD3DX12_CLEAR_VALUE(format, 1.f, 0);

	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&resource));

	resource->SetName(resourceName);
	resources.push_back(resource);
	return resource.Get();
}

Texture FrameManager::CreateTextureFromResource(ID3D12CommandQueue* commandQueue, ID3D12Resource* resource, bool isDepthTexture, DXGI_FORMAT format)
{
	Texture texture;
	texture.CreateTextureFromResource(device, commandQueue, resource, &textureHeap, SCREEN_WIDTH, SCREEN_HEIGHT, isDepthTexture, format);
	device->CopyDescriptorsSimple(1, gpuHeap.handleCPU(frameHeapCounter), texture.GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_GPU_DESCRIPTOR_HANDLE handle = gpuHeap.handleGPU(frameHeapCounter);
	texture.SetGPUHandle(handle);
	frameHeapCounter++;

	return texture;
}

Texture FrameManager::CreateTextureFromUAVResource(ID3D12CommandQueue* commandQueue, ID3D12Resource* resource, DXGI_FORMAT format)
{
	Texture texture;
	texture.CreateTextureFromUAVResource(device, commandQueue, resource, &textureHeap, SCREEN_WIDTH, SCREEN_HEIGHT, format);
	device->CopyDescriptorsSimple(1, gpuHeap.handleCPU(frameHeapCounter), texture.GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_GPU_DESCRIPTOR_HANDLE handle = gpuHeap.handleGPU(frameHeapCounter);
	texture.SetGPUHandle(handle);
	frameHeapCounter++;

	return texture;
}

D3D12_GPU_DESCRIPTOR_HANDLE FrameManager::Allocate(D3D12_CPU_DESCRIPTOR_HANDLE* handles, int num)
{
	auto currentFrameHeapCounter = frameHeapCounter;
	
	for (int i = 0; i < num; ++i)
	{
		device->CopyDescriptorsSimple(1, gpuHeap.handleCPU(frameHeapCounter), handles[i], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		frameHeapCounter++;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE handle = gpuHeap.handleGPU(currentFrameHeapCounter);
	return handle;
}

void FrameManager::ResetFrameCounter()
{
	if (frameHeapCounter > 2048)
	{
		frameHeapCounter = baseFreameHeapCounter;
	}
}

Entity* FrameManager::CreateEntity(Mesh* mesh, Material* material)
{
	ConstantBufferView cbv = CreateConstantBufferView(sizeof(VertexShaderExternalData));
	Entity* entity;
	entity = new Entity(mesh, gpuConstantBuffer, gpuHeap, constantBufferIndex, material, cbv);

	return entity;
}

Entity* FrameManager::CreateTransparentEntity(Mesh* mesh, Material* material)
{
	ConstantBufferView cbv = CreateConstantBufferView(sizeof(VertexShaderExternalData));
	Entity* entity;
	entity = new Entity(mesh, gpuConstantBuffer, gpuHeap, constantBufferIndex, material, cbv);

	return entity;
}

void FrameManager::CopyData(void* data, uint32_t size, ConstantBufferView cbv, uint32_t backBufferIndex)
{
	gpuConstantBuffer[backBufferIndex].CopyData(data, size, cbv.cbOffset);
}

D3D12_GPU_DESCRIPTOR_HANDLE FrameManager::GetGPUHandle(uint32_t index, uint32_t backBufferIndex)
{
	return gpuHeap.handleGPU(index);
}

DescriptorHeap& FrameManager::GetGPUDescriptorHeap(uint32_t backBufferIndex)
{
	return gpuHeap;
}

DescriptorHeap FrameManager::GetGPUDescriptorHeap()
{
	return gpuHeap;
}
