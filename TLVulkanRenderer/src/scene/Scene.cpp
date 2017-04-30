#include "Scene.h"
#include "lights/PointLight.h"
#include "sceneLoaders/gltfLoader.h"
#include <iostream>
#include "accel/SBVH.h"
#include "geometry/materials/MetalMaterial.h"
#include "geometry/materials/GlassMaterial.h"
#include "geometry/materials/TranslucentMaterial.h"
#include "geometry/Cube.h"
#include "TimeCounter.h"
#include <algorithm>

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
		100,
		SBVH::Spatial
		));

	TimeCounter::GetInstance()->NewCounter(TQ_BUILD_BVH);
	ParseSceneFile(fileName);
	PrepareTestScene();
}


Scene::~Scene() {
	for (Model* geom : models) {
		if (geom) {
			delete geom;
			geom = nullptr;
		}
	}

	for (Material* mat : materials) {
		if (mat) {
			delete mat;
			mat = nullptr;
		}
	}

	for (Light* l : lights) {
		if (l) {
			delete l;
			l = nullptr;
		}
	}

	m_accel->Destroy();
}

void Scene::ParseSceneFile(std::string fileName)
{
	if(!m_sceneLoader->Load(fileName, this)) {
		throw std::runtime_error("Failed to load scene file");
	}
}

Intersection 
Scene::GetIntersection(Ray& ray) 
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
Scene::ShadowRay(
	Ray& ray, 
	ColorRGB& color
)
{
	if (m_useAccel) {
		return m_accel->ShadowRay(ray, color);
	} else {
		// Loop through all objects and find any intersection

		bool result = false;
		float minT = INFINITY;
		// Find the first opaque object that can cast shadow
		for (auto geo : geometries)
		{
			if (geo->GetMaterial()->m_translucent == false)
			{
				Intersection isx = geo->GetIntersection(ray);
				if (isx.t > 0 && isx.t < minT && isx.hitObject->GetMaterial()->m_castShadow == true)
				{
					minT = isx.t;
					color *= 0.1f;
					result = true;
					break;
				}
			}
		}

		// Color shadows if there are other translucent objects in front of it
		for (auto geo : geometries)
		{
			if (geo->GetMaterial()->m_translucent)
			{
				Intersection isx = geo->GetIntersection(ray);
				if (isx.t > 0 && isx.t < minT)
				{
					color *= isx.hitObject->GetMaterial()->m_colorTransparent * 0.9f;
					result = true;
				}
			}
		}
		
		return result;
	}
}

bool Scene::DoesIntersect(Ray & ray)
{
	if (m_useAccel)
	{
		return m_accel->DoesIntersect(ray);
	}
	else
	{
		// Loop through all objects and find any intersection
		for (auto geo : geometries)
		{
			Intersection isx = geo->GetIntersection(ray);
			if (isx.t > 0 && isx.hitObject->GetMaterial()->m_castShadow == true) // Bypass translucent objects
			{
				return true;
			}
		}
		return false;
	}
}

void Scene::PrepareTestScene()
{
	static const Point3 SPONZA_EYE(10, -5, 1.5);
	static const Point3 SPONZA_LOOKAT(-6, -6, 0);
	static const Point3 TRUCK_EYE(4.548, 4.427, 13.23);
	static const Point3 TRUCK_LOOKAT(0.926, 2.763, -2.958);
	static const Point3 ELLIE_EYE(-2.75, -155.1811, 120.7701);
	static const Point3 ELLIE_LOOKAT(-2.75, -135, -12);
	camera.eye = vec3(0, -10, 15);
	//camera.eye = vec3(2, 7, 15);
	//camera.eye = SPONZA_EYE;
	//camera.lookAt = SPONZA_LOOKAT;
	//camera.eye = ELLIE_EYE;
	//camera.lookAt = ELLIE_LOOKAT;
	camera.RecomputeAttributes();

	MetalMaterial* mirror = new MetalMaterial();
	mirror->m_colorReflective = ColorRGB(0.8, 0.8, 0.8);

	GlassMaterial* glass = new GlassMaterial();
	glass->m_colorTransparent = ColorRGB(0.8, 0.8, 0.8);
	glass->m_refracti = 1.333f;

	LambertMaterial* lambertWhite = new LambertMaterial();
	lambertWhite->m_colorDiffuse = vec3(0.6, 0.6, 0.7);
	materials.push_back(lambertWhite);



	for (auto mat : materials) {
		mat->m_colorAmbient = vec3(0.1, 0.1, 0.1);
	}

	// Turn meshes into triangles
	for (int m = 0; m < meshes.size(); m++)
	{
		size_t len = 232271 < meshes[m].triangles.size() ? 232271 : meshes[m].triangles.size();
		//len = meshes[m].triangles.size();
		//len = 1;
		for (int t = 0; t < len; t++)
		{
			std::string name = "triangle" + t;
			meshes[m].triangles[t].SetName(name);
			geometries.push_back(std::shared_ptr<Geometry>(&meshes[m].triangles[t]));
		}
	}

	// duplicate meshes

	 //Add spheres
	//int numSpheres = 5;
	//for (int i = 0; i < numSpheres; i++) {
	//	Material* mat;
	//	if (i % 2)
	//	{
	//		mat = mirror;
	//	}
	//	else
	//	{
	//		mat = glass;
	//	}
	//	vec3 position = glm::vec3(
	//		sin(glm::radians(360.0f * i / numSpheres)) * 1,
	//		-2.4 + (0.5 + i * 0.25) * 0.5 + 2.5,
	//		cos(glm::radians(360.0f * i / numSpheres)) * 1 + 3
	//	);
	//	float radius = 0.5 + i * 0.25;
	//	std::shared_ptr<Sphere> s(new Sphere(position, radius, mat));
	//	std::string name = "Sphere" + i;
	//	s.get()->SetName(name);
	//	geometries.push_back(s);

	//	SpherePacked packedSphere = { position, radius };
	//	spherePackeds.push_back(packedSphere);
	//}

	//PrepareBoxes();
	//PrepareCornellBox();

	// Add lights
	PointLight* light = new PointLight(vec3(0, 10.0, 10.0), vec3(1, 1, 1), 200);
	lights.push_back(light);

	//light = new PointLight(vec3(0, -2.1, 2), vec3(1, 2, 1), 10);
	//lights.push_back(light);

	if (m_useAccel)
	{
		TimeCounter::GetInstance()->BeginRecord(TQ_BUILD_BVH);
		m_accel->Build(geometries);
		TimeCounter::GetInstance()->EndRecord(TQ_BUILD_BVH);
		m_accel->PrintStats();
		std::cout << "BVH build time: " << TimeCounter::GetInstance()->GetAverageRunTime(TQ_BUILD_BVH) << " ms" << std::endl;
	}

	std::cout << "Number of triangles: " << indices.size() << std::endl;
}

void Scene::PrepareCornellBox() {

	LambertMaterial* lambertGreen = new LambertMaterial();
	lambertGreen->m_colorDiffuse = vec3(0, 0.6, 0);
	materials.push_back(lambertGreen);

	LambertMaterial* lambertLime = new LambertMaterial();
	lambertLime->m_colorDiffuse = vec3(0.6, 0.8, 0);
	materials.push_back(lambertLime);

	LambertMaterial* lambertRed = new LambertMaterial();
	lambertRed->m_colorDiffuse = vec3(0.6, 0.2, 0.1);
	materials.push_back(lambertRed);

	LambertMaterial* lambertWhite = new LambertMaterial();
	lambertWhite->m_colorDiffuse = vec3(0.6, 0.6, 0.7);
	materials.push_back(lambertWhite);


	std::shared_ptr<Cube> floor(new Cube(vec3(0, -2.5, 0), vec3(5, 0.2, 5), lambertRed));
	floor.get()->SetName(std::string("Floor"));
	geometries.push_back(floor);

	std::shared_ptr<Cube> leftWall(new Cube(vec3(2.5, 0, 0), vec3(0.2, 5, 5), lambertLime));
	leftWall.get()->SetName(std::string("Left Wall"));
	geometries.push_back(leftWall);

	std::shared_ptr<Cube> rightWall(new Cube(vec3(-2.5, 0, 0), vec3(0.2, 5, 5), lambertGreen));
	rightWall.get()->SetName(std::string("Right Wall"));
	geometries.push_back(rightWall);

	std::shared_ptr<Cube> backwall(new Cube(vec3(0, 0, -2.5), vec3(5, 5, 0.2), lambertWhite));
	backwall.get()->SetName(std::string("Back Wall"));
	geometries.push_back(backwall);

	std::shared_ptr<Cube> ceiling(new Cube(vec3(0, 2.5, 0), vec3(5, 0.2, 5), lambertWhite));
	ceiling.get()->SetName(std::string("Ceiling"));
	geometries.push_back(ceiling);
}

void Scene::PrepareBoxes() {
	LambertMaterial* lambertWhite = new LambertMaterial();
	lambertWhite->m_colorDiffuse = vec3(0.6, 0.6, 0.7);
	materials.push_back(lambertWhite);

	TranslucentMaterial* translucent = new TranslucentMaterial();
	translucent->m_colorTransparent = vec3(0.6, 0.6, 0.0);

	// Add buildings
	std::shared_ptr<Cube> road(new Cube(vec3(0, -0.5, -1), vec3(30, 1, 10), lambertWhite));
	road.get()->SetName(std::string("road"));
	geometries.push_back(road);

	std::shared_ptr<Cube> bulding1(new Cube(vec3(2, 4, -5.5), vec3(6, 10, 5), lambertWhite));
	bulding1.get()->SetName(std::string("bulding1"));
	geometries.push_back(bulding1);

	std::shared_ptr<Cube> bulding2(new Cube(vec3(-4, 2.5, -5.5), vec3(8, 7, 5), lambertWhite));
	bulding2.get()->SetName(std::string("bulding2"));
	geometries.push_back(bulding2);

	std::shared_ptr<Cube> bulding3(new Cube(vec3(-9, 3, -5.5), vec3(6, 9, 5), lambertWhite));
	bulding3.get()->SetName(std::string("bulding3"));
	geometries.push_back(bulding3);

	std::shared_ptr<Cube> screen(new Cube(vec3(-1, 2.5, 5), vec3(5, 5, 1), translucent));
	screen.get()->SetName(std::string("screen"));
	geometries.push_back(screen);
}
