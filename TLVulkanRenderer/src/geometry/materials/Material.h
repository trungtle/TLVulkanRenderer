#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <ImageUtil.h>
#include <scene/SceneUtil.h>

class Intersection;

class Material {
public:
	Material()
		: m_shininess(0), m_refracti(0), m_reflectivity(0)
	{};
	Material(MaterialPacked packed, Texture& texture) :
		m_colorDiffuse(packed.diffuse),
		m_colorAmbient(packed.ambient),
		m_colorEmission(packed.emission),
		m_colorSpecular(packed.specular),
		m_colorReflective{glm::vec3()},
		m_colorTransparent{glm::vec3()},
		m_shininess(packed.shininess),
		m_refracti(packed.transparency),
		m_reflectivity(0.0)
	{};
	virtual ~Material() {}

	virtual glm::vec3 EvaluateEnergy(
		const Intersection& isx, 
		const glm::vec3& in, 
		glm::vec3& out) = 0;


	static void GetImageColor(glm::vec2 uv, Texture& texture) {
		
	}

	glm::vec3	m_colorDiffuse;
	glm::vec3	m_colorAmbient;
	glm::vec3	m_colorEmission;
	glm::vec3	m_colorSpecular;
	glm::vec3   m_colorReflective;
	glm::vec3   m_colorTransparent;
	float		m_shininess;
	float		m_refracti;
	float	    m_reflectivity;
	Texture		m_texture;
};