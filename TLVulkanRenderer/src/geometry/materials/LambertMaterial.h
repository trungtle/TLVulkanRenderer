#pragma once
#include "Material.h"

class LambertMaterial : public Material
{
public:
	LambertMaterial() : Material() {};
	LambertMaterial(MaterialPacked packed, Texture& texture) :
		Material(packed, texture)
	{}

	glm::vec3 EvaluateEnergy(
		const Intersection& isx,
		const glm::vec3& in,
		glm::vec3& out
	) override;
};

