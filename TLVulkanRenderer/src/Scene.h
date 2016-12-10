#pragma once

#include <map>
#include "tinygltfloader/tiny_gltf_loader.h"
#include "SceneUtil.h"

class Camera;

class Scene {
public:
	Scene(std::string fileName);
	~Scene();

	struct BVH
	{
		enum DIM
		{
			DIM_X = 0,
			DIM_Y,
			DIM_Z
		};

		struct Triangle
		{
		public:
			Triangle() {}
			Triangle(const Triangle& tr)
			{
				std::memcpy(	m_pos,		tr.m_pos, 3 * sizeof(glm::vec3));
				m_indices = tr.m_indices;
			}

			void set(const glm::vec3 pos0, const glm::vec3& pos1, const glm::vec3& pos2)
			{
				m_pos[0] = pos0; m_pos[1] = pos1; m_pos[2] = pos2;
			}

		public:

			glm::vec3	m_pos[3];
			glm::ivec3	m_indices;
		};

		struct TriDimExtent
		{
			void set(DIM dim, size_t triIdx, const Triangle& tri)
			{
				m_triIdx = triIdx;
				m_biggestExtent = glm::max(tri.m_pos[0][dim], tri.m_pos[1][dim]);
				m_biggestExtent = glm::max(m_biggestExtent, tri.m_pos[2][dim]);
			}

			static bool SortTris(TriDimExtent& rhs, TriDimExtent& lhs)
			{
				return rhs.m_biggestExtent < lhs.m_biggestExtent;
			}


			size_t	m_triIdx;
			float	m_biggestExtent;
		};

		struct BVHNode
		{
			BVHNode() {}
			BVHNode(const glm::vec4& bound0, const glm::vec4& bound1);

			void setAabb(const Triangle* tri, size_t numTris);
			void setLeftChild	(size_t aabbIdx);
			void setRightChild	(size_t aabbIdx);
			void setNumLeafChildren(size_t numChildren);
			void setAsLeafTri	(const glm::ivec3& 	triIndices);

			glm::vec4 m_minAABB; // .w := left aabb child index.
			glm::vec4 m_maxAABB; // .w := right aabb child index.
		};

		void buildBVHTree(Scene* pScene, const std::string& fileName);
		std::vector<BVHNode> m_aabbNodes;

	private:
		static int numHeaderAabbNodes;
		size_t _buildBVHTree(int depth, int maxLeafSize, const std::vector<Triangle>& tris, std::vector<Scene::BVH::BVHNode>& outNodes);

		int visit(int iCurNode, std::vector<Scene::BVH::BVHNode>& nodes);
	};
	

	Camera* camera;

	BVH						bvh;
	std::vector<MeshData*> meshesData;
	std::vector<Material> materials;
	std::vector<glm::ivec4> indices;
	std::vector<glm::vec4> verticePositions;
	std::vector<glm::vec4> verticeNormals;
};
