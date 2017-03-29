#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(std140, binding = 0) uniform UBO {
	mat4 mvp;
} ubo;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_color;

layout(location = 0) out vec3 v_color;

void main()
{
	vec4 position = vec4(a_position, 1);
	position.y = -position.y;
    position = ubo.mvp * position;
	
	v_color = a_color;
	gl_Position = position;
}
