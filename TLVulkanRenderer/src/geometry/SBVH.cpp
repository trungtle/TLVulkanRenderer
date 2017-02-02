#include "SBVH.h"
#include <algorithm>

// This comparator is used to sort bvh nodes based on its centroid's maximum extent
struct CompareCentroid
{
	CompareCentroid(int dim) : dim(dim) {};

	int dim;
	bool operator()(const SBVHNode* node1, const SBVHNode* node2) const {
		return node1->bbox.centroid[dim] < node2->bbox.centroid[dim];
	}
};

void 
SBVH::Build(
	const std::vector<Geometry*>& geoms
)
{
	std::vector<SBVHNode*> leaves;
	for (int i = 0; i < geoms.size(); i += m_maxPrimInNode)
	{
		std::vector<Geometry*> primitives;
		for (int j = 0; j < m_maxPrimInNode & (j + i) < geoms.size(); j++)
		{
			primitives.push_back(geoms[i + j]);
		}
		SBVHLeaf* leaf = new SBVHLeaf(primitives);
		leaves.push_back(leaf);
	}

	m_root = BuildRecursive(leaves, 0, leaves.size() - 1, geoms.size());
	Flatten();
}

SBVHNode*
SBVH::BuildRecursive(std::vector<SBVHNode*>& leaves, int first, int last, size_t nodeCount) {
	if (last < first || last < 0 || first < 0)
	{
		return nullptr;
	}

	if (last == first)
	{
		// We're at a leaf node
		return leaves.at(first);
	}

	auto node = new SBVHNode();
	node->nodeIdx = nodeCount;
	++nodeCount;

	// Choose a dimension to split
	auto dim = static_cast<int>((BBox::BBoxMaximumExtent(node->bbox)));

	// Compute the bounds of all geometries within this subtree
	for (auto i = first; i <= last; ++i)
	{
		node->bbox = BBox::BBoxUnion(leaves.at(i)->bbox, node->bbox);
	}

	// Partial sorting along the maximum extent and split at the middle
	int mid = (first + last) / 2;
	std::nth_element(&leaves[first], &leaves[mid], &leaves[last], CompareCentroid(dim));

	// Build near child
	node->nearChild = BuildRecursive(leaves, first, mid, nodeCount);
	node->nearChild->parent = node;
	node->nearChild->splitAxis = static_cast<BBox::EAxis>(dim);

	// Build far child
	node->farChild = BuildRecursive(leaves, mid + 1, last, nodeCount);
	node->farChild->parent = node;
	node->farChild->splitAxis = static_cast<BBox::EAxis>(dim);
	return node;
}


Intersection SBVH::GetIntersection(const Ray& r) 
{
	float nearestT = INFINITY;
	Intersection nearestIsx; 

	if (m_root->IsLeaf()) {
		SBVHLeaf* leaf = dynamic_cast<SBVHLeaf*>(m_root);
		for (auto prim : leaf->m_geoms)
		{
			Intersection isx = prim->GetIntersection(r);
			if (isx.t > 0 && isx.t < nearestT)
			{
				nearestT = isx.t;
				nearestIsx = isx;
			}
		}
		return nearestIsx;
	}

	// Traverse children
	GetIntersectionRecursive(r, m_root->nearChild, nearestT, nearestIsx);
	GetIntersectionRecursive(r, m_root->farChild, nearestT, nearestIsx);

	return nearestIsx;
}


void SBVH::GetIntersectionRecursive(
	const Ray& r, 
	SBVHNode* node, 
	float& nearestT, 
	Intersection& nearestIsx
	) 
{
	if (node == nullptr) {
		return;
	}

	if (node->IsLeaf()) {
		SBVHLeaf* leaf = dynamic_cast<SBVHLeaf*>(node);

		// Return nearest primitive
		for (auto prim : leaf->m_geoms) {
			Intersection isx = prim->GetIntersection(r);
			if (isx.t > 0 && isx.t < nearestT)
			{
				nearestT = isx.t;
				nearestIsx = isx;
			}
		}
		return;
	}

	Intersection isx = node->bbox.GetIntersection(r);
	if (isx.t > 0)
	{
		// Traverse children
		GetIntersectionRecursive(r, node->nearChild, nearestT, nearestIsx);
		GetIntersectionRecursive(r, node->farChild, nearestT, nearestIsx);
	}
}


bool SBVH::DoesIntersect(
	const Ray& r
	)
{
	if (m_root->IsLeaf())
	{
		SBVHLeaf* leaf = dynamic_cast<SBVHLeaf*>(m_root);

		for (auto prim : leaf->m_geoms)
		{
			Intersection isx = prim->GetIntersection(r);
			if (isx.t > 0)
			{
				return true;
			}
		}
		return false;
	}

	// Traverse children
	return DoesIntersectRecursive(r, m_root->nearChild) || DoesIntersectRecursive(r, m_root->farChild);
}

bool SBVH::DoesIntersectRecursive(
	const Ray& r, 
	SBVHNode* node
	)
{
	if (node == nullptr)
	{
		return false;
	}

	if (node->IsLeaf())
	{
		SBVHLeaf* leaf = dynamic_cast<SBVHLeaf*>(node);
		for (auto prim : leaf->m_geoms)
		{
			Intersection isx = prim->GetIntersection(r);
			if (isx.t > 0)
			{
				return true;
			}
		}
		return false;
	}

	Intersection isx = node->bbox.GetIntersection(r);
	if (isx.t > 0)
	{
		// Traverse children
		return DoesIntersectRecursive(r, node->nearChild) || DoesIntersectRecursive(r, node->farChild);
	}
	return false;
}



void SBVH::Destroy() {
	DestroyRecursive(m_root);
}

void 
SBVH::DestroyRecursive(SBVHNode* node) {

	if (node->IsLeaf())
	{
		delete node;
		node = nullptr;
	}

	DestroyRecursive(node->nearChild);
	DestroyRecursive(node->farChild);

	delete node;
	node == nullptr;
}

void SBVH::FlattenRecursive(
	SBVHNode* node
	)
{
	if (node == nullptr)
	{
		return;
	}

	m_nodes.push_back(node);
	FlattenRecursive(node->nearChild);
	FlattenRecursive(node->farChild);
}

void SBVH::Flatten() {

	FlattenRecursive(m_root);
}