#pragma once
#include "DXUtility.h"
#include "ConstantBuffer.h"
#include "Material.h"
#include "Texture.h"
#include "Entity.h"
#include "Constants.h"
#include "ConstantBufferView.h"


class FrameManager
{
public:
	void Initialize(ID3D12Device* device);
	ConstantBufferView CreateConstantBufferView(uint32_t bufferSize);
	Material CreateMaterial(
		const wchar_t* diffuseTextureFileName,
		const wchar_t* normalTextureFileName,
		const wchar_t* metalTextureFileName,
		const wchar_t* roughTextureFileName,
		ID3D12CommandQueue* commandQueue);

	Texture CreateTexture(
		const wchar_t* textureFileName,
		ID3D12CommandQueue* commandQueue,
		TextureType type = WIC);

	Entity* CreateEntity(Mesh* mesh, Material* material);
	void CopyData(void* data, size_t size, ConstantBufferView cbv);

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t index);
	DescriptorHeap& GetGPUDescriptorHeap();
	

private:
	uint32_t frameHeapCounter = 0;
	uint32_t frameIndex;
	uint32_t constantBufferIndex = 0;
	uint32_t cbOffset = 0;


	GPUConstantBuffer gpuConstantBuffer;
	DescriptorHeap gpuHeap;
	ID3D12Device* device;
};

