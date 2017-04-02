// Based on Sascha Willems example: https://github.com/SaschaWillems/Vulkan/tree/master/data/shaders/raytracing

#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 0) uniform sampler2D samplerColor;
layout (binding = 1) uniform Buffer {
	ivec2 iResolution;
};
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

void main() 
{
	vec2 q = gl_FragCoord.xy / iResolution.xy;
	vec2 p = -1.0+2.0*q;
	p.x *= -iResolution.x / iResolution.y;

	// SSAO
	// https://mynameismjp.wordpress.com/2009/03/10/reconstructing-position-from-depth/

	vec4 color = texture(samplerColor, inUV);
	outFragColor = color;
}