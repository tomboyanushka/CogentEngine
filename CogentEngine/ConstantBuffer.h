#pragma once
#include <DirectXMath.h>
constexpr int MaxLights = 1;

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
	XMFLOAT3 padding;
};

struct SphereAreaLight
{
	XMFLOAT4 Color;
	XMFLOAT3 LightPos;
	float Radius;
	float Intensity;
	float AboveHorizon;
	XMFLOAT2 padding;
};

struct DiscAreaLight
{
	XMFLOAT4 Color;
	XMFLOAT3 LightPos;
	float Radius;
	XMFLOAT3 PlaneNormal;
	float Intensity;
};

struct RectAreaLight
{
	XMFLOAT4 Color;
	XMFLOAT3 LightPos;
	float Intensity;
	XMFLOAT3 LightLeft;
	float LightWidth;
	XMFLOAT3 LightUp;
	float LighHeight;
	XMFLOAT3 PlaneNormal;
	float padding;
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

// ----------Default & PBR PS------------
struct PixelShaderExternalData
{
	DirectionalLight dirLight;
	PointLight pointLight[MaxLights];
	SphereAreaLight sphereLight[MaxLights];
	DiscAreaLight discLight[MaxLights];
	RectAreaLight rectLight[MaxLights];
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