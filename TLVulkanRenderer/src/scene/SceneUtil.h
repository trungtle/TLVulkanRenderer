#pragma once
#include "Typedef.h"
#include <glm/glm.hpp>
#include <map>
#include <vector>

// ---------
// VERTEX
// ----------

typedef enum {
	INDEX,
	POSITION,
	NORMAL,
	TEXCOORD,
	COLOR,
	TANGENT,
	BITANGENT,
	MATERIALID
} EVertexAttribute;

typedef struct VertexAttributeInfoTyp {
	size_t byteStride;
	size_t count;
	int componentLength;
	int componentTypeByteSize;
} VertexAttributeInfo;

// ---------
// GEOMETRY
// ----------

struct VertexData {
	std::map<EVertexAttribute, std::vector<Byte>> vertexData;
	std::map<EVertexAttribute, VertexAttributeInfo> attribInfo;
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
} MaterialPacked;
