#pragma once

typedef unsigned char Byte;

typedef enum
{
	INDEX,
	POSITION,
	NORMAL,
	TEXCOORD
} EVertexAttributeType;


typedef struct VertexAttributeInfoTyp
{
	size_t byteStride;
	size_t count;
	int componentLength;
	int componentTypeByteSize;

} VertexAttributeInfo;

typedef struct MaterialTyp
{
	glm::vec4 diffuse;
	glm::vec4 ambient;
	glm::vec4 emission;
	glm::vec4 specular;
	float shininess;
	float transparency;
} Material;


