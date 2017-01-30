#pragma once

#include <geometry/BBox.h>

class SBVHNode {
public:

	SBVHNode() : 
		parent(nullptr), 
		nearChild(nullptr), 
		farChild(nullptr),
		nodeIdx(-1)
	{};

	virtual bool IsLeaf() {
		return false;
	}

	SBVHNode* parent;
	SBVHNode* nearChild;
	SBVHNode* farChild;
	BBox bbox;
	uint32_t nodeIdx;
	BBox::EAxis splitAxis;
};

class SBVHLeaf : public SBVHNode {
public:
	SBVHLeaf(std::vector<Geometry*> geoms) : 
		SBVHNode(), m_geoms(geoms)
	{
		for (auto g : geoms) {
			BBox b = g->GetBBox();
			bbox = BBox::BBoxUnion(b, bbox);
		}
	}

	bool IsLeaf() final {
		return true;
	}

	std::vector<Geometry*> m_geoms;
};

class SBVH {

public:
	enum ESplitMethod
	{
		SAH
	};

	SBVH() : m_root(nullptr), m_maxPrimInNode(1), m_splitMethod(SAH)
	{};

	SBVH(
		int maxPrimInNode,
		ESplitMethod splitMethod
	) : m_root(nullptr),
		m_maxPrimInNode(maxPrimInNode),
		m_splitMethod(splitMethod)
	{}

	void Build(
		const std::vector<Geometry*>& geoms
	);

	Intersection GetIntersection(const Ray& r);
	bool DoesIntersect(const Ray& r);

	void Destroy();

	std::vector<SBVHNode*> m_nodes;

protected:

	SBVHNode* 
	BuildRecursive(std::vector<SBVHNode*>& leaves, int first, int last, size_t nodeCount);
	
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
	int m_maxPrimInNode;
	ESplitMethod m_splitMethod;
};

