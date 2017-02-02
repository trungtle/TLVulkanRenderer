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
		return m_radius - glm::distance(m_position, point);
	}

protected:

	float m_radius;
};
