#include "Lighting.hlsli"
#define MAX_POINT_LIGHTS 1

cbuffer externalData : register(b0)
{
	DirectionalLight dirLight;
	PointLight pointLight[MAX_POINT_LIGHTS];
	float3 cameraPosition;
	int pointLightCount;
}