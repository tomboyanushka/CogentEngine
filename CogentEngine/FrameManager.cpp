#include "FrameManager.h"
void FrameManager::Initialize(ID3D12Device* device)
{
	this->device = device;
	uint32_t bufferSize = sizeof(PixelShaderExternalData);
	bufferSize = (bufferSize + 255);
	bufferSize = bufferSize & ~255;
	gpuConstantBuffer.Create(device, c_MaxConstBufferSize, bufferSize);
	gpuHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
}

ConstantBufferView FrameManager::CreateConstantBufferView(uint32_t bufferSize)
{
	bufferSize = (bufferSize + 255);
	bufferSize = bufferSize & ~255;
	ConstantBufferView cbv;
	cbv.cbOffset = cbOffset;
	cbv.heapIndex = frameHeapCounter;
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = gpuConstantBuffer.GetAddressWithIndex(cbOffset);
	cbOffset += (bufferSize / 256);
	cbvDesc.SizeInBytes = bufferSize;
	device->CreateConstantBufferView(&cbvDesc, gpuHeap.handleCPU(frameHeapCounter)); //hCPUHeapStart = handleCPU(0)
	frameHeapCounter++;
	constantBufferIndex++;

	return cbv;
}

Material FrameManager::CreateMaterial(
	const wchar_t* diffuseTextureFileName,
	const wchar_t* normalTextureFileName,
	ID3D12CommandQueue* commandQueue)
{
	Material material;
	material.Create(device,
		diffuseTextureFileName,
		normalTextureFileName,
		commandQueue,
		frameHeapCounter,
		gpuHeap);
	frameHeapCounter += 2;
	return material;
}

Texture FrameManager::CreateTexture(const wchar_t* textureFileName, ID3D12CommandQueue* commandQueue, TextureType type)
{
	Texture texture;
	texture.Create(device,
		textureFileName,
		commandQueue,
		frameHeapCounter,
		gpuHeap,
		type);
	frameHeapCounter++;
		return texture;
}

Entity* FrameManager::CreateEntity(Mesh* mesh, Material* material) // ConstantBufferView cbv
{
	ConstantBufferView cbv = CreateConstantBufferView(sizeof(VertexShaderExternalData));
	Entity* entity = new Entity(mesh, gpuConstantBuffer.GetMappedAddressWithIndex(cbv.cbOffset), gpuHeap.handleGPU(cbv.heapIndex), constantBufferIndex, material, cbv);
	return entity;
}

void FrameManager::CopyData(void* data, size_t size, ConstantBufferView cbv)
{
	gpuConstantBuffer.CopyDataWithIndex(&data, sizeof(VertexShaderExternalData), cbv.cbOffset);
}

D3D12_GPU_DESCRIPTOR_HANDLE FrameManager::GetGPUHandle(uint32_t index)
{
	return gpuHeap.handleGPU(index);
}

DescriptorHeap& FrameManager::GetGPUDescriptorHeap()
{
	return gpuHeap;
}
