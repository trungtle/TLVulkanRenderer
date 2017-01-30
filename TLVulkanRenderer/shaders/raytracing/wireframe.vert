#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(std140, binding = 0) uniform UBO {
	mat4 mvp;
} ubo;

layout(location = 0) in vec3 a_position;

void main()
{
    vec4 position = ubo.mvp * vec4(a_position, 1);
	position.y = -position.y;
	gl_Position = position;
}
