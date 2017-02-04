#pragma once

#include "glm/glm.hpp"
#include <geometry/Geometry.h>

class BBox
{
public:
	typedef enum
	{
		X,
		Y,
		Z
	} EAxis;

	glm::vec3 m_min;
	glm::vec3 m_max;
	glm::vec3 m_centroid;
	Transform m_transform;

	BBox() : m_min(INFINITY, INFINITY, INFINITY), m_max(-INFINITY, -INFINITY, -INFINITY)
	{}

	bool DoesIntersect(const Ray& r) const;
	glm::vec3 Offset(const glm::vec3& point) const;
	float GetSurfaceArea() const;

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

	/**
	* \brief Return the maximum extent axis of the bounding box. This is used to sort the BVH tree
	* \param bbox
	* \return
	*/
	static EAxis BBoxMaximumExtent(const BBox& bbox);
};
