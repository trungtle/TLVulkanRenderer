#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 1) uniform samplerCube sEnvmap;

layout(location = 0) in vec3 iUVW;
layout (location = 0) out vec4 oFragcolor;

void main()
{
	oFragcolor = texture(sEnvmap, iUVW);
}
