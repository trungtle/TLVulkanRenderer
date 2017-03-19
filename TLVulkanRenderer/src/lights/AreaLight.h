#pragma once
#pragma once
#include "Light.h"

class AreaLight : public Light, public SquarePlane
{
public:

	AreaLight() : Light(), SquarePlane() {};
	AreaLight(vec3 pos, vec3 scale, vec3 normal, vec3 color, Material* material) :
		Light(pos, color), SquarePlane(pos, scale, normal, material)
	{
	}

	float Attenuation(const vec3& point) override {
		return 1.0f;
	}

};
