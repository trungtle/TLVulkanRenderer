#pragma once
#include "pcg/pcg_basic.h"

const float EPSILON = 0.0001;
const float PI = 3.14159265358979323846f;
const float TWO_PI = 2 * PI;
const float DEG2RAD = PI / 180.f;
const float RAD2DEG = 180.f / PI;

namespace TLMath {
	inline double randDouble(pcg32_random_t& rng) {
		return std::ldexp(pcg32_random_r(&rng), -32);
	}

	inline float lerp(float v0, float v1, float t) {
		return v0 + t * (v1 - v0);
	}
}
