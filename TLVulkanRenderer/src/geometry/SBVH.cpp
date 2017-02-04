#include "SBVH.h"
#include <algorithm>

// This comparator is used to sort bvh nodes based on its centroid's maximum extent
struct CompareCentroid
{
	CompareCentroid(int dim) : dim(dim) {};

	int dim;
	bool operator()(const SBVHGeometryInfo node1, SBVHGeometryInfo node2) const {
		return node1.bbox.m_centroid[dim] < node2.bbox.m_centroid[dim];
	}
};

void 
SBVH::Build(
	std::vector<std::shared_ptr<Geometry>>& geoms
)
{
	m_geoms = geoms;

	// Initialize geom info
	std::vector<SBVHGeometryInfo> geomInfos(geoms.size());
	for (size_t i = 0; i < geomInfos.size(); i++)
	{
		geomInfos[i] = { i, geoms[i]->GetBBox() };
	}

	size_t totalNodes = 0;

	std::vector<std::shared_ptr<Geometry>> orderedGeoms;
	m_root = BuildRecursive(geomInfos, 0, geomInfos.size(), totalNodes, orderedGeoms);
	m_geoms.swap(orderedGeoms);
	Flatten();
}

void PartitionEqualCounts(int dim, int first, int last, int& mid, std::vector<SBVHGeometryInfo>& geomInfos) {
	// Partial sorting along the maximum extent and split at the middle
	// Sort evenly to each half
	mid = (first + last) / 2;
	std::nth_element(&geomInfos[first], &geomInfos[mid], &geomInfos[last - 1] + 1, CompareCentroid(dim));

}

SBVHNode*
SBVH::BuildRecursive(
	std::vector<SBVHGeometryInfo>& geomInfos, 
	int first, 
	int last, 
	size_t& nodeCount,
	std::vector<std::shared_ptr<Geometry>>& orderedGeoms
	) 
{
	if (last < first || last < 0 || first < 0)
	{
		return nullptr;
	}

	// Compute bounds of all geometries in node
	BBox bboxAllGeoms;
	for (int i = first; i < last; i++) {
		bboxAllGeoms = BBox::BBoxUnion(bboxAllGeoms, geomInfos[i].bbox);
	}

	int numPrimitives = last - first;
	
	if (numPrimitives == 1) {
		// Create a leaf node
		size_t firstGeomOffset = orderedGeoms.size();

		for (int i = first; i < last; i++) {
			int geomIdx = geomInfos[i].geometryId;
			orderedGeoms.push_back(m_geoms[geomIdx]);
		}
		SBVHLeaf* leaf = new SBVHLeaf(nullptr, nodeCount, firstGeomOffset, numPrimitives, bboxAllGeoms);
		nodeCount++;
		return leaf;
	}

	// Choose a dimension to split
	BBox bboxCentroids;
	for (int i = first; i < last; i++) {
		bboxCentroids = BBox::BBoxUnion(bboxCentroids, geomInfos[i].bbox.m_centroid);
	}
	auto dim = static_cast<int>((BBox::BBoxMaximumExtent(bboxCentroids)));
	// If all centroids are the same, create leafe since there's no effective way to split the tree
	if (bboxCentroids.m_max[dim] == bboxCentroids.m_min[dim]) {
		// Create a leaf node
		size_t firstGeomOffset = orderedGeoms.size();

		for (int i = first; i < last; i++)
		{
			int geomIdx = geomInfos[i].geometryId;
			orderedGeoms.push_back(m_geoms[geomIdx]);
		}
		SBVHLeaf* leaf = new SBVHLeaf(nullptr, nodeCount, firstGeomOffset, numPrimitives, bboxAllGeoms);
		nodeCount++;
		return leaf;
	}

	int mid;
	switch(m_splitMethod) {
		case SAH: // Partition based on surface area heuristics
			if (numPrimitives <= 4)
			{
				PartitionEqualCounts(dim, first, last, mid, geomInfos);
			} else {
				// Create 12 buckets (based from pbrt)
				const int NUM_BUCKET = 12;
				struct BucketInfo {
					int count = 0;
					BBox bbox;
				};

				BucketInfo buckets[NUM_BUCKET];

				// For each primitive in range, determine which bucket it falls into
				for (int i = first; i < last; i++) {
					int b = NUM_BUCKET * bboxCentroids.Offset(geomInfos.at(i).bbox.m_centroid)[dim];
					assert(b < NUM_BUCKET);
					if (b == NUM_BUCKET) b = NUM_BUCKET - 1;
					
					buckets[b].count++;
					buckets[b].bbox = BBox::BBoxUnion(buckets[b].bbox, geomInfos.at(i).bbox);
				}

				// Compute cost for splitting after each bucket
				float costs[NUM_BUCKET - 1];
				for (int i = 0; i < NUM_BUCKET - 1; i++) {
					BBox bbox0, bbox1;
					int count0 = 0, count1 = 0;

					// Compute cost for buckets before split candidate
					for (int j = 0; j <= i ; j++) {
						bbox0 = BBox::BBoxUnion(bbox0, buckets[j].bbox);
						count0 += buckets[j].count;
					}

					// Compute cost for buckets after split candidate
					for (int j = i  + 1; j < NUM_BUCKET; j++)
					{
						bbox1 = BBox::BBoxUnion(bbox1, buckets[j].bbox);
						count1 += buckets[j].count;
					}

					const float COST_TRAVERSAL = 0.125f;
					const float COST_INTERSECTION = 1.0f;
					costs[i] = COST_TRAVERSAL + COST_INTERSECTION * (count0 * bbox0.GetSurfaceArea() + count1 * bbox1.GetSurfaceArea()) / bboxAllGeoms.GetSurfaceArea();
				}

				// Now that we have the costs, we can loop through our buckets and find
				// which bucket has the lowest cost
				float minCost = costs[0];
				int minCostBucket = 0;
				for (int i = 1; i < NUM_BUCKET - 1; i++) {
					if (costs[i] < minCost) {
						minCost = costs[i];
						minCostBucket = i;
					}
				}

				// Either create a leaf or split
				float leafCost = numPrimitives;
				if (numPrimitives > m_maxGeomsInNode || minCost < leafCost) {
					// Split node
					//std::partition()

				}
			}
			break;

		case EqualCounts: // Partition based on equal counts
		default:
			PartitionEqualCounts(dim, first, last, mid, geomInfos);
			break;
	}

	// Build near child
	BBox::EAxis splitAxis = static_cast<BBox::EAxis>(dim);;
	SBVHNode* nearChild = BuildRecursive(geomInfos, first, mid, nodeCount, orderedGeoms);

	// Build far child
	SBVHNode* farChild = BuildRecursive(geomInfos, mid, last, nodeCount, orderedGeoms);

	SBVHNode* node = new SBVHNode(nearChild, farChild, nodeCount, splitAxis);
	nodeCount++;
	return node;
}


Intersection SBVH::GetIntersection(const Ray& r) 
{
	float nearestT = INFINITY;
	Intersection nearestIsx; 

	if (m_root->IsLeaf()) {
		SBVHLeaf* leaf = dynamic_cast<SBVHLeaf*>(m_root);
		for (int i = 0; i < leaf->m_numGeoms; i++)
		{
			std::shared_ptr<Geometry> geom = m_geoms[leaf->m_firstGeomOffset + i];
			Intersection isx = geom->GetIntersection(r);
			if (isx.t > 0 && isx.t < nearestT)
			{
				nearestT = isx.t;
				nearestIsx = isx;
			}
		}
		return nearestIsx;
	}

	// Traverse children
	GetIntersectionRecursive(r, m_root->m_nearChild, nearestT, nearestIsx);
	GetIntersectionRecursive(r, m_root->m_farChild, nearestT, nearestIsx);

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
		for (int i = 0; i < leaf->m_numGeoms; i++)
		{
			std::shared_ptr<Geometry> geom = m_geoms[leaf->m_firstGeomOffset + i];
			Intersection isx = geom->GetIntersection(r);
			if (isx.t > 0 && isx.t < nearestT)
			{
				nearestT = isx.t;
				nearestIsx = isx;
			}
		}
		return;
	}

	if (node->m_bbox.DoesIntersect(r))
	{
		// Traverse children
		GetIntersectionRecursive(r, node->m_nearChild, nearestT, nearestIsx);
		GetIntersectionRecursive(r, node->m_farChild, nearestT, nearestIsx);
	}
}


bool SBVH::DoesIntersect(
	const Ray& r
	)
{
	if (m_root->IsLeaf())
	{
		SBVHLeaf* leaf = dynamic_cast<SBVHLeaf*>(m_root);

		for (int i = 0; i < leaf->m_numGeoms; i++)
		{
			std::shared_ptr<Geometry> geom = m_geoms[leaf->m_firstGeomOffset + i];
			Intersection isx = geom->GetIntersection(r);
			if (isx.t > 0)
			{
				return true;
			}
		}
		return false;
	}

	// Traverse children
	return DoesIntersectRecursive(r, m_root->m_nearChild) || DoesIntersectRecursive(r, m_root->m_farChild);
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
		for (int i = 0; i < leaf->m_numGeoms; i++)
		{
			std::shared_ptr<Geometry> geom = m_geoms[leaf->m_firstGeomOffset + i];
			Intersection isx = geom->GetIntersection(r);
			if (isx.t > 0)
			{
				return true;
			}
		}

		return false;
	}

	if (node->m_bbox.DoesIntersect(r))
	{
		// Traverse children
		return DoesIntersectRecursive(r, node->m_nearChild) || DoesIntersectRecursive(r, node->m_farChild);
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

	DestroyRecursive(node->m_nearChild);
	DestroyRecursive(node->m_farChild);

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
	FlattenRecursive(node->m_nearChild);
	FlattenRecursive(node->m_farChild);
}

void SBVH::Flatten() {

	FlattenRecursive(m_root);
}

