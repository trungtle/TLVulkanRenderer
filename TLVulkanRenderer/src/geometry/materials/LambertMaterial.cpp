#include "LambertMaterial.h"
#include <geometry/Geometry.h>

ColorRGB LambertMaterial::EvaluateEnergy(
	const Intersection& isx, 
	const Direction& lightDirection,
	const Direction& in,
	Direction& out)
{
	ColorRGB color;
	
	// Use half lambert here to pop up the color
	float diffuse = glm::dot(lightDirection, isx.hitNormal) * 0.5 + 0.5;
	//float specular = (m_shininess + 8) / (8 * PI)
	color = glm::clamp(
		diffuse, 0.0f, 1.0f) * isx.hitTextureColor + m_colorAmbient;

	// Out direction is some random on the hemisphere

	return color;
}
