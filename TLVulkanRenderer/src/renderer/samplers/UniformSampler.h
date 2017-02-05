#pragma once

#include "Sampler.h"

class UniformSampler : public Sampler {
public:


	UniformSampler() : Sampler(), m_uniformSamples(X4) {
	}

	UniformSampler(ESamples numSamples) : m_uniformSamples(numSamples) {	
	}

	std::vector<glm::vec2> Get2DSamples(const glm::vec2& point) override {

		std::vector<glm::vec2> samples;
		switch(m_uniformSamples) {
			case X4:
				// Return 4 corners and also the origin point
				samples.push_back({ point.x + 0.25f, point.y + 0.25f });
				samples.push_back({ point.x + 0.25f, point.y + 0.75f });
				samples.push_back({ point.x + 0.5f, point.y + 0.5f });
				samples.push_back({ point.x + 0.75f, point.y + 0.25f });
				samples.push_back({ point.x + 0.75f, point.y + 0.75f });
				break;
			case X8:
				for (float x = 0.25f; x <= 0.75f; x += .25f) {
					for (float y = 0.25f; y <= 0.75f; y += .25f)
					{
						samples.push_back({ point.x + x, point.y + y });
					}
				}
				break;
			case X16:
				samples.push_back({ point.x + 0.5f, point.y + 0.5f });
				for (float x = 0.2f; x <= 0.8f; x += .2f)
				{
					for (float y = 0.2f; y <= 0.8f; y += .2f)
					{
						samples.push_back({ point.x + x, point.y + y });
					}
				}
				break;
			case X1:
			default:
				samples.push_back(point);
				break;
		}

		return samples;
	}

protected:
	ESamples m_uniformSamples;
};
