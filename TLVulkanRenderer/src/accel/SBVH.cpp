#include "SBVH.h"
#include <algorithm>
#include <iostream>
#include <unordered_set>

// This comparator is used to sort bvh nodes based on its centroid's maximum extent
struct CompareCentroid
{
	CompareCentroid(Dim dim) : dim(dim) {};

	Dim dim;
	bool operator()(const PrimInfo node1, PrimInfo node2) const {
		return node1.bbox.m_centroid[dim] < node2.bbox.m_centroid[dim];
	}
};

void 
SBVH::Build(
	std::vector<std::shared_ptr<Geometry>>& prims
)
{
	m_prims = prims;
	if (prims.size() == 0) {
		return;
	}

	// Initialize primitives info
	std::vector<PrimInfo> primInfos(size_t(prims.size() * 1.2), {INVALID_ID, BBox()}); // Pad 20% more space for spatial split fragments
	for (size_t i = 0; i < m_prims.size(); i++)
	{
		primInfos[i] = { i, m_prims[i]->GetBBox()};
	}

	PrimID totalNodes = 0;

	std::vector<std::shared_ptr<Geometry>> orderedGeoms;
	PrimID first = 0;
	PrimID last = primInfos.size();
	m_root = BuildRecursive(first, last, totalNodes, primInfos, orderedGeoms, 0, true);
	//m_geoms.swap(orderedGeoms);
	Flatten();
}

void SBVH::PartitionEqualCounts(
	Dim dim, 
	PrimID first, 
	PrimID last,
	PrimID& mid,
	std::vector<PrimInfo>& geomInfos) const 
{
	// Partial sorting along the maximum extent and split at the middle
	// Sort evenly to each half
	mid = (first + last) / 2;
	std::nth_element(&geomInfos[first], &geomInfos[mid], &geomInfos[last - 1] + 1, CompareCentroid(dim));

}

void SBVH::PartitionObjects(
	Cost minCostBucket,
	Dim dim,
	PrimID first, 
	PrimID last, 
	PrimID& mid,
	std::vector<PrimInfo>& primInfos, 
	BBox& bboxCentroids
	) {
	
	std::vector<PrimInfo> temp;
	for (PrimID i = first; i < last; i++) {
		if (primInfos[i].primitiveId == INVALID_ID) continue;

		temp.push_back(primInfos.at(i));
		// Clear out that memory
		primInfos.at(i) = { INVALID_ID, BBox() };
	}

	PrimID front = first;
	PrimID back = last - 1;
	for (PrimInfo info : temp) {
		// Partition geometry into two halves, before and after the split, and leave the middle empty
		int whichBucket = NUM_BUCKET * bboxCentroids.Offset(info.bbox.m_centroid)[dim];
		assert(whichBucket <= NUM_BUCKET);
		if (whichBucket == NUM_BUCKET) whichBucket = NUM_BUCKET - 1;
		if(whichBucket <= minCostBucket) {
			primInfos.at(front) = info;
			front++;
		} else {
			primInfos.at(back) = info;
			back--;
		}
		assert(front <= back + 1);

	}

	mid = (front + back) / 2;

	//PrimInfo *pmid = std::partition(
	//	&geomInfos[first], &geomInfos[last - 1] + 1,
	//	[=](const PrimInfo& gi)
	//{
	//	// Partition geometry into two halves, before and after the split
	//	int whichBucket = NUM_BUCKET * bboxCentroids.Offset(gi.bbox.m_centroid)[dim];
	//	assert(whichBucket <= NUM_BUCKET);
	//	if (whichBucket == NUM_BUCKET) whichBucket = NUM_BUCKET - 1;
	//	return whichBucket <= minCostBucket;
	//});
	//mid = pmid - &geomInfos[0];
}

void SBVH::PartitionSpatial(
	BucketID minCostBucket,
	Dim dim,
	PrimID first,
	PrimID last,
	PrimID& mid,
	std::vector<PrimInfo>& primInfos,
	BBox& bboxAllGeoms
	) 
{
	std::vector<PrimInfo> temp;
	for (PrimID i = first; i < last; i++)
	{
		if (primInfos[i].primitiveId == INVALID_ID) continue;

		temp.push_back(primInfos.at(i));
		// Clear out that memory
		primInfos.at(i) = { INVALID_ID, BBox() };
	}

	PrimID front = first;
	PrimID back = last - 1;
	for (PrimInfo info : temp)
	{
		// Partition geometry into two halves, before and after the split, and leave the middle empty
		BucketID startEdgeBucket = NUM_BUCKET * bboxAllGeoms.Offset(info.bbox.m_min)[dim];
		assert(startEdgeBucket <= NUM_BUCKET);
		if (startEdgeBucket == NUM_BUCKET) startEdgeBucket = NUM_BUCKET - 1;
		if (startEdgeBucket <= minCostBucket)
		{
			primInfos.at(front) = info;
			front++;
		}
		else
		{
			primInfos.at(back) = info;
			back--;
		}
		assert(front <= back);

	}

	mid = (front + back) / 2;

	//PrimInfo *pmid = std::partition(
	//	&primInfos[first], &primInfos[last - 1] + 1,
	//	[=](const PrimInfo& gi)
	//{
	//	BucketID startEdgeBucket = NUM_BUCKET * bboxAllGeoms.Offset(gi.bbox.m_min)[dim];;
	//	assert(startEdgeBucket <= NUM_BUCKET);
	//	if (startEdgeBucket == NUM_BUCKET) startEdgeBucket = NUM_BUCKET - 1;

	//	return startEdgeBucket <= minCostBucket;
	//});
	//mid = pmid - &primInfos[0];
}

SBVHLeaf* SBVH::CreateLeaf(
	SBVHNode* parent,
	PrimID first,
	PrimID last,
	PrimID& nodeCount,
	std::vector<PrimInfo>& primInfos,
	std::vector<std::shared_ptr<Geometry>>& orderedGeoms,
	BBox& bboxAllGeoms
	) {

	PrimID numPrimitives = 0;
	for (auto prim : primInfos)
	{
		if (prim.primitiveId != INVALID_ID)
		{
			++numPrimitives;
		}
		else
		{
			continue;
		}
	}

	size_t firstGeomOffset = orderedGeoms.size();
	std::unordered_set<PrimID> geomIds;
	for (int i = first; i < last; i++)
	{
		PrimID primID = primInfos.at(i).primitiveId;
		if (primID == INVALID_ID) continue;

		orderedGeoms.push_back(m_prims[primID]);
		geomIds.insert(primID);
	}
	SBVHLeaf* leaf = new SBVHLeaf(parent, nodeCount, firstGeomOffset, numPrimitives, bboxAllGeoms);
	nodeCount++;
	leaf->m_geomIds = geomIds;
	return leaf;
}

SBVHNode* SBVH::CreateNode(
	SBVHNode* parent,
	SBVHNode* nearChild,
	SBVHNode* farChild,
	PrimID& nodeCount,
	Dim dim
	) {
	SBVHNode* node = new SBVHNode(parent, nearChild, farChild, nodeCount, dim);
	nodeCount++;
	return node;
}

std::tuple<Cost, BucketID> 
SBVH::CalculateObjectSplitCost(
	Dim dim,
	PrimID first,
	PrimID last,
	std::vector<PrimInfo>& primInfos,
	BBox& bboxCentroids,
	BBox& bboxAllGeoms
	) const 
{
	BucketInfo buckets[NUM_BUCKET];
	Cost costs[NUM_BUCKET - 1];
	float invAllGeometriesSA = 1.0f / bboxAllGeoms.GetSurfaceArea();

	// For each primitive in range, determine which bucket it falls into
	for (int i = first; i < last; i++)
	{
		if (primInfos[i].primitiveId == INVALID_ID) continue;

		int whichBucket = NUM_BUCKET * bboxCentroids.Offset(primInfos.at(i).bbox.m_centroid)[dim];
		assert(whichBucket <= NUM_BUCKET);
		if (whichBucket == NUM_BUCKET) whichBucket = NUM_BUCKET - 1;

		buckets[whichBucket].count++;
		buckets[whichBucket].bbox = BBox::BBoxUnion(buckets[whichBucket].bbox, primInfos.at(i).bbox);
	}

	// Compute cost for splitting after each bucket
	for (int i = 0; i < NUM_BUCKET - 1; i++)
	{
		BBox bbox0, bbox1;
		int count0 = 0, count1 = 0;

		// Compute cost for buckets before split candidate
		for (int j = 0; j <= i; j++)
		{
			bbox0 = BBox::BBoxUnion(bbox0, buckets[j].bbox);
			count0 += buckets[j].count;
		}

		// Compute cost for buckets after split candidate
		for (int j = i + 1; j < NUM_BUCKET; j++)
		{
			bbox1 = BBox::BBoxUnion(bbox1, buckets[j].bbox);
			count1 += buckets[j].count;
		}

		costs[i] = COST_TRAVERSAL + COST_INTERSECTION * (count0 * bbox0.GetSurfaceArea() + count1 * bbox1.GetSurfaceArea()) * invAllGeometriesSA;
	}

	// Now that we have the costs, we can loop through our buckets and find
	// which bucket has the lowest cost
	Cost minCost = costs[0];
	BucketID minCostBucket = 0;
	for (int i = 1; i < NUM_BUCKET - 1; i++)
	{
		if (costs[i] < minCost)
		{
			minCost = costs[i];
			minCostBucket = i;
		}
	}

	return std::tuple<Cost, BucketID>(minCost, minCostBucket);
}

std::tuple<Cost, BucketID> 
SBVH::CalculateSpatialSplitCost(
	Dim dim, 
	PrimID first, 
	PrimID last, 
	std::vector<PrimInfo>& primInfos, 
	BBox& bboxAllGeoms,
	std::vector<BucketInfo>& buckets
	) const {

	Cost costs[NUM_BUCKET - 1];
	float invAllGeometriesSA = 1.0f / bboxAllGeoms.GetSurfaceArea();
	float bucketSize = (bboxAllGeoms.m_max[dim] - bboxAllGeoms.m_min[dim]) / NUM_BUCKET;


	// For each primitive in range, determine which bucket it falls into
	for (int i = first; i < last; i++)
	{
		if (primInfos[i].primitiveId == INVALID_ID) continue;

		BucketID minBucket = (NUM_BUCKET * bboxAllGeoms.Offset(primInfos.at(i).bbox.m_min)[dim]);
		assert(minBucket <= NUM_BUCKET);
		if (minBucket == NUM_BUCKET) minBucket = NUM_BUCKET - 1;

		BucketID maxBucket = (NUM_BUCKET * bboxAllGeoms.Offset(primInfos.at(i).bbox.m_max)[dim]);
		assert(maxBucket <= NUM_BUCKET);
		if (maxBucket == NUM_BUCKET) maxBucket = NUM_BUCKET - 1;

		buckets[minBucket].enter++;
		buckets[maxBucket].exit++;

		if (minBucket != maxBucket)
		{

			// Naively split into equal sized bboxes
			for (BucketID b = minBucket; b <= maxBucket; b++)
			{			
				BBox bbox = primInfos.at(i).bbox;
				float nearSplitPlane = b * bucketSize + bboxAllGeoms.m_min[dim];
				float farSplitPlane = nearSplitPlane + bucketSize;
				if ((bbox.m_min[dim] < nearSplitPlane && bbox.m_max[dim] < nearSplitPlane) || bbox.m_min[dim] > farSplitPlane && bbox.m_max[dim] > farSplitPlane) {
					continue;
				}
				bbox.m_min[dim] = max(bbox.m_min[dim], nearSplitPlane);
				bbox.m_max[dim] = min(bbox.m_max[dim], farSplitPlane);
				assert(bbox.m_min[dim] <= bbox.m_max[dim]);
				bbox.m_centroid = BBox::Centroid(bbox.m_min, bbox.m_max);
				bbox.m_transform = Transform(bbox.m_centroid, glm::vec3(0), bbox.m_max - bbox.m_min);
#ifdef TIGHT_BBOX
				vec3 pointOnSplittingPlane = bboxAllGeoms.m_min;
				for (auto bucket = startEdgeBucket; bucket < endEdgeBucket; bucket++)
				{
					// Get the primitive reference from geometry info, then clip it against the splitting plane
					std::shared_ptr<Geometry> pGeom = m_geoms[geomInfos[i].geometryId];

					// Cast to triangle
					Triangle* pTri = dynamic_cast<Triangle*>(pGeom.get());
					if (pTri == nullptr)
					{
						// not a triangle
						// Geometry reference completely falls inside bucket
						int whichBucket = NUM_BUCKET * bboxCentroids.Offset(geomInfos.at(i).bbox.m_centroid)[dim];
						assert(whichBucket <= NUM_BUCKET);
						if (whichBucket == NUM_BUCKET) whichBucket = NUM_BUCKET - 1;

						spatialSplitBuckets[whichBucket].count++;
						spatialSplitBuckets[whichBucket].bbox = BBox::BBoxUnion(buckets[whichBucket].bbox, geomInfos.at(i).bbox);
						geomInfos[i].straddling = false;
						break;

					}

					// Find intersection with splitting plane
					pointOnSplittingPlane[dim] = bucketSize * (bucket + 1) + bboxAllGeoms.m_min[dim];

					vec3 planeNormal;
					EAxis splittinPlaneAxis[2];
					switch (dim)
					{
						case EAxis::X:
							// Find Y and Z intersections
							planeNormal = vec3(1, 0, 0);
							splittinPlaneAxis[0] = EAxis::Y;
							splittinPlaneAxis[1] = EAxis::Z;
							break;
						case EAxis::Y:
							// Find X and Z intersections
							planeNormal = vec3(0, 1, 0);
							splittinPlaneAxis[0] = EAxis::X;
							splittinPlaneAxis[1] = EAxis::Z;
							break;
						case EAxis::Z:
							// Find Y and X intersections
							planeNormal = vec3(0, 0, 1);
							splittinPlaneAxis[0] = EAxis::X;
							splittinPlaneAxis[1] = EAxis::Y;
							break;
					}

					bool isVert0BelowPlane = dot(pTri->vert0 - pointOnSplittingPlane, planeNormal) < 0;
					bool isVert1BelowPlane = dot(pTri->vert1 - pointOnSplittingPlane, planeNormal) < 0;
					bool isVert2BelowPlane = dot(pTri->vert2 - pointOnSplittingPlane, planeNormal) < 0;

					// Assert that not all vertices lie on the same side of the splitting plane
					assert(!(isVert0BelowPlane && isVert1BelowPlane && isVert2BelowPlane));
					assert(!(!isVert0BelowPlane && !isVert1BelowPlane && !isVert2BelowPlane));

					// Use similar triangles to find the point of intersection on splitting plane
					vec3 pointAbove;
					vec3 pointsBelow[2];
					if (isVert0BelowPlane && isVert1BelowPlane)
					{
						pointAbove = pTri->vert2;
						pointsBelow[0] = pTri->vert0;
						pointsBelow[1] = pTri->vert1;
					}
					else if (isVert0BelowPlane && isVert2BelowPlane)
					{
						pointAbove = pTri->vert1;
						pointsBelow[0] = pTri->vert0;
						pointsBelow[1] = pTri->vert2;
					}
					else if (isVert1BelowPlane && isVert2BelowPlane)
					{
						pointAbove = pTri->vert0;
						pointsBelow[0] = pTri->vert1;
						pointsBelow[1] = pTri->vert2;
					}

					std::vector<vec3> isxPoints;
					for (auto pointBelow : pointsBelow)
					{
						vec3 isxPoint;
						for (auto axis = 0; axis < 2; axis++)
						{
							isxPoint[dim] = pointOnSplittingPlane[dim];
							isxPoint[splittinPlaneAxis[axis]] =
								((pointBelow[splittinPlaneAxis[axis]] - pointAbove[splittinPlaneAxis[axis]]) /
								(pointBelow[dim] - pointAbove[dim])) *
									(pointOnSplittingPlane[dim] - pointAbove[dim]) +
								pointAbove[splittinPlaneAxis[axis]];
						}
						isxPoints.push_back(isxPoint);
					}

					// Found intersection, grow tight bounding box of reference for bucket
					SBVHGeometryInfo tightBBoxGeomInfo;

					// Create additional geometry info to whole the new tight bbox
					tightBBoxGeomInfo.geometryId = geomInfos[i].geometryId;
					tightBBoxGeomInfo.bbox = BBox::BBoxFromPoints(isxPoints);
					if (pTri->vert0[dim] > pointOnSplittingPlane[dim] - bucketSize &&
						pTri->vert0[dim] < pointOnSplittingPlane[dim])
					{
						tightBBoxGeomInfo.bbox = BBox::BBoxUnion(tightBBoxGeomInfo.bbox, pTri->vert0);
					}
					if (pTri->vert1[dim] > pointOnSplittingPlane[dim] - bucketSize &&
						pTri->vert1[dim] < pointOnSplittingPlane[dim])
					{
						tightBBoxGeomInfo.bbox = BBox::BBoxUnion(tightBBoxGeomInfo.bbox, pTri->vert1);
					}
					if (pTri->vert2[dim] > pointOnSplittingPlane[dim] - bucketSize &&
						pTri->vert2[dim] < pointOnSplittingPlane[dim])
					{
						tightBBoxGeomInfo.bbox = BBox::BBoxUnion(tightBBoxGeomInfo.bbox, pTri->vert2);
					}

					geomInfos.push_back(tightBBoxGeomInfo);

					spatialSplitBuckets[bucket].bbox = BBox::BBoxUnion(tightBBoxGeomInfo.bbox, spatialSplitBuckets[bucket].bbox);
				}

				// Increment entering and exiting reference counts
				spatialSplitBuckets[startEdgeBucket].enter++;
				spatialSplitBuckets[endEdgeBucket].exit++;
#endif
				// Create a new geom info to hold the splitted geometry
				PrimInfo fragInfo;
				fragInfo.bbox = bbox;
				fragInfo.primitiveId = primInfos.at(i).primitiveId;
				fragInfo.origPrimOffset = i;
				buckets[b].fragments.push_back(fragInfo);

				buckets[b].bbox = BBox::BBoxUnion(buckets[b].bbox, bbox);
			}
		} else {
			buckets[minBucket].bbox = primInfos.at(i).bbox;
		}
	}

	// Compute cost for splitting after each bucket
	for (int i = 0; i < NUM_BUCKET - 1; i++)
	{
		BBox bbox0, bbox1;
		int count0 = 0, count1 = 0;

		// Compute cost for buckets before split candidate
		for (BucketID j = 0; j <= i; j++)
		{
			bbox0 = BBox::BBoxUnion(bbox0, buckets[j].bbox);
			count0 += buckets[j].enter;
		}

		// Compute cost for buckets after split candidate
		for (BucketID j = i + 1; j < NUM_BUCKET; j++)
		{
			bbox1 = BBox::BBoxUnion(bbox1, buckets[j].bbox);
			count1 += buckets[j].exit;
		}

		costs[i] = COST_TRAVERSAL + COST_INTERSECTION * (count0 * bbox0.GetSurfaceArea() + count1 * bbox1.GetSurfaceArea()) * invAllGeometriesSA;
	}

	// Now that we have the costs, we can loop through our buckets and find
	// which bucket has the lowest cost
	Cost minCost = costs[0];
	BucketID minCostBucket = 0;
	for (int i = 1; i < NUM_BUCKET - 1; i++)
	{
		if (costs[i] < minCost)
		{
			minCost = costs[i];
			minCostBucket = i;
		}
	}

	return std::tuple<Cost, BucketID>(minCost, minCostBucket);
}

SBVHNode*
SBVH::BuildRecursive(
	PrimID first, 
	PrimID last,
	PrimID& nodeCount,
	std::vector<PrimInfo>& primInfos,
	std::vector<std::shared_ptr<Geometry>>& orderedGeoms,
	int depth,
	bool shouldInsertAtBack
	) 
{
	if (last <= first || last < 0 || first < 0)
	{
		return nullptr;
	}

	// == COMPUTE BOUNDS OF ALL GEOMETRIES
	BBox bboxAllGeoms;
	for (int i = first; i < last; i++) {
		if (primInfos[i].primitiveId == INVALID_ID) continue;
		bboxAllGeoms = BBox::BBoxUnion(bboxAllGeoms, primInfos[i].bbox);
	}

	// Num primitive should only reflect unique IDs
	PrimID numPrimitives = 0;
	for (auto prim : primInfos) {
		if (prim.primitiveId != INVALID_ID) {
			++numPrimitives;
		} else {
			continue;
		}
	}
	
	// == GENERATE SINGLE GEOMETRY LEAF NODE
	if (numPrimitives == 1 || depth == 20) {
		return CreateLeaf(nullptr, first, last, nodeCount, primInfos, orderedGeoms, bboxAllGeoms);
	}

	// COMPUTE BOUNDS OF ALL CENTROIDS
	BBox bboxCentroids;
	for (int i = first; i < last; i++) {
		if (primInfos[i].primitiveId == INVALID_ID) continue;

		bboxCentroids = BBox::BBoxUnion(bboxCentroids, primInfos[i].bbox.m_centroid);
	}

	// Get maximum extent
	auto dim = BBox::BBoxMaximumExtent(bboxCentroids);

	// === GENERATE PLANAR LEAF NODE
	// If all centroids are the same, create leafe since there's no effective way to split the tree
	if (bboxCentroids.m_max[dim] == bboxCentroids.m_min[dim]) {
		return CreateLeaf(nullptr, first, last, nodeCount, primInfos, orderedGeoms, bboxAllGeoms);
	}

	std::tuple<Cost, BucketID> objSplitCost;

	PrimID mid;
	switch(m_splitMethod) {
		case Spatial:
			// 1. Find object split candidate
			// Restrict spatial split of crossed threshold
			// 2. Find spatial split candidate
			// 3. Select winner candidate
			if (numPrimitives <= 4)
			{
				PartitionEqualCounts(dim, first, last, mid, primInfos);
			}
			else
			{
				// Update maximum extend to be all bounding boxes, not just the centroids
				Dim allGeomsDim = BBox::BBoxMaximumExtent(bboxAllGeoms);

				const float RESTRICT_ALPHA = 1e-5;

				// === FIND OBJECT SPLIT CANDIDATE
				// For each primitive in range, determine which bucket it falls into
				objSplitCost =
					CalculateObjectSplitCost(dim, first, last, primInfos, bboxCentroids, bboxAllGeoms);

				bool isSpatialSplit = false;
				Cost minSplitCost = std::get<Cost>(objSplitCost);
				std::tuple<Cost, BucketID> spatialSplitCost;
				if (m_spatialSplitBudget > 0) {
					std::vector<BucketInfo> spatialBuckets;
					spatialBuckets.resize(NUM_BUCKET);
					spatialSplitCost =
						CalculateSpatialSplitCost(allGeomsDim, first, last, primInfos, bboxAllGeoms, spatialBuckets);

					// Get the cheapest cost between object split candidate and spatial split candidate

					if (minSplitCost > std::get<Cost>(spatialSplitCost))
					{
						minSplitCost = std::get<Cost>(spatialSplitCost);
						isSpatialSplit = true;

						// Consume budget
						m_spatialSplitBudget--;

						std::vector<PrimInfo> origPrims;
						PrimID front = first;
						PrimID back = last - 1;
						BucketID spatialBucket = std::get<BucketID>(spatialSplitCost);
						for (auto frag : spatialBuckets[spatialBucket].fragments) {
							PrimInfo origPrim = primInfos.at(frag.origPrimOffset);
							origPrims.push_back(origPrim);
						}

						// Create new fragments
						PrimID count = 0;
						for (auto frag : spatialBuckets[std::get<BucketID>(spatialSplitCost)].fragments)
						{
							PrimInfo origPrim = origPrims.at(count);
							count++;

							PrimInfo left = { frag.primitiveId, frag.bbox };
							left.bbox.m_min[allGeomsDim] = origPrim.bbox.m_min[allGeomsDim];
							assert(left.bbox.m_min[allGeomsDim] <= left.bbox.m_max[allGeomsDim]);
							left.bbox.m_centroid = BBox::Centroid(left.bbox.m_min, left.bbox.m_max);
							left.bbox.m_transform = Transform(left.bbox.m_centroid, glm::vec3(0), left.bbox.m_max - left.bbox.m_min);

							PrimInfo right = { frag.primitiveId, frag.bbox };
							right.bbox.m_max[allGeomsDim] = origPrim.bbox.m_max[allGeomsDim];
							assert(right.bbox.m_min[allGeomsDim] <= right.bbox.m_max[allGeomsDim]);
							right.bbox.m_centroid = BBox::Centroid(right.bbox.m_min, right.bbox.m_max);
							right.bbox.m_transform = Transform(right.bbox.m_centroid, glm::vec3(0), right.bbox.m_max - right.bbox.m_min);


							// Test whether we can unfit this reference if it's overlapping is too small
							// Remember that spatial split cost is :
							// COST_TRAVERSAL + COST_INTERSECTION * (count0 * bbox0.GetSurfaceArea() + count1 * bbox1.GetSurfaceArea()) * invAllGeometriesSA;

							Cost csplit = (std::get<Cost>(spatialSplitCost) - COST_TRAVERSAL) * bboxAllGeoms.GetSurfaceArea() / COST_INTERSECTION;
							Cost c0 = BBox::BBoxUnion(left.bbox, origPrim.bbox).GetSurfaceArea() * 
								spatialBuckets[spatialBucket].enter + right.bbox.GetSurfaceArea() * (spatialBuckets[spatialBucket].exit - 1);
							Cost c1 = left.bbox.GetSurfaceArea() *
								(spatialBuckets[spatialBucket].enter - 1) + BBox::BBoxUnion(right.bbox, origPrim.bbox).GetSurfaceArea() * spatialBuckets[spatialBucket].exit;

							if (csplit < c0 && csplit < c1) {
								// Insert the new fragments into the priminfos list at both children
								primInfos.at(frag.origPrimOffset) = left;
								if (shouldInsertAtBack)
								{
									primInfos[back] = right;
									--back;
								}
								else
								{
									primInfos[front] = right;
									++front;
								}
							} else if (c0 < c1) {
								
							} else {
								
							}

						}
					}
				}

				// == CREATE LEAF OR SPLIT
				float leafCost = numPrimitives * COST_INTERSECTION;
				if (numPrimitives > m_maxGeomsInNode || minSplitCost < leafCost)
				{
					// Split node

					if (isSpatialSplit) {
						PartitionSpatial(std::get<BucketID>(spatialSplitCost), allGeomsDim, first, last, mid, primInfos, bboxAllGeoms);

					} else {
						PartitionObjects(std::get<BucketID>(objSplitCost), dim, first, last, mid, primInfos, bboxCentroids);
					}
				}
				else
				{
					// == CREATE LEAF
					SBVHLeaf* leaf = CreateLeaf(
						nullptr, first, last, nodeCount, primInfos, orderedGeoms, bboxAllGeoms
					);
					return leaf;
				}
			}
			break;

		case SAH: // Partition based on surface area heuristics
			if (numPrimitives <= 4)
			{
				PartitionEqualCounts(dim, first, last, mid, primInfos);
			}
			else
			{
				objSplitCost =
					CalculateObjectSplitCost(dim, first, last, primInfos, bboxCentroids, bboxAllGeoms);

				// Either create a leaf or split
				float leafCost = numPrimitives;
				if (numPrimitives > m_maxGeomsInNode || std::get<Cost>(objSplitCost) < leafCost)
				{
					// Split node
					PartitionObjects(std::get<BucketID>(objSplitCost), dim, first, last, mid, primInfos, bboxCentroids);
				}
				else
				{
					// Cost of splitting buckets is too high, create a leaf node instead
					SBVHLeaf* leaf = CreateLeaf(
						nullptr, first, last, nodeCount, primInfos, orderedGeoms, bboxAllGeoms
					);
					return leaf;
				}
			}
			break;

		case EqualCounts: // Partition based on equal counts
		default:
			PartitionEqualCounts(dim, first, last, mid, primInfos);
			break;
	}

	// Build near child
	SBVHNode* nearChild = BuildRecursive(first, mid, nodeCount, primInfos, orderedGeoms, depth + 1, true);

	// Build far child
	SBVHNode* farChild = BuildRecursive(mid, last, nodeCount, primInfos, orderedGeoms, depth + 1, false);

	SBVHNode* node = new SBVHNode(nullptr, nearChild, farChild, nodeCount, dim);
	if (nearChild)
		nearChild->m_parent = node;
	if (farChild)
		farChild->m_parent = node;
	nodeCount++;
	return node;
}


Intersection SBVH::GetIntersection(Ray& r) 
{
	float nearestT = INFINITY;
	Intersection nearestIsx; 

	// Update ray's traversal cost for visual debugging
	r.m_traversalCost += COST_TRAVERSAL;

	if (m_root->IsLeaf()) {
		SBVHLeaf* leaf = dynamic_cast<SBVHLeaf*>(m_root);
		//for (int i = 0; i < leaf->m_numGeoms; i++)
		//{
		//	r.m_traversalCost += COST_INTERSECTION;
		//	
		//	std::shared_ptr<Geometry> geom = m_geoms[leaf->m_firstGeomOffset + i];
		//	Intersection isx = geom->GetIntersection(r);
		//	if (isx.t > 0 && isx.t < nearestT)
		//	{
		//		nearestT = isx.t;
		//		nearestIsx = isx;
		//	}
		//}
		for (auto geomId : leaf->m_geomIds) {
			r.m_traversalCost += COST_INTERSECTION;
				
			std::shared_ptr<Geometry> geom = m_prims[geomId];
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
	Ray& r, 
	SBVHNode* node, 
	float& nearestT, 
	Intersection& nearestIsx
	) 
{
	// Update ray's traversal cost for visual debugging
	r.m_traversalCost += COST_TRAVERSAL;

	if (node == nullptr) {
		return;
	}

	if (node->IsLeaf()) {
		SBVHLeaf* leaf = dynamic_cast<SBVHLeaf*>(node);
		if (leaf->m_numGeoms < 4 || node->m_bbox.DoesIntersect(r)) {
			// Return nearest primitive
			//for (int i = 0; i < leaf->m_numGeoms; i++)
			//{
			//	r.m_traversalCost += COST_INTERSECTION;

			//	std::shared_ptr<Geometry> geom = m_geoms[leaf->m_firstGeomOffset + i];
			//	Intersection isx = geom->GetIntersection(r);
			//	if (isx.t > 0 && isx.t < nearestT)
			//	{
			//		r.m_traversalCost = COST_INTERSECTION;
			//		nearestT = isx.t;
			//		nearestIsx = isx;
			//	}
			//}
			for (auto geomId : leaf->m_geomIds)
			{
				r.m_traversalCost += COST_INTERSECTION;

				std::shared_ptr<Geometry> geom = m_prims[geomId];
				Intersection isx = geom->GetIntersection(r);
				if (isx.t > 0 && isx.t < nearestT)
				{
					nearestT = isx.t;
					nearestIsx = isx;
				}

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
	Ray& r
	)
{
	if (m_root->IsLeaf())
	{
		SBVHLeaf* leaf = dynamic_cast<SBVHLeaf*>(m_root);
		if (leaf->m_numGeoms < 4 || m_root->m_bbox.DoesIntersect(r))
		{
			for (auto geomId : leaf->m_geomIds)
			{
				r.m_traversalCost += COST_INTERSECTION;

				std::shared_ptr<Geometry> geom = m_prims[geomId];
				Intersection isx = geom->GetIntersection(r);
				if (isx.t > 0)
				{
					return true;
				}

			}
		}
		return false;
	}

	// Traverse children
	return DoesIntersectRecursive(r, m_root->m_nearChild) || DoesIntersectRecursive(r, m_root->m_farChild);
}

bool SBVH::DoesIntersectRecursive(
	Ray& r, 
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
		if (leaf->m_numGeoms < 4 || node->m_bbox.DoesIntersect(r))
		{
			for (auto geomId : leaf->m_geomIds)
			{
				r.m_traversalCost += COST_INTERSECTION;

				std::shared_ptr<Geometry> geom = m_prims[geomId];
				Intersection isx = geom->GetIntersection(r);
				if (isx.t > 0)
				{
					return true;
				}

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

	if (node == nullptr) {
		return;
	}

	if (node->IsLeaf())
	{
		delete node;
		node = nullptr;
		return;
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
	std::cout << "Number of BVH nodes: " << m_nodes.size() << std::endl;
}

void SBVH::GenerateVertices(std::vector<uint16>& indices, std::vector<SWireframe>& vertices)
{
	size_t verticeCount = 0;
	vec3 color;
	for (auto node : m_nodes)
	{
		if (node->IsLeaf())
		{
			color = vec3(0, 1, 1);
		}
		else
		{
			color = vec3(1, 0, 0);
		}
		// Setup vertices
		glm::vec3 centroid = node->m_bbox.m_centroid;
		glm::vec3 translation = centroid;
		glm::vec3 scale = glm::vec3(glm::vec3(node->m_bbox.m_max) - glm::vec3(node->m_bbox.m_min));
		glm::mat4 transform = glm::translate(glm::mat4(1.0), translation) * glm::scale(glm::mat4(1.0f), scale);

		vertices.push_back(
		{
			glm::vec3(transform * glm::vec4(.5f, .5f, .5f, 1)),
			color
		});
		vertices.push_back(
		{
			glm::vec3(transform * glm::vec4(.5f, .5f, -.5f, 1)),
			color
		});
		vertices.push_back(
		{
			glm::vec3(transform * glm::vec4(.5f, -.5f, .5f, 1)),
			color
		});
		vertices.push_back(
		{
			glm::vec3(transform * glm::vec4(.5f, -.5f, -.5f, 1)),
			color
		});
		vertices.push_back(
		{
			glm::vec3(transform * glm::vec4(-.5f, .5f, .5f, 1)),
			color
		});
		vertices.push_back(
		{
			glm::vec3(transform * glm::vec4(-.5f, .5f, -.5f, 1)),
			color
		});
		vertices.push_back({
			glm::vec3(transform * glm::vec4(-.5f, -.5f, .5f, 1)),
			color
		});
		vertices.push_back({
			glm::vec3(transform * glm::vec4(-.5f, -.5f, -.5f, 1)),
			color
		});

		// Setup indices

		indices.push_back(0 + verticeCount);
		indices.push_back(1 + verticeCount);
		indices.push_back(1 + verticeCount);
		indices.push_back(3 + verticeCount);
		indices.push_back(3 + verticeCount);
		indices.push_back(2 + verticeCount);
		indices.push_back(2 + verticeCount);
		indices.push_back(0 + verticeCount);
		indices.push_back(0 + verticeCount);
		indices.push_back(4 + verticeCount);
		indices.push_back(4 + verticeCount);
		indices.push_back(6 + verticeCount);
		indices.push_back(6 + verticeCount);
		indices.push_back(2 + verticeCount);
		indices.push_back(3 + verticeCount);
		indices.push_back(7 + verticeCount);
		indices.push_back(7 + verticeCount);
		indices.push_back(6 + verticeCount);
		indices.push_back(1 + verticeCount);
		indices.push_back(5 + verticeCount);
		indices.push_back(5 + verticeCount);
		indices.push_back(4 + verticeCount);
		indices.push_back(5 + verticeCount);
		indices.push_back(7 + verticeCount);

		verticeCount += 8;

	}
}
