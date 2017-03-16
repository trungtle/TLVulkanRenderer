#pragma once

#include <glm/glm.hpp>
#include <geometry/Geometry.h>
#include "geometry/BBox.h"
#include "Color.h"

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

	UV GetUV(const vec3&) const override {
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

	inline Point3 GetPosition() {
		return m_position;
	}

	inline ColorRGB GetColor() {
		return m_color;
	}

protected:
	Point3 m_position;
	ColorRGB m_color;

};