#include "MetalMaterial.h"
#include <geometry/Geometry.h>

ColorRGB MetalMaterial::EvaluateEnergy(
	const Intersection& isx, 
	const Direction& lightDirection,
	const Direction& in,
	Direction& out)
{
	ColorRGB color;

	out = glm::reflect(in, isx.hitNormal);
	if (glm::dot(out, isx.hitNormal) >= 0) {
		color *= m_colorReflective;
	}

	return color;
}
