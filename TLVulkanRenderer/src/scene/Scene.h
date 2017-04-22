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
	Intersection GetIntersection(Ray& ray);
	bool DoesIntersect(Ray & ray);
	bool ShadowRay(Ray& ray, ColorRGB& color);

	Camera camera;
	
	std::vector<glm::ivec4> indices;
	std::vector<glm::vec4> positions;
	std::vector<glm::vec4> normals;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> tangents;

	std::vector<Model*> models;
	std::vector<MaterialPacked> materialPackeds;
	std::vector<LambertMaterial*> materials;
	std::vector<Mesh> meshes;
	std::vector<std::shared_ptr<Geometry>> geometries;
	std::vector<SpherePacked> spherePackeds;
	std::vector<Light*> lights;
	std::unique_ptr<AccelStructure> m_accel;


private:

	void PrepareTestScene();
	void PrepareCornellBox();
	void PrepareBoxes();

	std::unique_ptr<SceneLoader> m_sceneLoader;	
	bool m_useAccel;

};
