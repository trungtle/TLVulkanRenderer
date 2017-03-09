#pragma once

#include "glm/glm.hpp"
#include <geometry/Geometry.h>
#include <memory>

typedef unsigned int Dim;

enum EPlane {
	XY,
	ZY,
	XZ
};

class BBox
{
public:
	glm::vec3 m_min;
	glm::vec3 m_max;
	glm::vec3 m_centroid;
	Transform m_transform;
	float m_area;
	bool m_isDirty;

	BBox() : 
		m_min(INFINITY, INFINITY, INFINITY), 
		m_max(-INFINITY, -INFINITY, -INFINITY), 
		m_centroid(vec3(0, 0, 0)),
		m_transform(Transform()),
		m_area(-1),
		m_isDirty(false)
	{}

	bool DoesIntersect(const Ray& r) const;
	glm::vec3 Offset(const glm::vec3& point) const;
	float GetSurfaceArea();

	bool IsInside(const vec3& point) const;

	static glm::vec3 Centroid(
		const glm::vec3& a,
		const glm::vec3& b
	);

	/**
	* \brief Generate a union bounding box from a and b
	* \param a : first bounding box
	* \param b : second bounding box
	* \return the union bounding box
	*/
	static BBox BBoxUnion(const BBox& a, const BBox& b);
	static BBox BBoxUnion(const BBox& a, const vec3& point);

	static BBox BBoxOverlap(const BBox& a, const BBox& b);
	static BBox BBoxFromPoints(std::vector<vec3>& points);

	/**
	* \brief Return the maximum extent axis of the bounding box. This is used to sort the BVH tree
	* \param bbox
	* \return
	*/
	static Dim BBoxMaximumExtent(const BBox& bbox);
};
