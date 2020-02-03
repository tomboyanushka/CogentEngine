#pragma once
#include <DirectXMath.h>
#define MAX_POINT_LIGHTS 3

using namespace DirectX;

//----------LIGHTS------------
struct DirectionalLight
{
	XMFLOAT4 AmbientColor;
	XMFLOAT4 DiffuseColor;
	XMFLOAT3 Direction;
	float Intensity;
};

struct PointLight
{
	XMFLOAT4 Color;
	XMFLOAT3 Position;
	float Range;
	float Intensity;
	float padding[3];
};

struct VertexShaderExternalData
{
	XMFLOAT4X4 world;
	XMFLOAT4X4 view;
	XMFLOAT4X4 proj;
};
struct SkyboxExternalData
{
	XMFLOAT4X4 view;
	XMFLOAT4X4 proj;
};

struct PixelShaderExternalData
{
	DirectionalLight dirLight;
	PointLight pointLight[MAX_POINT_LIGHTS];
	XMFLOAT3 cameraPosition;
	int pointLightCount;
};

struct TransparencyExternalData
{
	DirectionalLight dirLight;
	XMFLOAT3 cameraPosition;
	float blendAmount;
};

