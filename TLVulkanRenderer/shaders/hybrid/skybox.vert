#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 view;
} ubo;

layout(location = 0) in vec3 iPos;
layout(location = 1) in vec3 iNor;
layout(location = 2) in vec2 iUV;
layout(location = 0) out vec3 oUVW;

void main()
{
	oUVW = iPos;
	oUVW.y = -oUVW.y;
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(iPos, 1.0);
}
