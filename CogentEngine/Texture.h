#pragma once

#include "Constants.h"
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include "DXUtility.h"
#include "DDSTextureLoader.h"

enum TextureType
{
	WIC, DDS
};

class Texture
{
public:
	uint32 CreateTexture(ID3D12Device* device, const std::string& fileName, ID3D12CommandQueue* commandQueue, uint32_t index, const DescriptorHeap* heap, TextureType type = WIC);
	ID3D12Resource* GetResource();
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(const DescriptorHeap* heap, uint32_t backBufferIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(const DescriptorHeap* heap, uint32_t backBufferIndex);

	std::string GetName();

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;

	uint32_t textureIndex = -1;
	std::string fileName;
};