#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D samplerColor;
layout (binding = 2) uniform sampler2D samplerNormalMap;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inWorldPos;
layout (location = 4) in vec3 inTangent;
layout (location = 5) in float inMaterialIdNormalized;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;

void main() 
{
	outPosition = vec4(inWorldPos, inMaterialIdNormalized);

	// Calculate normal in tangent space

	// Optional: read from normal map
	//vec3 N = normalize(inNormal);
	//N.y = -N.y;
	//vec3 T = normalize(inTangent);
	//vec3 B = cross(N, T);
	//mat3 TBN = mat3(T, B, N);
	//vec3 tnorm = TBN * normalize(texture(samplerNormalMap, inUV).xyz * 2.0 - vec3(1.0));
	//outNormal = vec4(tnorm, 1.0);
	outNormal = vec4(inNormal, 1.0);

	if (inColor == vec3(0.0, 0.0, 0.0)) {
		discard;
	}
	outAlbedo = vec4(inColor, 1.0);
}