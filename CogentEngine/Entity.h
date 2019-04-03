#pragma once
#include <DirectXMath.h>
#include "Mesh.h"
using namespace DirectX;
class Entity
{
public:
	Entity(Mesh* mesh);
	~Entity();

	//VertShaderExternalData* data;

	void SetPosition(XMFLOAT3 position);
	void SetScale(XMFLOAT3 scale);
	void SetRotation(XMFLOAT3 rotation);

	void Move(float x, float y, float z);
	void Rotate(float x, float y, float z);

	Mesh* GetMesh();
	XMFLOAT4X4 GetWorldMatrix();
	void UpdateWorldMatrix();

private:
	XMFLOAT4X4 worldMatrix;
	XMFLOAT3 position;
	XMFLOAT3 rotation;
	XMFLOAT3 scale;
	Mesh* mesh;
};

