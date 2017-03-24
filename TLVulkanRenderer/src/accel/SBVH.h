#pragma once

#include "AccelStructure.h"
#include <geometry/BBox.h>
#include <memory>
#include <unordered_set>

typedef size_t PrimID;
typedef size_t SBVHNodeId;
typedef size_t BucketID;
typedef float Cost;

const PrimID INVALID_ID = std::numeric_limits<size_t>::max();
const BucketID NUM_BUCKET = 12;

struct PrimInfo
{
	PrimID primitiveId;
	BBox bbox;
	PrimID origPrimOffset;
};

class BucketInfo
{
public:
	BucketInfo() : count(0), bbox{ BBox() }, enter(0), exit(0) {};

	int count = 0;
	BBox bbox;
	int enter = 0; // Number of entering references
	int exit = 0; // Number of exiting references 
	std::vector<PrimInfo> fragments;
};

class SBVHNode {
public:

	SBVHNode(
		SBVHNode* parent,
		SBVHNode* nearChild, 
		SBVHNode* farChild, 
		SBVHNodeId nodeIdx, 
		Dim m_dim) : 
		m_parent(parent),
		m_nearChild(nearChild),
		m_farChild(farChild),
		m_nodeIdx(nodeIdx),
		m_dim(m_dim)
	{
		if (nearChild) {
			m_bbox = BBox::BBoxUnion(m_bbox, nearChild->m_bbox);
		}

		if (farChild) {
			m_bbox = BBox::BBoxUnion(m_bbox, farChild->m_bbox);
		}
	};

	virtual ~SBVHNode() {
		if (m_nearChild) {
			delete m_nearChild;
		}

		if (m_farChild) {
			delete m_farChild;
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
	Dim m_dim;
};

class SBVHLeaf : public SBVHNode {
public:
	SBVHLeaf(
		SBVHNode* parent, 
		size_t nodeIdx, 
		size_t firstGeomOffset, 
		size_t numGeoms, 
		const BBox& bbox) : 
		SBVHNode(parent, nullptr, nullptr, nodeIdx, 0),
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
	std::unordered_set<PrimID> m_geomIds;
};

class SBVH : public AccelStructure {

public:
	enum ESplitMethod
	{
		EqualCounts,
		SAH,
		Spatial
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
	bool ShadowRay(
		Ray& ray,
		ColorRGB& color) override;

	void Destroy() override;

	std::vector<SBVHNode*> m_nodes;

protected:
	SBVHNode*
	BuildRecursive(
		PrimID first,
		PrimID last,
		PrimID& nodeCount,
		std::vector<PrimInfo>& geomInfos,
		std::vector<std::shared_ptr<Geometry>>& orderedGeoms,
		int depth,
		bool shouldInsertAtBack
	);
	
	void
	GetIntersectionRecursive(
		Ray& r, 
		SBVHNode* node, 
		float& nearestT, 
		Intersection& nearestIsx
	);

	bool 
	DoesIntersectRecursive(
		Ray& r,
		SBVHNode* node);

	bool 
	ShadowRayRecursive(
		Ray& ray, 
		ColorRGB& color,
		SBVHNode* node);

	void 
	DestroyRecursive(SBVHNode* node);

	void
	Flatten();

	void
	FlattenRecursive(SBVHNode* node);

	void PartitionEqualCounts(
		Dim dim,
		PrimID first,
		PrimID last,
		PrimID& mid,
		std::vector<PrimInfo>& geomInfos
		) const;

	static void
	PartitionObjects(
		Cost minCostBucket,
		Dim dim,
		PrimID first,
		PrimID last,
		PrimID& mid,
		std::vector<PrimInfo>& geomInfos,
		BBox& bboxCentroids
		);

	static void
	PartitionSpatial(
		BucketID minCostBucket,
		Dim dim,
		PrimID first,
		PrimID last,
		PrimID& mid,
		std::vector<PrimInfo>& geomInfos,
		BBox& bboxAllGeoms
		);

	SBVHLeaf*
	CreateLeaf(
		SBVHNode* parent,
		PrimID first,
		PrimID last,
		PrimID& nodeCount,
		std::vector<PrimInfo>& geomInfos,
		std::vector<std::shared_ptr<Geometry>>& orderedGeoms,
		BBox& bboxAllGeoms
		);

	static SBVHNode*
	CreateNode(
		SBVHNode* parent,
		SBVHNode* nearChild,
		SBVHNode* farChild,
		PrimID& nodeCount,
		Dim dim
		);

	Cost
	CalculateLeafCost();

	std::tuple<Cost, BucketID>
	CalculateObjectSplitCost(
		Dim dim,
		PrimID first,
		PrimID last,
		std::vector<PrimInfo>& geomInfos,
		BBox& bboxCentroids,
		BBox& bboxAllGeoms
	) const;

	std::tuple<Cost, BucketID>
	CalculateSpatialSplitCost(
		Dim dim,
		PrimID first,
		PrimID last,
		std::vector<PrimInfo>& geomInfos,
		BBox& bboxAllGeoms,
		std::vector<BucketInfo>& buckets
		) const;

	SBVHNode* m_root;
	int m_maxGeomsInNode;
	ESplitMethod m_splitMethod;
	std::vector<std::shared_ptr<Geometry>> m_prims;
	unsigned int m_maxDepth = 10;
	size_t m_spatialSplitBudget = 10;
	unsigned int m_spatialSplitCount = 0;
};

