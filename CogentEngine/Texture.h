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
	uint32 CreateTexture(ID3D12Device* device, const std::string& fileName, ID3D12CommandQueue* commandQueue, const DescriptorHeap* textureHeap, TextureType type = WIC);

	uint32 CreateTextureFromResource(ID3D12Device* device, ID3D12CommandQueue* commandQueue, ID3D12Resource* resource, DescriptorHeap* textureHeap, uint32 textureWidth, uint32 textureHeight, bool isDepthTexture);

	ID3D12Resource* GetResource();
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle();
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle();
	void SetGPUHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle);

	std::string GetName();

private:
	static uint32_t textureIndexTracker;
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	D3D12_CPU_DESCRIPTOR_HANDLE textureCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE textureGPUHandle;

	uint32_t textureIndex = -1;
	std::string fileName;
};