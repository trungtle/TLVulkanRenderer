#pragma once
#include "Typedef.h"
#include <glm/glm.hpp>

// ---------
// VERTEX
// ----------

typedef enum {
	INDEX,
	POSITION,
	NORMAL,
	TEXCOORD
} EVertexAttributeType;

typedef struct VertexAttributeInfoTyp {
	size_t byteStride;
	size_t count;
	int componentLength;
	int componentTypeByteSize;

} VertexAttributeInfo;

// ---------
// GEOMETRY
// ----------

struct MeshData {
	std::map<EVertexAttributeType, std::vector<Byte>> vertexData;
	std::map<EVertexAttributeType, VertexAttributeInfo> vertexAttributes;
};

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
} Material;
