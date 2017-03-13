#include "MetalMaterial.h"
#include <geometry/Geometry.h>

glm::vec3 MetalMaterial::EvaluateEnergy(
	const Intersection& isx, 
	const glm::vec3& lightDirection, 
	const glm::vec3& in, 
	glm::vec3& out)
{
	vec3 color;

	out = glm::reflect(in, isx.hitNormal);
	if (glm::dot(out, isx.hitNormal) >= 0) {
		color *= m_colorReflective;
	}

	return color;
}
