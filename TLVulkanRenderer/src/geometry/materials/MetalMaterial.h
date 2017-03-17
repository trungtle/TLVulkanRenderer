#pragma once
#include "Material.h"

class MetalMaterial : public Material
{
public:
	MetalMaterial() : Material() {};
	MetalMaterial(MaterialPacked packed, Texture* texture) :
		Material(packed, texture)
	{}

	ColorRGB EvaluateEnergy(
		const Intersection& isx,
		const Direction& lightDirection,
		const Direction& in,
		Direction& out
	) override;
};

