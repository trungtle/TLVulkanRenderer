#version 450

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
layout(location = 3) out vec3 fragPosition;


void main() {
	vec4 position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
	vec4 positionW = ubo.model * vec4(inPosition, 1.0);
	
	// -- Out
	fragNormal = inNormal;
	fragPosition = vec3(positionW);
	lightDirection = normalize(vec3(10.0, 20.0, 20.0) - vec3(positionW));

	// -- Position
	gl_Position = position;
}
