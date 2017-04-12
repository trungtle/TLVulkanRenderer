#pragma once
#include "Model.h"
#include <glm/glm.hpp>

// ---------
// MATERIAL
// ----------

typedef struct MaterialTyp {
	glm::vec4 diffuse;
	glm::vec4 ambient;
	glm::vec4 emission;
	glm::vec4 specular;
	float shininess;
	float transparency;
	glm::ivec2 _pad;
} MaterialPacked;
