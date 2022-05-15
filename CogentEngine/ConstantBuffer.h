#pragma once
#include <DirectXMath.h>
constexpr int MaxPointLights = 1;

using namespace DirectX;

// ----------LIGHTS------------
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

// ----------SKY------------
struct SkyboxExternalData
{
	XMFLOAT4X4 view;
	XMFLOAT4X4 proj;
};

// ----------Default PS------------
struct PixelShaderExternalData
{
	DirectionalLight dirLight;
	PointLight pointLight[MaxPointLights];
	XMFLOAT3 cameraPosition;
	int pointLightCount;
};

// ----------TRANSPARENCY------------
struct TransparencyExternalData
{
	DirectionalLight dirLight;
	XMFLOAT3 cameraPosition;
	float blendAmount;
};

//----------Post Processing------------

struct QuadExternalData
{
	XMFLOAT4 position;
	XMFLOAT2 uv;
};

struct BlurExternalData
{
	float pixelWidth;
	float pixelHeight;
	float blurAmount;
	float focusPlaneZ;
	float zFar;
	float zNear;
	float pad0;
	float pad1;
};

struct RefractionExternalData
{
	XMFLOAT4X4 view;
	XMFLOAT4X4 proj;
	XMFLOAT4X4 projInv;
	XMFLOAT3 cameraPosition;
	bool doubleBounce;
	float zNear;
	float zFar;
	XMFLOAT2 padding;
};