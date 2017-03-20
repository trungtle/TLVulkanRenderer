#pragma once
#include "Material.h"

class TranslucentMaterial : public Material
{
public:
	TranslucentMaterial() : Material() {};
	TranslucentMaterial(MaterialPacked packed, Texture* texture) :
		Material(packed, texture)
	{
		m_castShadow = true;
		m_receiveShadow = false;
		m_translucent = true;
	}

	ColorRGB EvaluateEnergy(
		const Intersection& isx,
		const Direction& lightDirection,
		const Ray& in,
		Ray& out,
		bool& shouldTerminate
	) override;
};

