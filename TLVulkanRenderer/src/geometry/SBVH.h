#pragma once

#include <geometry/BBox.h>

class SBVHNode {
public:

	SBVHNode() {};

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
		m_geoms(geoms)
	{
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
	void Traverse();

	void Destroy();

protected:

	SBVHNode* 
	BuildRecursive(std::vector<SBVHNode*>& leaves, int first, int last, size_t nodeCount);
	
	void 
	DestroyRecursive(SBVHNode* node);

	SBVHNode* m_root;
	int m_maxPrimInNode;
	ESplitMethod m_splitMethod;
};

