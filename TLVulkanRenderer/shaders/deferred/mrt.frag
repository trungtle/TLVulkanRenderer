#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

struct Light {
	vec4 position;
	vec3 color;
	float radius;
};

layout (binding = 1) uniform sampler2D samplerColor;
//layout (binding = 2) uniform sampler2D samplerNormalMap;

layout (binding = 2) uniform LIGHT 
{
	Light lights[6];
	vec4 viewPos;
} lightsUBO;

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
//layout (location = 3) in flat int inMaterialID;
//layout (location = 3) in vec3 inColor;
//layout (location = 4) in vec3 inTangent;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;

void main() 
{
	outPosition = vec4(inWorldPos, 0);

	// Calculate normal in tangent space

	// Optional: read from normal map
	//vec3 N = normalize(inNormal);
	//N.y = -N.y;
	//vec3 T = normalize(inTangent);
	//vec3 B = cross(N, T);
	//mat3 TBN = mat3(T, B, N);
	//vec3 tnorm = TBN * normalize(texture(samplerNormalMap, inUV).xyz * 2.0 - vec3(1.0));
	//outNormal = vec4(tnorm, 1.0);
	outNormal = vec4(inNormal, 0.0);

	vec4 color = texture(samplerColor, inUV);

	for (int l = 0; i < 6; ++i) {
		if (lightsUBO[i].position == vec4(inWorldPos, 1)) {
			color = vec3(1, 1, 1);
		}
	}

	outAlbedo = color;
}