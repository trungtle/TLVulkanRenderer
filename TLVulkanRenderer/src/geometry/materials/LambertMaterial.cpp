#include "LambertMaterial.h"
#include <geometry/Geometry.h>

glm::vec3 LambertMaterial::EvaluateEnergy(const Intersection& isx, const glm::vec3& in, const glm::vec3& out) 
{
	return glm::clamp(
		glm::dot(in, isx.hitNormal), 0.0f, 1.0f) * m_colorDiffuse * isx.hitTextureColor;
}
