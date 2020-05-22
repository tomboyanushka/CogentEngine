#pragma once
#include "d3dx12.h"
#include "Texture.h"

class Material
{


public:
	uint32_t Create(ID3D12Device* device, 
		const wchar_t* diffuseTextureFileName, 
		const wchar_t* normalTextureFileName, 
		const wchar_t* metalTextureFileName, 
		const wchar_t* roughTextureFileName, 
		ID3D12CommandQueue* commandQueue, 
		uint32_t index, 
		const DescriptorHeap* heap,
		uint32_t heapCount);

	D3D12_GPU_DESCRIPTOR_HANDLE GetFirstGPUHandle(const DescriptorHeap* heap, uint32_t backBufferIndex);

private:
	Texture diffuseTexture;
	Texture normalTexture;
	Texture metalTexture;
	Texture roughTexture;

	uint32_t materialIndex;
};

