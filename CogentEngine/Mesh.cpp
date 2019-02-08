#include "Mesh.h"





Mesh::Mesh(const char * file, ID3D12Device * device)
{
}

Mesh::~Mesh()
{
}

ID3D12Resource * Mesh::GetVertexBuffer()
{
	return vertexBuffer;
}

ID3D12Resource * Mesh::GetIndexBuffer()
{
	return indexBuffer;
}

int Mesh::GetIndexCount()
{
	return indexCount;
}

void Mesh::CreateBasicGeometry(Vertex * vertices, UINT vertexCount, UINT * indices, UINT indexCount, ID3D12Device * device)
{
	// Create some temporary variables to represent colors
	// - Not necessary, just makes things more readable
	XMFLOAT4 red = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4 green = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	XMFLOAT4 blue = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

	// Set up the vertices of the triangle we would like to draw
	// - We're going to copy this array, exactly as it exists in memory
	//    over to a DirectX-controlled data structure (the vertex buffer)
	Vertex v1[] =
	{
	{ XMFLOAT3(+0.0f, +1.0f, +0.0f), red },
	{ XMFLOAT3(+1.5f, -1.0f, +0.0f), blue },
	{ XMFLOAT3(-1.5f, -1.0f, +0.0f), green },
	};

	// Set up the indices, which tell us which vertices to use and in which order
	// - This is somewhat redundant for just 3 vertices (it's a simple example)
	// - Indices are technically not required if the vertices are in the buffer 
	//    in the correct order and each one will be used exactly once
	// - But just to see how it's done...
	int i1[] = { 0, 1, 2 };

	// Create geometry buffers  ------------------------------------
	//CreateVertexBuffer(sizeof(Vertex), 3, vertices, &vertexBuffer, &vbView);
	//CreateIndexBuffer(DXGI_FORMAT_R32_UINT, 3, indices, &indexBuffer, &ibView);
}
