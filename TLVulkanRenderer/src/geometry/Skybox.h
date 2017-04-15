#pragma once
#include "Cube.h"
#include "renderer/vulkan/VulkanBuffer.h"

class Skybox {
	
public:

	Skybox() : m_device(nullptr) {};

	Skybox(VulkanDevice* device, std::string filename): m_device(nullptr) {

		std::vector<uint16_t> indices;
		std::vector<SPolygonVertexLayout> vertices;
		Cube box(vec3(0), vec3(1000));
		box.GenerateVertices(0, indices, vertices);
		m_buffer.CreatePolygon(
			device,
			indices,
			vertices
		);

		// Load skybox
		m_cubemap.CreateCubemapFromFile(
			device,
			filename,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_IMAGE_USAGE_SAMPLED_BIT
		);
	}

	~Skybox() {
		m_buffer.Destroy();
	}

	inline VulkanImage::Image& Cubemap() {
		return m_cubemap; 
	}

	inline VulkanBuffer::VertexBuffer& Buffer() {
		return m_buffer;
	}

private:
	VulkanDevice* m_device;
	VulkanBuffer::VertexBuffer m_buffer;
	VulkanImage::Image m_cubemap;
};
