#pragma once

#include "SceneUtil.h"
#include "scene/Camera.h"
#include <geometry/Geometry.h>
#include <geometry/materials/LambertMaterial.h>
#include <accel/AccelStructure.h>
#include "lights/Light.h"
#include "sceneLoaders/SceneLoader.h"


class Scene {
public:
	Scene(std::string fileName, std::map<std::string, std::string>& config);
	~Scene();

	void ParseSceneFile(std::string fileName);
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
	std::unique_ptr<AccelStructure> m_accel;


private:

	void PrepareTestScene();

	std::unique_ptr<SceneLoader> m_sceneLoader;	
	bool m_useAccel;

};
