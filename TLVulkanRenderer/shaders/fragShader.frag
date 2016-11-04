#version 450
#extension GL_ARB_separate_shader_objects: enable

layout(location = 0) in vec3 fragNormal;
layout(location = 2) in vec3 lightDirection;
layout(location = 3) in vec3 fragPosition;

layout(location = 0) out vec4 outColor;

void main() {

	// Lambertian term
	float lambertian = clamp(dot(fragNormal, lightDirection), 0, 1);
	
	vec4 color = vec4(abs(fragNormal), 1.0); 
	color = vec4(0.85, 0.85, 0.4, 1.0);
	//color = vec4(fragPosition.z, fragPosition.z, fragPosition.z, 1.0);
	outColor = color * lambertian;
}