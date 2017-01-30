#include "Geometry.h"
#include <geometry/BBox.h>

Geometry::Geometry() {
}


Geometry::~Geometry() {
}

BBox Sphere::GetBBox() {
	BBox bbox;

	// Convert to world first
	glm::vec3 p0 = glm::vec3(m_transform.T() * glm::vec4(-.5f, -.5f, -.5f, 1.0f));
	glm::vec3 p1 = glm::vec3(m_transform.T() * glm::vec4(-.5f, -.5f, .5f, 1.0f));
	glm::vec3 p2 = glm::vec3(m_transform.T() * glm::vec4(-.5f, .5f, -.5f, 1.0f));
	glm::vec3 p3 = glm::vec3(m_transform.T() * glm::vec4(-.5f, .5f, .5f, 1.0f));
	glm::vec3 p4 = glm::vec3(m_transform.T() * glm::vec4(.5f, -.5f, -.5f, 1.0f));
	glm::vec3 p5 = glm::vec3(m_transform.T() * glm::vec4(.5f, -.5f, .5f, 1.0f));
	glm::vec3 p6 = glm::vec3(m_transform.T()* glm::vec4(.5f, .5f, -.5f, 1.0f));
	glm::vec3 p7 = glm::vec3(m_transform.T() * glm::vec4(.5f, .5f, .5f, 1.0f));

	bbox.min = glm::min(p0, glm::min(p1, glm::min(p2, glm::min(p3, glm::min(p4, glm::min(p5, glm::min(p6, p7)))))));
	bbox.max = glm::max(p0, glm::max(p1, glm::max(p2, glm::max(p3, glm::max(p4, glm::max(p5, glm::max(p6, p7)))))));
	bbox.centroid = BBox::Centroid(bbox.min, bbox.max);
	bbox.m_transform = Transform(bbox.centroid, glm::vec3(0), bbox.max - bbox.min);
	return bbox;
}

BBox Cube::GetBBox() {
	BBox bbox;

	// Convert to world first
	glm::vec3 p0 = glm::vec3(m_transform.T() * glm::vec4(-.5f, -.5f, -.5f, 1.0f));
	glm::vec3 p1 = glm::vec3(m_transform.T() * glm::vec4(-.5f, -.5f, .5f, 1.0f));
	glm::vec3 p2 = glm::vec3(m_transform.T() * glm::vec4(-.5f, .5f, -.5f, 1.0f));
	glm::vec3 p3 = glm::vec3(m_transform.T() * glm::vec4(-.5f, .5f, .5f, 1.0f));
	glm::vec3 p4 = glm::vec3(m_transform.T() * glm::vec4(.5f, -.5f, -.5f, 1.0f));
	glm::vec3 p5 = glm::vec3(m_transform.T() * glm::vec4(.5f, -.5f, .5f, 1.0f));
	glm::vec3 p6 = glm::vec3(m_transform.T()* glm::vec4(.5f, .5f, -.5f, 1.0f));
	glm::vec3 p7 = glm::vec3(m_transform.T() * glm::vec4(.5f, .5f, .5f, 1.0f));

	bbox.min = glm::min(p0, glm::min(p1, glm::min(p2, glm::min(p3, glm::min(p4, glm::min(p5, glm::min(p6, p7)))))));
	bbox.max = glm::max(p0, glm::max(p1, glm::max(p2, glm::max(p3, glm::max(p4, glm::max(p5, glm::max(p6, p7)))))));
	bbox.centroid = BBox::Centroid(bbox.min, bbox.max);
	bbox.m_transform = Transform(bbox.centroid, glm::vec3(0), bbox.max - bbox.min);
	return bbox;
}

BBox Triangle::GetBBox() {
	BBox box;
	glm::vec3 p0 = glm::vec3(m_transform.T() * glm::vec4(vert0, 1.f));
	glm::vec3 p1 = glm::vec3(m_transform.T() * glm::vec4(vert1, 1.f));
	glm::vec3 p2 = glm::vec3(m_transform.T() * glm::vec4(vert2, 1.f));
	box.min = glm::min(p0, glm::min(p1, p2));
	box.max = glm::max(p0, glm::max(p1, p2));
	box.centroid = BBox::Centroid(box.min, box.max);
	box.m_transform = Transform(box.centroid, glm::vec3(0), box.max - box.min);
	return box;
}

BBox Mesh::GetBBox() {
	BBox result;
	for (auto g : triangles)
	{
		BBox box = g.GetBBox();
		result = BBox::BBoxUnion(result, box);
	}
	return result;
}
