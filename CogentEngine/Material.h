#pragma once
#include "d3dx12.h"
#include "Texture.h"

class Material
{


public:
	uint32_t Create(ID3D12Device* device, 
		const wchar_t* diffuseTextureFileName, 
		const wchar_t* normalTextureFileName, 
		ID3D12CommandQueue* commandQueue, 
		uint32_t index, 
		const DescriptorHeap& heap);

	D3D12_GPU_DESCRIPTOR_HANDLE GetFirstGPUHandle();

private:
	Texture diffuseTexture;
	Texture normalTexture;

};

