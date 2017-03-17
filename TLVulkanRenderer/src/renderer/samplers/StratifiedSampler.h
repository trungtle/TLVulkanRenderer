#pragma once
#pragma once

#include "Sampler.h"
#include <map>

class StratifiedSampler : public Sampler
{
public:


	StratifiedSampler() : Sampler(), m_samples(X1) {
	}

	StratifiedSampler(ESamples numSamples, uint32_t screenWidth) : m_samples(numSamples), m_screenWidth(screenWidth) {
	}

	std::vector<glm::vec2> Get2DSamples(const glm::vec2& point) override {

		// Cache stratified offset
		uint32_t index = point.x + point.y * m_screenWidth;
		if (m_offsets.find(index) == m_offsets.end()) {
			float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			r *= 0.3;
			m_offsets[index] = r;
		}
		float offset = m_offsets[index];

		std::vector<glm::vec2> samples;
		switch (m_samples)
		{
			case X4:
				samples.push_back({ point.x + 0.25f, point.y + 0.25f });
				samples.push_back({ point.x + 0.25f, point.y + 0.75f });
				samples.push_back({ point.x + 0.75f, point.y + 0.25f });
				samples.push_back({ point.x + 0.75f, point.y + 0.75f });
				break;
			case X8:
				for (float x = 0.25f; x <= 0.75f; x += .25f)
				{
					for (float y = 0.25f; y <= 0.75f; y += .25f)
					{
						samples.push_back({ point.x + x, point.y + y });
					}
				}
				break;
			case X16:
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
				samples.push_back(glm::vec2(point.x + offset, point.y + offset));
				break;
		}

		return samples;
	}

protected:
	ESamples m_samples;
	std::map<unsigned int, float> m_offsets;
	uint32_t m_screenWidth;
};
