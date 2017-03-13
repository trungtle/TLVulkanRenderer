#pragma once
#include "Material.h"

class MetalMaterial : public Material
{
public:
	MetalMaterial() : Material() {};
	MetalMaterial(MaterialPacked packed, Texture* texture) :
		Material(packed, texture)
	{}

	glm::vec3 EvaluateEnergy(
		const Intersection& isx,
		const glm::vec3& in,
		glm::vec3& out
	) override;
};

