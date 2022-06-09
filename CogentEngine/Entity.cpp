#include "Entity.h"



Entity::Entity(Mesh * mesh, GPUConstantBuffer* gpuConstantBuffer, const DescriptorHeap gpuHeap, uint32_t constantBufferIndex, Material* material, ConstantBufferView cbv)
{
	this->mesh = mesh;
	this->material = material;
	this->constantBufferIndex = constantBufferIndex;
	this->SetPosition(XMFLOAT3(0.0, 0.0, 0.0));
	this->SetScale(XMFLOAT3(1.0, 1.0, 1.0));
	for (int i = 0; i < FRAME_BUFFER_COUNT; ++i)
	{
		this->cbv = cbv;
		this->gpuAddress = gpuConstantBuffer[i].GetMappedAddress(cbv.cbOffset);
		this->handle = gpuHeap.handleGPU(cbv.heapIndex);
	}

	//this->boundingOrientedBox = mesh->GetBoundingBox();

}

Entity::~Entity()
{
}

XMFLOAT3 Entity::GetPosition()
{
	return m_position;
}

XMFLOAT3 Entity::GetRotation()
{
	return m_rotation;
}

XMFLOAT3 Entity::GetScale()
{
	return m_scale;
}

void Entity::SetPosition(XMFLOAT3 setPos)
{
	m_position = setPos;
}

void Entity::SetScale(XMFLOAT3 setScale)
{
	m_scale = setScale;
	//this->boundingOrientedBox = GetBoundingOrientedBox();
	boundingOrientedBox.Extents.x *= setScale.x;
	boundingOrientedBox.Extents.y *= setScale.y;
	boundingOrientedBox.Extents.z *= setScale.z;
}

// Overload function if scale is uniform
void Entity::SetScale(float scale)
{
	m_scale = XMFLOAT3(scale, scale, scale);
}

void Entity::SetRotation(XMFLOAT3 setRot)
{
	m_rotation.x = setRot.x * XM_PI / 180;
	m_rotation.y = setRot.y * XM_PI / 180;
	m_rotation.z = setRot.z * XM_PI / 180;
}

void Entity::Move(float x, float y, float z)
{
	m_position.x += x;
	m_position.y += y;
	m_position.z += z;
}

void Entity::Rotate(float x, float y, float z)
{
	m_rotation.x += x;
	m_rotation.y += y;
	m_rotation.z += z;
}

Mesh * Entity::GetMesh()
{
	return mesh;
}

void Entity::SetMesh(Mesh * mesh)
{
	this->mesh = mesh;
	
}

XMFLOAT4X4 Entity::GetWorldMatrix()
{
	//coverting them to vectors
	XMVECTOR vPosition = XMLoadFloat3(&m_position);
	XMVECTOR vRotation = XMLoadFloat3(&m_rotation);
	XMVECTOR vScale = XMLoadFloat3(&m_scale);

	//coverting to matrices
	XMMATRIX mPosition = XMMatrixTranslationFromVector(vPosition);
	XMMATRIX mRotation = XMMatrixRotationRollPitchYawFromVector(vRotation);
	XMMATRIX mScale = XMMatrixScalingFromVector(vScale);


	XMMATRIX wake = XMMatrixIdentity();

	//calculte world matrix
	XMMATRIX world = mScale * mRotation * mPosition;

	//storing the world matrix

	XMStoreFloat4x4(&m_worldMatrix, XMMatrixTranspose(world));

	return m_worldMatrix;
}

void Entity::UpdateWorldMatrix()
{
	XMMATRIX trans = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
	XMMATRIX sc = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);

	XMMATRIX total = sc * rot * trans;
	XMStoreFloat4x4(&m_worldMatrix, XMMatrixTranspose(total));
}

char * Entity::GetAddress()
{
	return gpuAddress;
}

uint32_t Entity::GetConstantBufferIndex()
{
	return constantBufferIndex;
}

ConstantBufferView Entity::GetConstantBufferView()
{
	return cbv;
}

Material * Entity::GetMaterial()
{
	return material;
}

void Entity::SetMaterial(Material* material)
{
	this->material = material;
}

D3D12_GPU_DESCRIPTOR_HANDLE Entity::GetHandle()
{
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE Entity::GetSRVHandle()
{
	return srvHandle;
}

void Entity::SetSRVHandle(D3D12_GPU_DESCRIPTOR_HANDLE srvHandle)
{
	this->srvHandle = srvHandle;
}

BoundingOrientedBox & Entity::GetBoundingOrientedBox()
{
	box = mesh->GetBoundingBox();
	box.Center = m_position;
	box.Extents.x *= m_scale.x;
	box.Extents.y *= m_scale.y;
	box.Extents.z *= m_scale.z;
	XMVECTOR quaternion = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&m_rotation));
	XMFLOAT4 rotationQuaternion;
	XMStoreFloat4(&rotationQuaternion, quaternion);
	box.Orientation = rotationQuaternion;
	return box;
}

