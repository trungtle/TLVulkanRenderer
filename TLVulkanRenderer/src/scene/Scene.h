#pragma once

#include <map>
#include "tinygltfloader/tiny_gltf_loader.h"
#include "SceneUtil.h"
#include "scene/Camera.h"
#include <geometry/Geometry.h>
#include <geometry/materials/LambertMaterial.h>
#include <geometry/SBVH.h>
#include "lights/Light.h"


class Scene {
public:
	Scene(std::string fileName, std::map<std::string, std::string>& config);
	~Scene();

	Intersection GetIntersection(const Ray& ray);
	bool DoesIntersect(const Ray& ray);

	Camera camera;
	
	std::vector<glm::ivec4> indices;
	std::vector<glm::vec4> verticePositions;
	std::vector<glm::vec4> verticeNormals;
	std::vector<glm::vec2> verticeUVs;

	std::vector<MeshData*> meshesData;
	std::vector<MaterialPacked> materialPackeds;
	std::vector<LambertMaterial*> materials;
	std::vector<Mesh> meshes;
	std::vector<std::shared_ptr<Geometry>> geometries;
	std::vector<Light*> lights;
	SBVH m_sbvh;

private:
	
	bool m_useSBVH;

};
