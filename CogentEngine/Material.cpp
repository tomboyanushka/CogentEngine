#include "Material.h"



Material::Material(D3D12_GPU_DESCRIPTOR_HANDLE handle)
{
	this->albedoHandle = handle;
}


Material::~Material()
{
}

void Material::SetSRVHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle)
{
}

D3D12_GPU_DESCRIPTOR_HANDLE Material::GetSRVHandle()
{
	return albedoHandle;
}
