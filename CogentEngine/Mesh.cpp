#include "Mesh.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>



Mesh::Mesh(const char * objFile, ID3D12Device * device, ID3D12GraphicsCommandList* commandList)
{
	// Variables used while reading the file
	std::vector<XMFLOAT3> positions;     // Positions from the file
	std::vector<XMFLOAT3> normals;       // Normals from the file
	std::vector<XMFLOAT3> tangents;
	std::vector<XMFLOAT2> uvs;           // UVs from the file
	std::vector<Vertex> verts;           // Verts we're assembling
	std::vector<UINT> indices;           // Indices of these verts
	unsigned int vertCounter = 0;        // Count of vertices/indices
	//char chars[100];                     // String for line reading


	std::vector<Vertex> vertices;           // Verts we're assembling
	std::vector<UINT> indexVals;           // Indices of these verts
	int numFaces = 0;
	std::string warnings;
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, &warnings, objFile);


	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];
			numFaces += fv;

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
				tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
				tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
				tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];
				Vertex vertex;
				vertex.Position = XMFLOAT3(vx, vy, vz);
				positions.push_back(vertex.Position);
				vertex.Normal = XMFLOAT3(nx, ny, nz);
				normals.push_back(vertex.Normal);
				vertex.UV = XMFLOAT2(tx, ty);
				uvs.push_back(vertex.UV);
				vertices.push_back(vertex);

				indexVals.push_back((UINT)index_offset + (UINT)v);
				// Optional: vertex colors
				// tinyobj::real_t red = attrib.colors[3*idx.vertex_index+0];
				// tinyobj::real_t green = attrib.colors[3*idx.vertex_index+1];
				// tinyobj::real_t blue = attrib.colors[3*idx.vertex_index+2];
			}
			index_offset += fv;

			// per-face material
			shapes[s].mesh.material_ids[f];
			
		}
	}
	/*tangents.resize(positions.size());
	DirectX::ComputeTangentFrame(indexVals.data(), numFaces, positions.data(), normals.data(), uvs.data(), positions.size(), tangents.data(), nullptr);

	for (int i = 0; i < vertices.size(); ++i)
	{
		vertices[i].Tangent = tangents[i];
	}
*/

	vertexCount = uint32_t(vertices.size());
	indexCount = uint32_t(indexVals.size());

	CreateBasicGeometry(vertices.data(), vertexCount, indexVals.data(), indexCount, device, commandList);
	this->vertices = vertices;
	this->indices = indexVals;
}

Mesh::Mesh(Vertex* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t indexCount, ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	CreateBasicGeometry(vertices, vertexCount, indices, indexCount, device, commandList);
	this->vertexCount = vertexCount;
	this->indexCount = indexCount;
}

void CalculateTangents(Vertex* vertices, UINT vertexCount, UINT* indices, UINT indexCount)
{
	XMFLOAT3* tan1 = new XMFLOAT3[vertexCount * 2];
	XMFLOAT3* tan2 = tan1 + vertexCount;
	ZeroMemory(tan1, vertexCount * sizeof(XMFLOAT3) * 2);
	int triangleCount = indexCount / 3;
	for (UINT i = 0; i < indexCount; i += 3)
	{
		int i1 = indices[i];
		int i2 = indices[i + 2];
		int i3 = indices[i + 1];
		auto v1 = vertices[i1].Position;
		auto v2 = vertices[i2].Position;
		auto v3 = vertices[i3].Position;

		auto w1 = vertices[i1].UV;
		auto w2 = vertices[i2].UV;
		auto w3 = vertices[i3].UV;

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;

		float s1 = w2.x - w1.x;
		float s2 = w3.x - w1.x;
		float t1 = w2.y - w1.y;
		float t2 = w3.y - w1.y;
		float r = 1.0F / (s1 * t2 - s2 * t1);

		XMFLOAT3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r,
			(t2 * z1 - t1 * z2) * r);

		XMFLOAT3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r,
			(s1 * z2 - s2 * z1) * r);

		XMStoreFloat3(&tan1[i1], XMLoadFloat3(&tan1[i1]) + XMLoadFloat3(&sdir));
		XMStoreFloat3(&tan1[i2], XMLoadFloat3(&tan1[i2]) + XMLoadFloat3(&sdir));
		XMStoreFloat3(&tan1[i3], XMLoadFloat3(&tan1[i3]) + XMLoadFloat3(&sdir));

		XMStoreFloat3(&tan2[i1], XMLoadFloat3(&tan2[i1]) + XMLoadFloat3(&tdir));
		XMStoreFloat3(&tan2[i2], XMLoadFloat3(&tan2[i2]) + XMLoadFloat3(&tdir));
		XMStoreFloat3(&tan2[i3], XMLoadFloat3(&tan2[i3]) + XMLoadFloat3(&tdir));
	}

	for (UINT a = 0; a < vertexCount; a++)
	{
		auto n = vertices[a].Normal;
		auto t = tan1[a];

		// Gram-Schmidt orthogonalize
		auto dot = XMVector3Dot(XMLoadFloat3(&n), XMLoadFloat3(&t));
		XMStoreFloat3(&vertices[a].Tangent, XMVector3Normalize(XMLoadFloat3(&t) - XMLoadFloat3(&n) * dot));

		// Calculate handedness
		/*tangent[a].w = (Dot(Cross(n, t), tan2[a]) < 0.0F) ? -1.0F : 1.0F;*/
	}

	delete[] tan1;
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

BoundingOrientedBox & Mesh::GetBoundingBox()
{
	return boundingBox;
}

std::vector<Vertex> Mesh::GetVertices()
{
	return vertices;
}

std::vector<UINT> Mesh::GetIndices()
{
	return indices;
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

void Mesh::CreateBasicGeometry(Vertex* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t indexCount, ID3D12Device * device, ID3D12GraphicsCommandList* commandList)
{
	CalculateTangents(vertices, vertexCount, indices, indexCount);

	//// Create geometry buffers  ------------------------------------
	CreateVertexBuffer(sizeof(Vertex), vertexCount, vertices, &vertexBuffer, &vbView, device, commandList);
	CreateIndexBuffer(DXGI_FORMAT_R32_UINT, indexCount, indices, &indexBuffer, &ibView, device, commandList);

	XMFLOAT3 vMinf3(FLT_MAX, FLT_MAX, FLT_MAX);
	XMFLOAT3 vMaxf3(FLT_MIN, FLT_MIN, FLT_MIN);

	XMVECTOR vMin = XMLoadFloat3(&vMinf3);
	XMVECTOR vMax = XMLoadFloat3(&vMaxf3);
	for (UINT i = 0; i < vertexCount; ++i)
	{
		XMVECTOR P = XMLoadFloat3(&vertices[i].Position);
		vMin = XMVectorMin(vMin, P);
		vMax = XMVectorMax(vMax, P);
	}
	
	XMStoreFloat3(&boundingBox.Center, 0.5f * (vMin + vMax));
	XMStoreFloat3(&boundingBox.Extents, 0.5f * (vMax - vMin));
}
