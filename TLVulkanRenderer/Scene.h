#pragma once

#include <glm/glm.hpp>
#include <map>
#include "thirdparty/tinygltfloader/tiny_gltf_loader.h"

typedef unsigned char Byte;

typedef enum {
	INDEX,
	POSITION,
	NORMAL,
	TEXCOORD
} EVertexAttributeType;


typedef struct VertexAttributeInfoTyp
{
	size_t byteOffset;
	size_t byteStride;
	size_t count;
	int componentLength;
	int componentTypeByteSize;

} VertexAttributeInfo;

class Scene
{
public:
	Scene(std::string fileName);
	~Scene();
	
	std::map<EVertexAttributeType, std::vector<Byte>> m_vertexData;
	std::map<EVertexAttributeType, VertexAttributeInfo> m_vertexAttributes;
};

