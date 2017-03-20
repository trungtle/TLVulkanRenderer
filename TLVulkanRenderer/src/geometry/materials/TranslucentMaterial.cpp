#include "TranslucentMaterial.h"
#include <geometry/Geometry.h>

ColorRGB TranslucentMaterial::EvaluateEnergy(
	const Intersection& isx,
	const Direction& lightDirection,
	const Ray& in,
	Ray& out,
	bool& shouldTerminate
)
{
	ColorRGB color(0.1, 0.1, 0.1);
	color *= m_colorTransparent;

	// Offset origin slightly bit
	out.m_origin = isx.hitPoint + in.m_direction * EPSILON;
	out.m_direction = in.m_direction;

	// Need to handle total internal refraction
	return color;
}
