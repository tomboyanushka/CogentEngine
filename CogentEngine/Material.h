#pragma once
#include "d3dx12.h"


class Material
{
protected:

	D3D12_GPU_DESCRIPTOR_HANDLE albedoHandle;

public:
	Material(D3D12_GPU_DESCRIPTOR_HANDLE handle);
	~Material();

	void SetSRVHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle);
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandle();
};

