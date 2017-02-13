#pragma once
#include <glm/glm.hpp>

class Ray
{
public:
	glm::vec3 m_origin;
	glm::vec3 m_direction;
	Ray() : m_origin(glm::vec3(0)), m_direction(glm::vec3(0)) {}

	Ray(glm::vec3 origin, glm::vec3 direction) :
		m_origin(origin), m_direction(direction)
	{}

	Ray GetTransformedCopy(glm::mat4 transform) const {

		glm::vec4 origin = transform * glm::vec4(m_origin, 1);
		glm::vec4 direction = transform * glm::vec4(m_direction, 0);

		return Ray(origin, direction);
	}

	glm::vec3 GetPointOnRay(float t) const {
		return m_origin + m_direction * t;
	}
};


