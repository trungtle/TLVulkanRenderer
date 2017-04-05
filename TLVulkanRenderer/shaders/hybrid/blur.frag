#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 0) uniform sampler2D uSSAO;

layout (location = 0) in vec2 inUV;

layout (location = 0) out float outFragColor; 

void main() 
{
	vec2 texelSize = 1.0 / vec2(textureSize(uSSAO, 0));
	float result = 0.0;

	// Just blur 4x4 for our ssao
	for (int x = -2; x < 2; ++x) {
		for (int y = -2; y < 2; ++y) {
			vec2 offset = vec2(float(x), float(y)) * texelSize;
			result += texture(uSSAO, inUV + offset).r;
		}
	}

	outFragColor = result / (4.0 * 4.0); 
}