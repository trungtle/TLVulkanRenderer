#pragma once
#include "Light.h"

class PointLight : public Light {
public:

	PointLight() : Light() {};
	PointLight(vec3 pos, vec3 color, float radius) :
		Light(pos, color), m_radius(radius)
	{
	}

	float Attenuation(const vec3& point) override { 
		float dist = glm::distance(m_position, point);
		return m_radius / glm::max(0.1f, glm::pow(dist, 2.0f)) + 0.1;
	}

protected:

	float m_radius;
};
