#pragma once
#include "DirectXMath.h"
#include <string>
using namespace DirectX;

class Camera
{
public:
	Camera(float x, float y, float z);
	~Camera();

	void MoveRelative(float x, float y, float z);
	void MoveAbsolute(float x, float y, float z);
	void Rotate(float x, float y);

	XMFLOAT3 GetPosition();
	std::string GetPositionString();
	XMFLOAT4X4 GetViewMatrixTransposed();
	XMFLOAT4X4 GetProjectionMatrixTransposed();
	XMFLOAT4X4 GetProjectionMatrixInverseTransposed();

	void Update(float dt);
	void UpdateViewMatrix();
	void UpdateProjectionMatrix(float aspectRatio);

	inline constexpr float GetNearZ() { return zNear; }
	inline constexpr float GetFarZ() { return zFar; }

private:

	XMFLOAT4X4 viewMatrix;
	XMFLOAT4X4 projMatrix;


	//For the look to matrix
	XMFLOAT3 startPosition;
	XMFLOAT3 position;
	XMFLOAT4 direction;

	float roll; //x rotation
	float pitch; //y rotation

	float zNear;
	float zFar;
};


