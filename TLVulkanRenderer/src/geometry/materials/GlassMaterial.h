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
		const Direction& in,
		Direction& out
	) override;
};

