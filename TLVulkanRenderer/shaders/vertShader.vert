#version 450
#extension GL_ARB_separate_objects : enable

out gl_PerVertex {
	vec4 gl_Position;
};

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 fragNormal;
layout(location = 2) out vec3 lightDirection;



void main() {
	vec4 position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
	
	// -- Out
	fragNormal = inNormal;
	lightDirection = normalize(vec3(-5.0, 2.0, 5.0) - vec3(position));

	// -- Position
	gl_Position = position;
}
