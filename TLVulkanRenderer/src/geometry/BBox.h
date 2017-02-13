#pragma once

#include "glm/glm.hpp"
#include <geometry/Geometry.h>
#include <memory>

typedef enum
{
	X,
	Y,
	Z
} EAxis;

enum EPlane {
	XY,
	ZY,
	XZ
};

enum EBBoxEdgeType
{
	START,
	END
};

struct BBoxFace {
	BBoxFace() : m_point(vec3(0)), m_normal(vec3(0)) {}

	BBoxFace(vec3 point, vec3 normal) :
		m_point(point), m_normal(normal)
	{};

	vec3 m_point;
	vec3 m_normal;
};

struct BBoxEdge
{
	BBoxEdge() : m_t(0), m_geomId(-1), m_type(EBBoxEdgeType::START)
	{}

	BBoxEdge(float t, size_t geomId, EBBoxEdgeType type) :
		m_t(t), m_geomId(geomId), m_type(type)
	{}

	float m_t;
	size_t m_geomId;
	EBBoxEdgeType m_type;
};

class BBox
{
public:
	glm::vec3 m_min;
	glm::vec3 m_max;
	glm::vec3 m_centroid;
	Transform m_transform;

	BBox() : m_min(INFINITY, INFINITY, INFINITY), m_max(-INFINITY, -INFINITY, -INFINITY)
	{}

	bool DoesIntersect(const Ray& r) const;
	glm::vec3 Offset(const glm::vec3& point) const;
	float GetSurfaceArea() const;


	/**
	 * \brief Clip geometry along a dimension
	 * \param geom 
	 * \param dim 
	 * \return 
	 */
	BBox ClipGeometry(std::shared_ptr<Geometry> geom, EAxis dim);
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
	static EAxis BBoxMaximumExtent(const BBox& bbox);
};
