#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include "DXUtility.h"

class Texture
{
public:
	void Create(ID3D12Device* device, const wchar_t* fileName, ID3D12CommandQueue* commandQueue, uint32_t index, const DescriptorHeap &heap);
	ID3D12Resource* GetResource();
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle();
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle();

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
};

