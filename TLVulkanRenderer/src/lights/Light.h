#pragma once

#include <glm/glm.hpp>
#include <geometry/Geometry.h>
#include "geometry/BBox.h"

using namespace glm;

class Light : public Geometry {
public:
	Light() :
		m_position(vec3()), m_color(vec3(1,1,1))
	{		
	}

	Light(vec3 pos, vec3 color) :
		m_position(pos), m_color(color)
	{	
	}

	Intersection GetIntersection(const Ray& ray) override {
		return Intersection();
	};

	vec2 GetUV(const vec3&) const override {
		return vec2();
	}

	BBox GetBBox() override {
		return BBox();
	}

	/**
	 * \brief Return how much light has attenuated once reached point
	 * \param point 
	 * \return 
	 */
	virtual float Attenuation(const vec3& point) {
		return 1;
	}

	vec3 m_position;
	vec3 m_color;

};