#pragma once
#include <DirectXMath.h>
#include "Light.h"
using namespace DirectX;

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
	PointLight pointLight;
	XMFLOAT3 cameraPosition;
	float padding;
};

struct TransparencyExternalData
{
	DirectionalLight dirLight;
	XMFLOAT3 cameraPosition;
	float blendAmount;
};