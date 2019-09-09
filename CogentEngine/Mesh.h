#pragma once
#include "DXCore.h"
#include <d3d12.h>
#include <DirectXMath.h>
#include "Vertex.h"
#include <vector>
#include <DirectXCollision.h>
#include "DirectXMesh.h"

using namespace DirectX;
class Mesh
{
public:

	Mesh(const char* file, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	Mesh(Vertex* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t indexCount,
		ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	~Mesh();

	ID3D12Resource * GetVertexBuffer();
	ID3D12Resource * GetIndexBuffer();
	int GetIndexCount();
	D3D12_VERTEX_BUFFER_VIEW &GetVertexBufferView();
	D3D12_INDEX_BUFFER_VIEW &GetIndexBufferView();
	BoundingOrientedBox& GetBoundingBox();
	std::vector<Vertex> GetVertices();
	std::vector<UINT> GetIndices();


	HRESULT CreateStaticBuffer(
		unsigned int dataStride, 
		unsigned int dataCount, 
		void * data, 
		ID3D12Resource ** buffer, 
		ID3D12Device* device, 
		ID3D12GraphicsCommandList* commandList, 
		ID3D12Resource** uploadHeap);

	HRESULT CreateVertexBuffer(unsigned int dataStride, unsigned int dataCount, void * data, ID3D12Resource ** buffer, 
		D3D12_VERTEX_BUFFER_VIEW * vbView, ID3D12Device * device, ID3D12GraphicsCommandList* commandList);

	HRESULT CreateIndexBuffer(DXGI_FORMAT format, unsigned int dataCount, void * data, ID3D12Resource ** buffer, 
		D3D12_INDEX_BUFFER_VIEW * ibView, ID3D12Device * device, ID3D12GraphicsCommandList* commandList);

	void CreateBasicGeometry(Vertex* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t indexCount, 
		ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

private:
	ID3D12Resource * vertexBuffer;
	ID3D12Resource * indexBuffer;
	ID3D12Resource* vbUploadHeap;
	ID3D12Resource* ibUploadHeap;

	D3D12_VERTEX_BUFFER_VIEW vbView;
	D3D12_INDEX_BUFFER_VIEW ibView;
	uint32_t indexCount;
	uint32_t vertexCount;

	std::vector<Vertex> vertices;           // Verts we're assembling
	std::vector<UINT> indices;           // Indices of these verts

	BoundingOrientedBox boundingBox;


};

