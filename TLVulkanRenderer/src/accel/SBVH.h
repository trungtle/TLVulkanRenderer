#pragma once

#include "AccelStructure.h"
#include <geometry/BBox.h>
#include <memory>

struct SBVHGeometryInfo {
	GeomId geometryId;
	BBox bbox;
	bool straddling;
	int where;
	bool isSubGeom;
};

class SBVHNode {
public:

	SBVHNode(
		SBVHNode* nearChild, 
		SBVHNode* farChild, 
		SBVHNodeId nodeIdx, 
		EAxis splitAxis) : 
		m_parent(nullptr), 
		m_nearChild(nearChild),
		m_farChild(farChild),
		m_nodeIdx(nodeIdx),
		m_splitAxis(splitAxis) 
	{
		if (nearChild) {
			m_bbox = BBox::BBoxUnion(m_bbox, nearChild->m_bbox);
		}

		if (farChild) {
			m_bbox = BBox::BBoxUnion(m_bbox, farChild->m_bbox);
		}
	};

	virtual bool IsLeaf() {
		return false;
	}

	SBVHNode* m_parent;
	SBVHNode* m_nearChild;
	SBVHNode* m_farChild;
	BBox m_bbox;
	size_t m_nodeIdx;
	EAxis m_splitAxis;
};

class SBVHLeaf : public SBVHNode {
public:
	SBVHLeaf(
		SBVHNode* parent, 
		size_t nodeIdx, 
		size_t firstGeomOffset, 
		size_t numGeoms, 
		const BBox& bbox) : 
		SBVHNode(nullptr, nullptr, nodeIdx, EAxis::X),
		m_firstGeomOffset(firstGeomOffset), 
		m_numGeoms(numGeoms)
	{
		m_bbox = bbox;
	}

	bool IsLeaf() final {
		return true;
	}

	size_t m_firstGeomOffset;
	size_t m_numGeoms;
	std::vector<GeomId> m_geomIds;
};

class SBVH : public AccelStructure {

public:
	enum ESplitMethod
	{
		EqualCounts,
		SAH,
		SpatialSplit_SAH
	};

	SBVH() : m_root(nullptr), m_maxGeomsInNode(1), m_splitMethod(EqualCounts)
	{};

	SBVH(
		int maxPrimInNode,
		ESplitMethod splitMethod
	) : m_root(nullptr),
		m_maxGeomsInNode(maxPrimInNode),
		m_splitMethod(splitMethod)
	{}

	void Build(
		std::vector<std::shared_ptr<Geometry>>& geoms
	) override;

	void GenerateVertices(std::vector<uint16>& indices, std::vector<SWireframe>& vertices) override;
	Intersection GetIntersection(Ray& r) override;
	bool DoesIntersect(Ray& r) override;

	void Destroy() override;

	std::vector<SBVHNode*> m_nodes;

protected:
	SBVHNode*
		BuildRecursive(
			std::vector<SBVHGeometryInfo>& geomInfos,
			int first,
			int last,
			size_t& nodeCount,
			std::vector<std::shared_ptr<Geometry>>& orderedGeoms,
			int depth
		);
	
	void GetIntersectionRecursive(
		Ray& r, 
		SBVHNode* node, 
		float& nearestT, 
		Intersection& nearestIsx
	);

	bool DoesIntersectRecursive(
		Ray& r,
		SBVHNode* node);

	void 
	DestroyRecursive(SBVHNode* node);

	void
	Flatten();

	void
	FlattenRecursive(SBVHNode* node);


	SBVHNode* m_root;
	int m_maxGeomsInNode;
	ESplitMethod m_splitMethod;
	std::vector<std::shared_ptr<Geometry>> m_geoms;

};

