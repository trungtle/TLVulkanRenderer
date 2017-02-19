#pragma once
#include "geometry/Geometry.h"
#include <memory>

struct SWireframe
{
	glm::vec3 position;
	glm::vec3 color;
};

class AccelStructure
{
public:

	virtual void Build(std::vector<std::shared_ptr<Geometry>>& geoms) = 0;	
	virtual Intersection GetIntersection(const Ray& r) = 0;
	virtual bool DoesIntersect(const Ray& r) = 0;
	virtual void GenerateVertices(std::vector<uint16>& indices, std::vector<SWireframe>& vertices) = 0;
	virtual void Destroy() = 0;
};
