#version 450
#extension GL_ARB_separate_shader_objects: enable

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexcoord;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4(abs(fragNormal), 1.0); 
	outColor = vec4(1.0f, 0.0f, 0.0f, 1.0);
}