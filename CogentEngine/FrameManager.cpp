#include "FrameManager.h"

//To Do: Make it simpler by sending index in and calling a for loop only once

void FrameManager::Initialize(ID3D12Device* device)
{
	this->device = device;
	uint32_t bufferSize = sizeof(PixelShaderExternalData);
	bufferSize = (bufferSize + 255);
	bufferSize = bufferSize & ~255;
	for (int i = 0; i < c_FrameBufferCount; ++i)
	{
		gpuConstantBuffer[i].Create(device, c_MaxConstBufferSize, bufferSize);
		gpuHeap[i].Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, c_HeapSize, true);
	}

	materialHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, c_HeapSize, false);
	textureHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, c_HeapSize, false);

}

ConstantBufferView FrameManager::CreateConstantBufferView(uint32_t bufferSize)
{
	bufferSize = (bufferSize + 255);
	bufferSize = bufferSize & ~255;
	ConstantBufferView cbv;
	cbv.cbOffset = cbOffset;
	cbv.heapIndex = frameHeapCounter;
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	for (int i = 0; i < c_FrameBufferCount; ++i)
	{
		cbvDesc.BufferLocation = gpuConstantBuffer[i].GetAddress(cbOffset);
		cbvDesc.SizeInBytes = bufferSize;
		device->CreateConstantBufferView(&cbvDesc, gpuHeap[i].handleCPU(frameHeapCounter));
	}

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
			c_FrameBufferCount,
			type);

		//for (int i = 0; i < c_FrameBufferCount; ++i)
		{
			device->CopyDescriptorsSimple(4, gpuHeap[0].handleCPU(frameHeapCounter), material.GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		D3D12_GPU_DESCRIPTOR_HANDLE handles[c_FrameBufferCount] = { gpuHeap[0].handleGPU(frameHeapCounter), gpuHeap[1].handleGPU(frameHeapCounter), gpuHeap[2].handleGPU(frameHeapCounter) };
		material.SetGPUHandle(handles);
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
		//for (int i = 0; i < c_FrameBufferCount; ++i)
		{
			device->CopyDescriptorsSimple(1, gpuHeap[0].handleCPU(frameHeapCounter), texture.GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		D3D12_GPU_DESCRIPTOR_HANDLE handles[c_FrameBufferCount] = { gpuHeap[0].handleGPU(frameHeapCounter), gpuHeap[1].handleGPU(frameHeapCounter), gpuHeap[2].handleGPU(frameHeapCounter) };
		texture.SetGPUHandle(handles);
		frameHeapCounter++;
		textureMap.insert({ textureFileName, texture });
	}

	return texture;
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
	return gpuHeap[backBufferIndex].handleGPU(index);
}

DescriptorHeap& FrameManager::GetGPUDescriptorHeap(uint32_t backBufferIndex)
{
	return gpuHeap[backBufferIndex];
}

DescriptorHeap* FrameManager::GetGPUDescriptorHeap()
{
	return gpuHeap;
}
