
cbuffer externalData : register(b0)
{
    float4x4 view;
    float3 cameraPosition;
    float pad0;
};


// Defines the input to this pixel shader
// - Should match the output of our corresponding vertex shader
struct VertexToPixel
{
    float4 position                 : SV_POSITION;
    float2 uv                       : TEXCOORD;
    float3 normal                   : NORMAL;
    float3 tangent                  : TANGENT;
    float3 worldPos                 : POSITION;
    noperspective float2 screenUV   : TEXCOORD1;
    float depth                     : DEPTH;
};

// TEXTURE STUFF
Texture2D ScenePixels           : register(t0);
Texture2D NormalMap             : register(t4);
Texture2D CustomDepthTexture    : register(t7);
SamplerState BasicSampler       : register(s0);
SamplerState RefractSampler     : register(s1);


// Entry point for this pixel shader
float4 main(VertexToPixel input) : SV_TARGET
{
	// Fix for poor normals: re-normalizing interpolated normals
    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);

	// Sample and unpack normal
    float3 normalFromTexture = NormalMap.Sample(BasicSampler, input.uv).xyz * 2 - 1;
    
    float backFaceDepth = CustomDepthTexture.Sample(BasicSampler, input.uv).r;
    float frontFaceDepth = input.depth;

	// Create the TBN matrix which allows us to go from TANGENT space to WORLD space
    float3 N = input.normal;
    float3 T = normalize(input.tangent - N * dot(input.tangent, N));
    float3 B = cross(T, N);
    float3x3 TBN = float3x3(T, B, N);

	// Overwrite the existing normal (we've been using for lighting),
	// with the version from the normal map, AFTER we convert to world space
    input.normal = normalize(mul(normalFromTexture, TBN));

	// Vars for controlling refraction - Adjust as you see fit
    float indexOfRefr = 0.9f; // Ideally keep this below 1 - not physically accurate, but prevents "total internal reflection"
    float refrAdjust = 0.1f; // Makes our refraction less extreme, since we're using UV coords not world units
	
	// Calculate the refraction amount in WORLD SPACE
    float3 ogRay = normalize(input.worldPos - cameraPosition);
    float3 refRay = refract(ogRay, input.normal, indexOfRefr);
    
    // Finding the angles to apply Snell's law to get the transmitted vector
    float angleOfIncidence = acos(dot(input.normal, ogRay)) / (length(input.normal) * length(ogRay));
    angleOfIncidence = degrees(angleOfIncidence);
    
    float angleOfTransmission = acos(dot(-input.normal, ogRay)) / (length(input.normal) * length(ogRay));
    angleOfTransmission = degrees(angleOfTransmission);
    
    // nI * sin(angle(i)) = nT * sin(angle(t))
    // where ni and nt are the indices of refraction for the incident and transmission media
    float indexOfTransmission = (indexOfRefr * sin(angleOfIncidence)) / sin(angleOfTransmission);
    
    // Calculate the distance between point of refraction and point it will hit on the backface
    // by getting a difference in depth from the two depth textures
    // dv = distance between the front and back face, read from z buffer texture
    // dn = distance along normal
    // d = weight distance
    float dn = length(input.normal);
    float dv = abs(backFaceDepth - frontFaceDepth);
    float angle = (angleOfTransmission / angleOfIncidence);
    float depthBetween = (angle * dv) + ((1 - angle) * dn);

    float3 p2 = input.worldPos + (depthBetween * refRay);
    float2 p2UV = mul(float4(p2, 0.0f), view).xy;

	// Get the refraction XY direction in VIEW SPACE (relative to the camera)
	// We use this as a UV offset when sampling the texture
    float2 refrUV = mul(float4(refRay, 0.0f), view).xy * refrAdjust;
    refrUV.x *= -1.0f; // Flip the X to point away from the edge (Y already does this due to view space <-> texture space diff)


    float3 output = ScenePixels.Sample(RefractSampler, input.screenUV + p2UV).rgb;
	// Sample the pixels of the render target and return
    return float4(output, 1.f);
}