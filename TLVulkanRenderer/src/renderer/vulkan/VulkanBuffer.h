#pragma once

#include <vulkan.h>
#include "scene/SceneUtil.h"
#include <map>
#include "geometry/Geometry.h"

class VulkanDevice;

namespace VulkanBuffer {
	// ===================
	// BUFFER
	// ===================

	struct StorageBuffer {
		VkBuffer buffer;
		VkDeviceMemory memory;
		VkDescriptorBufferInfo descriptor;
		const VulkanDevice* m_device;

		void Create(
			const VulkanDevice* device,
			const VkDeviceSize size,
			const VkBufferUsageFlags usage,
			const VkMemoryPropertyFlags memoryProperties
		);

		void CreateFromData(
			VulkanDevice* device,
			void* data,
			const VkDeviceSize size,
			const VkBufferUsageFlags usage,
			const VkMemoryPropertyFlags memoryProperties,
			bool isCompute = false
		);


		void Destroy(
		) const;
	};

	// ===================
	// GEOMETRIES
	// ===================
	struct VertexBuffer {
		/**
		* \brief Byte offsets for vertex attributes and resource buffers into our unified buffer
		*/
		std::map<EAttrib, VkDeviceSize> offsets;

		StorageBuffer storageBuffer;
		uint32_t indexCount;

		void Create(
			const VulkanDevice* device,
			Model* meshData
		);

		void CreateWireframe(
			const VulkanDevice* device,
			const std::vector<uint16_t>& indices,
			const std::vector<SWireframeVertexLayout>& vertices
		);

		void CreatePolygon(
			const VulkanDevice* device,
			const std::vector<uint16_t>& indices,
			const std::vector<SPolygonVertexLayout>& vertices
		);
	};
}
