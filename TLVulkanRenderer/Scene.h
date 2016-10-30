#pragma once

typedef struct VertexAttributesTyp 
{
	size_t positionByteOffset;
	size_t positionByteStride;
	size_t positionCount;

	size_t normalByteOffset;
	size_t normalByteStride;
	size_t normalCount;
	
	size_t texcoordByteOffset;
	size_t texcoordByteStride;
	size_t texcoordCount;

} VertexAttributes;

class Scene
{
public:
	Scene();
	~Scene();
	
	std::vector<unsigned char> m_indexData;
	std::vector<unsigned char> m_vertexData;
	int m_indexCount;

	VertexAttributes m_vertexAttributes;
};

