#pragma once

#include "AccelStructure.h"
#include <geometry/BBox.h>
#include <memory>
#include <unordered_set>
#include <string>
#include <gtx/string_cast.hpp>

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

	virtual std::string ToString() {
		std::string str = "ID: ";
		str += std::to_string(m_nodeIdx);
		str += ", Min: ";
		str += glm::to_string(m_bbox.m_min);
		str += ", Max: ";
		str += glm::to_string(m_bbox.m_max);

		if (m_nearChild) {
			str += ", Near ID: ";
			str += std::to_string(m_nearChild->m_nodeIdx);
		}

		if (m_farChild) {
			str += ", Far ID: ";
			str += std::to_string(m_farChild->m_nodeIdx);
		}

		str += ", Is Leaf? ";
		str += IsLeaf() ? "True" : "False";

		str += "\n";

		return str;
	}

	SBVHNode* m_parent;
	SBVHNode* m_nearChild;
	SBVHNode* m_farChild;
	BBox m_bbox;
	size_t m_nodeIdx;
	Dim m_dim;
	size_t m_depth;
	bool m_isSpatialSplit;
};

struct SBVHNodePacked {
	int32_t parent;
	int32_t geomId;
	int32_t nearId;

	int32_t farId;
	glm::vec4 min;
	glm::vec4 max;

	inline std::string ToString() const {
		std::string str = "";
		str += ", Min: ";
		str += glm::to_string(min);
		str += ", Max: ";
		str += glm::to_string(max);

		str += ", Parent ID: ";
		str += std::to_string(parent);

		str += ", Near ID: ";
		str += std::to_string(nearId);

		str += ", Far ID: ";
		str += std::to_string(farId);

		str += ", Prim ID: ";
		str += std::to_string(geomId);

		str += "\n";

		return str;
	}
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

	void GenerateVertices(std::vector<uint16>& indices, std::vector<SWireframeVertexLayout>& vertices) override;
	Intersection GetIntersection(Ray& r) override;
	bool DoesIntersect(Ray& r) override;
	bool ShadowRay(
		Ray& ray,
		ColorRGB& color) override;

	void Destroy() override;

	std::vector<SBVHNode*> m_nodes;
	std::vector<SBVHNodePacked> m_nodesPacked;

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

	void PrintStats();

	SBVHNode* m_root;
	int m_maxGeomsInNode;
	ESplitMethod m_splitMethod;
	std::vector<std::shared_ptr<Geometry>> m_prims;
	unsigned int m_depthLimit = 20;
	unsigned int m_maxDepth = 0;
	size_t m_spatialSplitBudget = 1000;
	unsigned int m_spatialSplitCount = 0;
	size_t m_numPrimInfos = 0;
};

