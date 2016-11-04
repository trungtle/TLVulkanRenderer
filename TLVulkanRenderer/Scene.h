#pragma once

#include <glm/glm.hpp>
#include <map>
#include "thirdparty/tinygltfloader/tiny_gltf_loader.h"
#include "SceneUtil.h"

typedef struct GeometryDataTyp
{
	std::map<EVertexAttributeType, std::vector<Byte>> vertexData;
	std::map<EVertexAttributeType, VertexAttributeInfo> vertexAttributes;
} GeometryData;

class Scene
{
public:
	Scene(std::string fileName);
	~Scene();
	
	std::vector<GeometryData*> m_geometriesData;
	std::vector<Material> m_materials;
};

