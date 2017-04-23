#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 view;
} ubo;


layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in float inMaterialID;


layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outTangent;
layout (location = 4) out float outMaterialID;


out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
	
	vec4 tmpPos = vec4(inPos, 1.0);

	// -- out UV
	outUV = inUV;

	// Vertex position in world space
	outWorldPos = vec3(ubo.model * tmpPos);
	// GL to Vulkan coord space
	outWorldPos.y = -outWorldPos.y;
	gl_Position = ubo.projection * ubo.view * vec4(outWorldPos, 1.0);

	// Normal in world space
	mat3 mNormal = transpose(inverse(mat3(ubo.model)));
	outNormal = inNormal;
	
	// Tangent in world space
	outTangent = mNormal * normalize(inTangent);
	
	outMaterialID = inMaterialID;
}
