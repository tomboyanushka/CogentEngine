#include "Lighting.hlsli"

cbuffer externalData : register(b0)
{
	DirectionalLight dirLight;
	PointLight pointLight;
	float3 cameraPosition;
	float padding;
}