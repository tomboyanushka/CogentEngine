#pragma once

#include <wrl.h>
#include <assert.h>
#include <d3d12.h>
#include "d3dx12.h"
//#include "ResourceManager.h"
#include <string>

#define SafeRelease(comObj) if(comObj) comObj->Release();

constexpr uint64_t AlignmentSize = 256;

std::wstring ToWString(const std::string& str);

class GPUConstantBuffer
{
public:
	void Create(ID3D12Device* device,uint32_t bufferSize)
	{
		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

		device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			0,
			IID_PPV_ARGS(&resource));
		void* gpuAddress;
		resource->Map(0, 0, &gpuAddress);

		address = reinterpret_cast<char*>(gpuAddress);
	}

	void CopyData(void* data, uint32_t size, uint32_t offset = 0)
	{
		memcpy(address + offset, data, size);
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetAddress(uint64_t offset = 0)
	{
		return resource->GetGPUVirtualAddress() + offset;
	}

	char* GetMappedAddress(uint64_t offset = 0)
	{
		return address + offset;
	}


private:
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	char* address;
	
};

class DescriptorHeap
{
public:
	DescriptorHeap() { memset(this, 0, sizeof(*this)); }

	HRESULT Create(
		ID3D12Device* pDevice,
		D3D12_DESCRIPTOR_HEAP_TYPE Type,
		UINT NumDescriptors,
		bool bShaderVisible = false)
	{
		HeapDesc.Type = Type;
		HeapDesc.NumDescriptors = NumDescriptors;
		HeapDesc.Flags = (bShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : (D3D12_DESCRIPTOR_HEAP_FLAGS)0);

		HRESULT hr = pDevice->CreateDescriptorHeap(&HeapDesc,
			__uuidof(ID3D12DescriptorHeap),
			(void**)& pDescriptorHeap);
		if (FAILED(hr)) return hr;
		pDescriptorHeap->SetName(ToWString(std::to_string(NumDescriptors) + " Heap").c_str());
		hCPUHeapStart = pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		if (bShaderVisible)
		{
			hGPUHeapStart = pDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		}
		else
		{
			hGPUHeapStart.ptr = 0;
		}
		HandleIncrementSize = pDevice->GetDescriptorHandleIncrementSize(HeapDesc.Type);
		return hr;
	}
	operator ID3D12DescriptorHeap* () { return pDescriptorHeap.Get(); }

	uint64_t MakeOffsetted(uint64_t ptr, UINT index) const
	{
		uint64_t offsetted;
		offsetted = ptr + static_cast<uint64_t>(index * HandleIncrementSize);
		return offsetted;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU(UINT index) const
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle;
		handle.ptr = MakeOffsetted(hCPUHeapStart.ptr, index);
		return handle;
	}
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU(UINT index) const
	{
		assert(HeapDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		D3D12_GPU_DESCRIPTOR_HANDLE handle;
		handle.ptr = MakeOffsetted(hGPUHeapStart.ptr, index);
		return handle;
	}
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE hCPUHeapStart;
	D3D12_GPU_DESCRIPTOR_HANDLE hGPUHeapStart;
	UINT HandleIncrementSize;
};