#include "Entity.h"



Entity::Entity(Mesh * mesh, char* address, D3D12_GPU_DESCRIPTOR_HANDLE hp, uint32_t constantBufferIndex, Material* material, ConstantBufferView cbv)
{
	this->mesh = mesh;
	this->material = material;
	this->constantBufferIndex = constantBufferIndex;
	this->SetPosition(XMFLOAT3(0.0, 0.0, 0.0));
	this->SetScale(XMFLOAT3(1.0, 1.0, 1.0));
	this->gpuAddress = address;
	this->handle = hp;
	this->boundingOrientedBox = mesh->GetBoundingBox();
	this->cbv = cbv;
}

Entity::~Entity()
{
}

XMFLOAT3 Entity::GetPosition()
{
	return position;
}

void Entity::SetPosition(XMFLOAT3 setPos)
{
	position = setPos;
}

void Entity::SetScale(XMFLOAT3 setScale)
{
	scale = setScale;
	this->boundingOrientedBox = GetBoundingOrientedBox();
	boundingOrientedBox.Extents.x *= setScale.x;
	boundingOrientedBox.Extents.y *= setScale.y;
	boundingOrientedBox.Extents.z *= setScale.z;
}

void Entity::SetRotation(XMFLOAT3 setRot)
{
	rotation = setRot;
}

void Entity::Move(float x, float y, float z)
{
	position.x += x;
	position.y += y;
	position.z += z;
}

void Entity::Rotate(float x, float y, float z)
{
	rotation.x += x;
	rotation.y += y;
	rotation.z += z;
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
	XMVECTOR vPosition = XMLoadFloat3(&position);
	XMVECTOR vRotation = XMLoadFloat3(&rotation);
	XMVECTOR vScale = XMLoadFloat3(&scale);

	//coverting to matrices
	XMMATRIX mPosition = XMMatrixTranslationFromVector(vPosition);
	XMMATRIX mRotation = XMMatrixRotationRollPitchYawFromVector(vRotation);
	XMMATRIX mScale = XMMatrixScalingFromVector(vScale);


	XMMATRIX wake = XMMatrixIdentity();

	//calculte world matrix
	XMMATRIX world = mScale * mRotation * mPosition;

	//storing the world matrix

	XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(world));

	return worldMatrix;
}

void Entity::UpdateWorldMatrix()
{
	XMMATRIX trans = XMMatrixTranslation(position.x, position.y, position.z);
	XMMATRIX rot = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	//XMMATRIX rotX = XMMatrixRotationX(rotation.x);
	//XMMATRIX rotY = XMMatrixRotationY(rotation.y);
	//XMMATRIX rotZ = XMMatrixRotationZ(rotation.z);
	XMMATRIX sc = XMMatrixScaling(scale.x, scale.y, scale.z);

	XMMATRIX total = sc * rot * trans;
	XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(total));
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
	box.Center = position;
	box.Extents.x *= scale.x;
	box.Extents.y *= scale.y;
	box.Extents.z *= scale.z;
	XMVECTOR quaternion = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&rotation));
	XMFLOAT4 rotationQuaternion;
	XMStoreFloat4(&rotationQuaternion, quaternion);
	box.Orientation = rotationQuaternion;
	return box;
}

