#pragma once

#include <geometry/BBox.h>
#include <memory>

struct SBVHGeometryInfo {
	size_t geometryId;
	BBox bbox;
};

class SBVHNode {
public:

	SBVHNode(
		SBVHNode* nearChild, 
		SBVHNode* farChild, 
		size_t nodeIdx, 
		BBox::EAxis splitAxis) : 
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
	BBox::EAxis m_splitAxis;
};

class SBVHLeaf : public SBVHNode {
public:
	SBVHLeaf(
		SBVHNode* parent, 
		size_t nodeIdx, 
		size_t firstGeomOffset, 
		size_t numGeoms, 
		const BBox& bbox) : 
		SBVHNode(nullptr, nullptr, nodeIdx, BBox::EAxis::X),
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
};

class SBVH {

public:
	enum ESplitMethod
	{
		EqualCounts,
		SAH
	};

	SBVH() : m_root(nullptr), m_maxGeomsInNode(1), m_splitMethod(SAH)
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
	);

	Intersection GetIntersection(const Ray& r);
	bool DoesIntersect(const Ray& r);

	void Destroy();

	std::vector<SBVHNode*> m_nodes;

protected:
	SBVHNode*
		BuildRecursive(
			std::vector<SBVHGeometryInfo>& geomInfos,
			int first,
			int last,
			size_t& nodeCount,
			std::vector<std::shared_ptr<Geometry>>& orderedGeoms
		);
	
	void GetIntersectionRecursive(const Ray& r, 
		SBVHNode* node, float& nearestT, Intersection& nearestIsx);

	bool DoesIntersectRecursive(
		const Ray& r,
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

