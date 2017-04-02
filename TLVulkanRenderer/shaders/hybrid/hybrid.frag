#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 0) uniform sampler2D samplerColor;
layout (binding = 1) uniform Buffer {
	ivec2	iResolution;
	float	nearClip;
	float	farClip;

	vec2	noiseScale;
	float	radius;
	int		sampleKernelSize;

	mat4	projectionMat;
	mat4    viewMat;

	vec3	sampleKernel[16];
	vec3	viewRay;
} ubo;
layout (binding = 2) uniform sampler2D samplerDepth;
layout (binding = 3) uniform sampler2D uNormal;
layout (binding = 4) uniform sampler2D uNoiseTexture;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

vec4 hatchingColor(vec2 ndcPixel, vec4 color) {
	float hatching = (clamp((sin(ndcPixel.x * 170.0 +
							ndcPixel.y *110.0)), 0.0, 1.0));   
	hatching = max(hatching, (clamp((sin(ndcPixel.x * -170.0 +
							ndcPixel.y * 110.0)), 0.0, 1.0)));  
    
	if (color.y == 0.0) {
		color *= min(pow(length(ndcPixel), 4.0) * 0.125,1.0);
	}

	vec4 mCol = mix(color, color * 0.5, hatching);

	return mCol;
}

// Retriev the depth buffer valuye
float getLinearDepth() {
	// https://mynameismjp.wordpress.com/2009/03/10/reconstructing-position-from-depth/

	float z = texture(samplerDepth, inUV).r;
	float n = ubo.nearClip; // near plane
	float f = ubo.farClip; // far plane
	float c = (2.0 * n) / (f + n - z * (f - n)); // convert to linear

	return c;
}

float SSAO() {

	// SSAO
	// https://john-chapman-graphics.blogspot.com/2013/01/ssao-tutorial.html

	vec3 origin = ubo.viewRay * getLinearDepth();
	vec3 normal = vec3(ubo.viewMat * texture(uNormal, inUV));
	normal = normal * 2.0 - 1.0;
	normal = normalize(normal);

	// Compute tbn matrix by reorient the normal with the random noise
	// Use Gram-Schmidt process to get 3 normalized vectors
	// https://en.wikipedia.org/wiki/Gram%E2%80%93Schmidt_process
	vec3 rvec = texture(uNoiseTexture, inUV * ubo.noiseScale).xyz * 2.0 - 1.0;
	vec3 tangent = normalize(rvec - normal * dot(rvec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 tbn = mat3(tangent, bitangent, normal);

	// Compute occulusion
	float occlusion = 0.0;
	for (int i = 0; i < ubo.sampleKernelSize; ++i) {
		// get sample position
		vec3 samplePos = tbn * ubo.sampleKernel[i];
		samplePos = samplePos * ubo.radius + origin;

		// project sample pos
		vec4 offset = vec4(samplePos, 1.0);
		offset = ubo.projectionMat * offset;
		offset.xy /= offset.w;
		offset.xy = offset.xy * 0.5 + 0.5;

		// get sample depth
		float sampleDepth = texture(samplerDepth, offset.xy).r;
		float n = ubo.nearClip; // near plane
		float f = ubo.farClip; // far plane
		sampleDepth = (2.0 * n) / (f + n - sampleDepth * (f - n)); // convert to linear

		// range check & accumulate
		float rangeCheck = abs(origin.z - sampleDepth) < ubo.radius ? 1.0 : 0.0;
		occlusion += (sampleDepth <= samplePos.z ? 1.0 : 0.0) * rangeCheck;
	}

	occlusion = 1.0 - (occlusion / ubo.sampleKernelSize);

	return occlusion;
}

void main() 
{
	vec2 q = gl_FragCoord.xy / ubo.iResolution.xy;
	vec2 p = -1.0+2.0*q;
	p.x *= -ubo.iResolution.x / ubo.iResolution.y;

	float c = getLinearDepth();
	vec4 color = texture(samplerColor, inUV);
	//color = vec4(c, c, c, 1.0);
	//color = texture(uNoiseTexture, inUV);
	float ssao = SSAO();
	//color = vec4(ssao, ssao, ssao, 1.0);
	outFragColor = color * ssao;
}