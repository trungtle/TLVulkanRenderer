#include "LambertMaterial.h"
#include <geometry/Geometry.h>

glm::vec3 LambertMaterial::EvaluateEnergy(
	const Intersection& isx, 
	const glm::vec3& lightDirection, 
	const glm::vec3& in, 
	glm::vec3& out)
{
	vec3 color;

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
	float diffuse = glm::dot(lightDirection, isx.hitNormal) * 0.5 + 0.5;
	//float specular = (m_shininess + 8) / (8 * PI)
	color = glm::clamp(
		diffuse, 0.0f, 1.0f) * isx.hitTextureColor + m_colorAmbient;

	// Out direction is some random on the hemisphere

	return color;
}
