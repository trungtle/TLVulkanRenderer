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

void main() {
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
	fragNormal = inNormal;
}
