#include "LambertMaterial.h"
#include <geometry/Geometry.h>

glm::vec3 LambertMaterial::EvaluateEnergy(const Intersection& isx, const glm::vec3& in, glm::vec3& out) 
{

	vec3 color;
	// === Reflection === //
	if (m_reflectivity > 0)
	{
		out = normalize(reflect(out, isx.hitNormal));
	}

	// === Refraction === //
	if (false)
	{
		float ei = 1.0;
		float et = 1.5;
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
	}

	// Use half lambert here to pop up the color
	float lambert = glm::dot(in, isx.hitNormal) * 0.5 + 0.5;
	color = glm::clamp(
		lambert, 0.0f, 1.0f) * m_colorDiffuse * isx.hitTextureColor + m_colorAmbient;

	if (m_reflectivity > 0) {
		color *= m_colorReflective;
	}

	return color;
}
