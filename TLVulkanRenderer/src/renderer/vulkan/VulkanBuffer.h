#pragma once

#include <vulkan.h>
#include "scene/SceneUtil.h"
#include <map>

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
		);;

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
		std::map<EVertexAttribute, VkDeviceSize> offsets;

		StorageBuffer storageBuffer;

		void Create(
			const VulkanDevice* device,
			VertexData* meshData
		);
	};
}
