#pragma once

#include <map>
#include "tinygltfloader/tiny_gltf_loader.h"
#include "SceneUtil.h"
#include "scene/Camera.h"
#include <geometry/Geometry.h>

class Camera;

class Scene {
public:
	Scene(std::string fileName);
	~Scene();

	Camera camera;
	std::vector<MeshData*> meshesData;
	std::vector<Material> materials;
	std::vector<glm::ivec4> indices;
	std::vector<glm::vec4> verticePositions;
	std::vector<glm::vec4> verticeNormals;
	std::vector<Mesh> meshes;
};
