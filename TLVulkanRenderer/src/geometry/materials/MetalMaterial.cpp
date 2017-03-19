#include "MetalMaterial.h"
#include <geometry/Geometry.h>

ColorRGB MetalMaterial::EvaluateEnergy(
	const Intersection& isx, 
	const Direction& lightDirection,
	const Ray& in,
	Ray& out,
	bool& shouldTerminate
	)
{
	ColorRGB color;

	out.m_direction = glm::reflect(in.m_direction, isx.hitNormal);
	if (glm::dot(out.m_direction, isx.hitNormal) >= 0) {
		color *= m_colorReflective;
		out.m_origin = isx.hitPoint + EPSILON * out.m_direction;
	}

	return color;
}
