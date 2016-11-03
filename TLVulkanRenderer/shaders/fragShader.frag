#version 450
#extension GL_ARB_separate_shader_objects: enable

layout(location = 0) in vec3 fragNormal;
layout(location = 2) in vec3 lightDirection;

layout(location = 0) out vec4 outColor;

void main() {

	// Lambertian term
	float lambertian = clamp(dot(fragNormal, lightDirection), 0, 1);

	outColor = vec4(abs(fragNormal), 1.0); 
	//outColor = vec4(1, 0, 0, 1);
}