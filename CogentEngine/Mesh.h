#pragma once
#include "DXCore.h"
#include <d3d12.h>
#include <DirectXMath.h>
#include "Vertex.h"
#include <vector>

using namespace DirectX;
class Mesh
{
public:

	Mesh(const char* file, ID3D12Device* device);
	~Mesh();

	ID3D12Resource * GetVertexBuffer();
	ID3D12Resource * GetIndexBuffer();
	int GetIndexCount();

	void CreateBasicGeometry(Vertex* vertices, UINT vertexCount, UINT* indices, UINT indexCount, ID3D12Device* device);
private:
	ID3D12Resource * vertexBuffer;
	ID3D12Resource * indexBuffer;

	D3D12_VERTEX_BUFFER_VIEW vbView;
	D3D12_INDEX_BUFFER_VIEW ibView;
	int indexCount;

};

