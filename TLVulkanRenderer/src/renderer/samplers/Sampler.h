#pragma once

#include <glm/glm.hpp>
#include <vector>


enum ESamples
{
	X1,
	X4,
	X8,
	X16
};

class Sampler {
	
public:
	Sampler() {};

	virtual std::vector<glm::vec2> Get2DSamples(const glm::vec2& point) = 0;

protected:
	unsigned int m_samplesPerPoint;
};
