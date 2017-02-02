#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 v_color;

layout (location = 0) out vec4 outFragcolor;

void main()
{
    outFragcolor = vec4(v_color, 1);
}
