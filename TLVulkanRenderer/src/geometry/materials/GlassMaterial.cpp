#include "GlassMaterial.h"
#include <geometry/Geometry.h>

ColorRGB GlassMaterial::EvaluateEnergy(
	const Intersection& isx,
	const Direction& lightDirection,
	const Direction& in,
	Direction& out)
{
	vec3 color;

	// === Refraction === //
	// heckbert method
	// t = lerp(0.225m 0.465, cos theta)
	// Rv = lerp(i, -n, t)
	float ei = 1.0;
	float et = m_refracti;
	float cosi = clamp(dot(in, isx.hitNormal), -1.0f, 1.0f);
	bool entering = cosi < 0;
	if (!entering)
	{
		float t = ei;
		ei = et;
		et = t;
	}
	float eta = ei / et;
	out = normalize(refract(in, isx.hitNormal, eta));

	return color;
}
