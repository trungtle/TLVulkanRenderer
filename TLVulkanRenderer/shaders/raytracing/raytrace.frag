#version 450
#extension GL_ARB_separate_shader_objects: enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 0) uniform sampler2D s_sampler;

layout(location = 0) in vec2 f_uv;

layout(location = 0) out vec4 o_color;

void main() {
	o_color = texture(s_sampler, vec2(f_uv));
	//o_color = vec4(1,0,0,1);
}