#include "Mesh.h"





Mesh::Mesh(const char * file, ID3D12Device * device)
{
}

Mesh::~Mesh()
{
	vertexBuffer->Release();
	indexBuffer->Release();
	vbUploadHeap->Release();
	ibUploadHeap->Release();
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

D3D12_VERTEX_BUFFER_VIEW &Mesh::GetVertexBufferView()
{
	return vbView;
}

D3D12_INDEX_BUFFER_VIEW &Mesh::GetIndexBufferView()
{
	return ibView;
}

HRESULT Mesh::CreateStaticBuffer(unsigned int dataStride, unsigned int dataCount, void * data, ID3D12Resource ** buffer, ID3D12Device* device, ID3D12GraphicsCommandList* commandList, ID3D12Resource** uploadHeap)
{
	// Potential result
	HRESULT hr = 0;

	D3D12_HEAP_PROPERTIES props = {};
	props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	props.CreationNodeMask = 1;
	props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	props.Type = D3D12_HEAP_TYPE_DEFAULT;
	props.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC desc = {};
	desc.Alignment = 0;
	desc.DepthOrArraySize = 1;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.Height = 1; // Width x Height, where width is size, height is 1
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Width = dataStride * dataCount; // Size of the buffer

	hr = device->CreateCommittedResource(
		&props,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COPY_DEST, // Will eventually be "common", but we're copying to it first!
		0,
		IID_PPV_ARGS(buffer));
	if (FAILED(hr))
		return hr;

	// Now create an intermediate upload heap for copying initial data
	D3D12_HEAP_PROPERTIES uploadProps = {};
	uploadProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadProps.CreationNodeMask = 1;
	uploadProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	uploadProps.Type = D3D12_HEAP_TYPE_UPLOAD; // Can only ever be Generic_Read state
	uploadProps.VisibleNodeMask = 1;

	hr = device->CreateCommittedResource(
		&uploadProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		0,
		IID_PPV_ARGS(&(*uploadHeap)));
	if (FAILED(hr))
		return hr;

	// Do a straight map/memcpy/unmap
	void* gpuAddress = 0;
	hr = (*uploadHeap)->Map(0, 0, &gpuAddress);
	if (FAILED(hr))
		return hr;
	memcpy(gpuAddress, data, dataStride * dataCount);
	(*uploadHeap)->Unmap(0, 0);


	// Copy the whole buffer from uploadheap to vert buffer
	//commandList->CopyBufferRegion(vertexBuffer, 0, vbUploadHeap, 0, sizeof(Vertex) * 3);
	commandList->CopyResource(*buffer, *uploadHeap);


	// Transition the vertex buffer to generic read
	D3D12_RESOURCE_BARRIER rb = {};
	rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	rb.Transition.pResource = *buffer;
	rb.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	rb.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList->ResourceBarrier(1, &rb);


	// Execute the command list
	//CloseExecuteAndResetCommandList();
	//uploadHeap->Release();

	return S_OK;
}

HRESULT Mesh::CreateVertexBuffer(unsigned int dataStride, unsigned int dataCount, void * data, ID3D12Resource ** buffer, D3D12_VERTEX_BUFFER_VIEW * vbView, ID3D12Device * device, ID3D12GraphicsCommandList* commandList)
{
	// Create the static buffer
	HRESULT hr = CreateStaticBuffer(dataStride, dataCount, data, buffer, device, commandList, &vbUploadHeap);
	if (FAILED(hr))
		return hr;

	// Set the resulting info in the view
	vbView->StrideInBytes = dataStride;
	vbView->SizeInBytes = dataStride * dataCount;
	vbView->BufferLocation = (*buffer)->GetGPUVirtualAddress();

	// Everything's good
	return S_OK;
}

HRESULT Mesh::CreateIndexBuffer(DXGI_FORMAT format, unsigned int dataCount, void * data, ID3D12Resource ** buffer, D3D12_INDEX_BUFFER_VIEW * ibView, ID3D12Device * device, ID3D12GraphicsCommandList* commandList)
{
	// Create the static buffer
	HRESULT hr = CreateStaticBuffer(sizeof(UINT), dataCount, data, buffer, device, commandList, &ibUploadHeap);
	if (FAILED(hr))
		return hr;

	// Set the resulting info in the view
	ibView->Format = format;
	ibView->SizeInBytes = sizeof(UINT) * dataCount;
	ibView->BufferLocation = (*buffer)->GetGPUVirtualAddress();

	// Everything's good
	return S_OK;
}

void Mesh::CreateBasicGeometry(ID3D12Device * device, ID3D12GraphicsCommandList* commandList)
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
	CreateVertexBuffer(sizeof(Vertex), 3, v1, &vertexBuffer, &vbView, device, commandList);
	CreateIndexBuffer(DXGI_FORMAT_R32_UINT, 3, i1, &indexBuffer, &ibView, device, commandList);
}
