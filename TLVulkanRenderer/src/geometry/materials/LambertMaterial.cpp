#include <random>
#include "LambertMaterial.h"
#include <geometry/Geometry.h>
#include <ctime>

Point3 RandomInUnitSphere() {
	srand((int)time(0));
	Point3 p;
	float len = glm::length(p);
	do {
		float r1 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		float r2 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		float r3 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		p = 2.0f * Point3(r1, r2, r3) - Point3(1, 1, 1);
		len = glm::length(p);
	} while (len * len >= 1.0);

	return p;
}

ColorRGB LambertMaterial::EvaluateEnergy(
	const Intersection& isx,
	const Direction& lightDirection,
	const Ray& in,
	Ray& out,
	bool& shouldTerminate
)
{
	ColorRGB color;
	
	// Use half lambert here to pop up the color
	float diffuse = glm::dot(lightDirection, isx.hitNormal) * 0.5 + 0.5;
	//float specular = (m_shininess + 8) / (8 * PI)
	color = glm::clamp(
		diffuse, 0.0f, 1.0f) * isx.hitTextureColor + m_colorAmbient;

	// Out direction is some random on the hemisphere
	Point3 target = isx.hitPoint + isx.hitNormal + RandomInUnitSphere();
	out.m_direction = glm::normalize(target - isx.hitPoint);
	out.m_origin = isx.hitPoint;

	shouldTerminate = true;

	return color;
}
