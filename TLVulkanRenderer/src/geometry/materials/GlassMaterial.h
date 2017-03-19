#pragma once
#include "Material.h"

class GlassMaterial : public Material
{
public:
	GlassMaterial() : Material() {};
	GlassMaterial(MaterialPacked packed, Texture* texture) :
		Material(packed, texture)
	{}

	ColorRGB EvaluateEnergy(
		const Intersection& isx,
		const Direction& lightDirection,
		const Ray& in,
		Ray& out,
		bool& shouldTerminate
	) override;
};

