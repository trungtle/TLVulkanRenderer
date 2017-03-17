#include "GlassMaterial.h"
#include <geometry/Geometry.h>

bool Refract(const Direction& v, const Normal& n, float niOvernt, Direction& refracted) {
	vec3 uv = glm::normalize(v);
	float dt = dot(uv, n);
	float discriminant = 1.0 - niOvernt * niOvernt * (1 - dt * dt);
	if (discriminant > 0) {
		refracted = niOvernt * (uv - n * dt) - n * sqrt(discriminant);
		return true;
	}

	return false;
}

float Schlick(float cosine, float refracti) {
	float r0 = (1.0f - refracti) / (1.0f + refracti);
	r0 = r0 * r0;
	return r0 + (1.0f - r0) * pow((1.0f - cosine), 5.0f);
}

ColorRGB GlassMaterial::EvaluateEnergy(
	const Intersection& isx,
	const Direction& lightDirection,
	const Ray& in,
	Ray& out,
	bool& shouldTerminate
)
{
	ColorRGB color(1.0, 1.0, 1.0);
	Direction refracted;
	Direction reflected = glm::reflect(in.m_direction, isx.hitNormal);
	float reflectProb;

	// === Refraction === //
	// heckbert method
	// t = lerp(0.225m 0.465, cos theta)
	// Rv = lerp(i, -n, t)
	float ei = 1.0;
	float et = m_refracti;
	
	float cosi = clamp(dot(in.m_direction, isx.hitNormal), -1.0f, 1.0f);
	float cost;
	bool entering = cosi < 0;
	Direction normal = isx.hitNormal;

	if (!entering)
	{
		float t = ei;
		ei = et;
		et = t;
		normal = -normal;
		cost = m_refracti * dot(in.m_direction, isx.hitNormal) / glm::length(in.m_direction);
	} else {
		cost = -dot(in.m_direction, isx.hitNormal) / glm::length(in.m_direction);
	}
	float eta = ei / et;
	if (Refract(in.m_direction, normal, eta, refracted)) {
		reflectProb = Schlick(cost, m_refracti);
	} else {
		reflectProb = 1.0f;
		out.m_direction = reflected;
	}

	// Fresnel
	static float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
	if (r < reflectProb) {
		out.m_direction = reflected;
	} else {
		out.m_direction = refracted;
	}

	// Offset origin slightly bit
	out.m_origin = isx.hitPoint;
	if (entering) {
		out.m_origin -= EPSILON * isx.hitNormal;
	} else {
		out.m_origin += EPSILON * isx.hitNormal;
	}

	// Need to handle total internal refraction
	return color;
}
