#pragma once
#include "Material.h"

class LambertMaterial : public Material
{
public:
	LambertMaterial() : Material() {};
	LambertMaterial(MaterialPacked packed, Texture* texture) :
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

