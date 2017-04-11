#pragma once
#include "Light.h"

class DirectionalLight : public Light
{
public:

	DirectionalLight() : Light() {};
	DirectionalLight(vec3 pos, vec3 color, Direction direction) :
		Light(pos, color), m_direction(direction)
	{
	}

protected:

	Direction m_direction;
};
