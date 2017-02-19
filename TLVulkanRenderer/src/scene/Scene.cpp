#include "Scene.h"
#include "lights/PointLight.h"
#include "sceneLoaders/gltfLoader.h"
#include <iostream>
#include "accel/SBVH.h"

Scene::Scene(
	std::string fileName,
	std::map<std::string, std::string>& config	
) : m_useAccel(false) 
{
	m_sceneLoader.reset(new gltfLoader());

	// Parse scene config
	if (config.find("USE_SBVH") != config.end()) {
		m_useAccel = config["USE_SBVH"].compare("true") == 0;
	}

	// Construct SBVH
	m_accel.reset(new SBVH(
		1,
		SBVH::SAH
		));

	ParseSceneFile(fileName);
	PrepareTestScene();
}


Scene::~Scene() {
	for (MeshData* geom : meshesData) {
		delete geom;
		geom = nullptr;
	}

	for (Material* mat : materials) {
		delete mat;
		mat = nullptr;
	}

	for (Light* l : lights) {
		delete l;
		l = nullptr;
	}

	m_accel->Destroy();
}

void Scene::ParseSceneFile(std::string fileName)
{
	m_sceneLoader->Load(fileName, this);
}

Intersection 
Scene::GetIntersection(const Ray& ray) 
{
	if (m_useAccel) {
		return m_accel->GetIntersection(ray);
	} else {

		// Loop through all objects in scene
		float nearestT = INFINITY;
		Intersection nearestIsx;
		for (auto geo : geometries)
		{
			Intersection isx = geo->GetIntersection(ray);
			if (isx.t > 0 && isx.t < nearestT)
			{
				nearestT = isx.t;
				nearestIsx = isx;
			}
		}

		return nearestIsx;
	}
}

bool 
Scene::DoesIntersect(const Ray& ray) 
{
	if (m_useAccel) {
		return m_accel->DoesIntersect(ray);
	} else {
		// Loop through all objects and find any intersection
		for (auto geo : geometries)
		{
			Intersection isx = geo->GetIntersection(ray);
			if (isx.t > 0)
			{
				return true;
			}
		}
		return false;
	}
}

void Scene::PrepareTestScene()
{
	// Setup materials
	LambertMaterial* reflectiveMat = new LambertMaterial();
	reflectiveMat->m_colorDiffuse = vec3(0.2, 0.3, 0.6);
	reflectiveMat->m_reflectivity = 0.5;
	reflectiveMat->m_colorReflective = vec3(1, 1, 1);
	materials.push_back(reflectiveMat);

	LambertMaterial* lambertGreen = new LambertMaterial();
	lambertGreen->m_colorDiffuse = vec3(0, 0.6, 0);
	materials.push_back(lambertGreen);

	LambertMaterial* lambertLime = new LambertMaterial();
	lambertLime->m_colorDiffuse = vec3(0.6, 0.8, 0);
	materials.push_back(lambertLime);

	LambertMaterial* lambertRed = new LambertMaterial();
	lambertRed->m_colorDiffuse = vec3(0.6, 0.1, 0);
	materials.push_back(lambertRed);

	LambertMaterial* lambertWhite = new LambertMaterial();
	lambertWhite->m_colorDiffuse = vec3(0.5, 0.5, 0.5);
	materials.push_back(lambertWhite);

	for (auto mat : materials) {
		mat->m_colorAmbient = vec3(0.1, 0.1, 0.1);
	}


	// Turn meshes into triangles
	for (int m = 0; m < meshes.size(); m++)
	{
		meshes[m].SetTransform(Transform(glm::vec3(0, 0, 0), glm::vec3(0), glm::vec3(1, 1, 1)));
		for (int t = 0; t < meshes[m].triangles.size(); t++)
		{
			std::string name = "triangle" + t;
			meshes[m].triangles[t].SetName(name);
			geometries.push_back(std::shared_ptr<Geometry>(&meshes[m].triangles[t]));
		}
	}

	// Add spheres
	int numSpheres = 0;
	for (int i = 0; i < numSpheres; i++) {
		std::shared_ptr<Sphere> s(new Sphere(glm::vec3(
			sin(glm::radians(360.0f * i / numSpheres)) * 2,
			cos(glm::radians(360.0f * i / numSpheres)) * 2,
			-1), 1, lambertWhite));
		std::string name = "Sphere" + i;
		s.get()->SetName(name);
		geometries.push_back(s);
	}

	// Add planes
	//std::shared_ptr<Cube> floor(new Cube(vec3(0, -2.5, 0), vec3(5, 0.2, 5), lambertRed));
	//floor.get()->SetName(std::string("Floor"));
	//geometries.push_back(floor);

	//std::shared_ptr<Cube> leftWall(new Cube(vec3(2.5, 0, 0), vec3(0.2, 5, 5), lambertLime));
	//leftWall.get()->SetName(std::string("Left Wall"));
	//geometries.push_back(leftWall);

	//std::shared_ptr<Cube> rightWall(new Cube(vec3(-2.5, 0, 0), vec3(0.2, 5, 5), lambertGreen));
	//rightWall.get()->SetName(std::string("Right Wall"));
	//geometries.push_back(rightWall);

	//std::shared_ptr<Cube> backwall(new Cube(vec3(0, 0, -2.5), vec3(5, 5, 0.2), lambertWhite));
	//backwall.get()->SetName(std::string("Back Wall"));
	//geometries.push_back(backwall);

	//std::shared_ptr<Cube> ceiling(new Cube(vec3(0, 2.5, 0), vec3(5, 0.2, 5), lambertWhite));
	//ceiling.get()->SetName(std::string("Ceiling"));
	//geometries.push_back(ceiling);


	// Add lights
	PointLight* light = new PointLight(vec3(0, 10.0, 10.0), vec3(1, 1, 1), 200);
	lights.push_back(light);

	//light = new PointLight(vec3(0, -2.1, 2), vec3(1, 2, 1), 10);
	//lights.push_back(light);

	m_accel->Build(geometries);

	std::cout << "Number of triangles: " << indices.size() << std::endl;
}
