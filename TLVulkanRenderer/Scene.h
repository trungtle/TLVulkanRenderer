#pragma once

#include <glm/glm.hpp>
#include <map>
#include "thirdparty/tinygltfloader/tiny_gltf_loader.h"
#include "SceneUtil.h"

class Scene
{
public:
	Scene(std::string fileName);
	~Scene();
	
	std::map<EVertexAttributeType, std::vector<Byte>> m_vertexData;
	std::map<EVertexAttributeType, VertexAttributeInfo> m_vertexAttributes;
};

