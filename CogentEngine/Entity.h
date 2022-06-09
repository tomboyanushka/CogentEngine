#pragma once

#include "Constants.h"
#include <DirectXMath.h>
#include "Mesh.h"
#include "Material.h"
#include "ConstantBufferView.h"
using namespace DirectX;
class Entity
{
public:
	Entity(Mesh* mesh, GPUConstantBuffer* gpuConstantBuffer, const DescriptorHeap gpuHeap, uint32_t constantBufferIndex, Material* material, ConstantBufferView cbv);
	~Entity();

	XMFLOAT3 GetPosition(); 
	XMFLOAT3 GetRotation(); 
	XMFLOAT3 GetScale();

	void SetPosition(XMFLOAT3 position);
	void SetScale(XMFLOAT3 scale);
	void SetScale(float scale);
	void SetRotation(XMFLOAT3 rotation);

	void Move(float x, float y, float z);
	void Rotate(float x, float y, float z);

	Mesh* GetMesh();
	void SetMesh(Mesh* mesh);

	Material* GetMaterial();
	void SetMaterial(Material* material);

	XMFLOAT4X4 GetWorldMatrix();
	void UpdateWorldMatrix();
	char* GetAddress();
	uint32_t GetConstantBufferIndex();
	ConstantBufferView GetConstantBufferView();


	D3D12_GPU_DESCRIPTOR_HANDLE GetHandle();
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandle();
	void SetSRVHandle(D3D12_GPU_DESCRIPTOR_HANDLE srvHandle);
	BoundingOrientedBox& GetBoundingOrientedBox();

private:
	XMFLOAT4X4 m_worldMatrix;
	XMFLOAT3 m_position;
	XMFLOAT3 m_rotation;
	XMFLOAT3 m_scale;
	Mesh* mesh;
	uint32_t constantBufferIndex;
	Material* material;
	char* gpuAddress;
	D3D12_GPU_DESCRIPTOR_HANDLE handle;
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle;
	UINT64 handlePtr;
	BoundingOrientedBox boundingOrientedBox;
	BoundingOrientedBox box;
	ConstantBufferView cbv;

};

