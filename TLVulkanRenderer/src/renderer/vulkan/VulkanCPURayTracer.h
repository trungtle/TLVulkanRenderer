#pragma once
#include "VulkanRenderer.h"
#include "VulkanBuffer.h"
#include "renderer/Film.h"
#include <thread>

class VulkanCPURaytracer : public VulkanRenderer
{

public:
	VulkanCPURaytracer(
		GLFWwindow* window,
		Scene* scene
	);

	virtual ~VulkanCPURaytracer() final;

	void
	Update() override final;

	void
	Render() override final;

protected:

	// -----------
	// GRAPHICS PIPELINE
	// -----------
	void
		PrepareGraphics() final;

	VkResult
		PrepareGraphicsPipeline() final;

	VkResult
		PrepareGraphicsVertexBuffer() final;

	VkResult
		PrepareGraphicsUniformBuffer() final;

	// --- Descriptor

	VkResult
		PrepareGraphicsDescriptorPool() final;

	VkResult
		PrepareGraphicsDescriptorSetLayout() final;

	VkResult
		PrepareGraphicsDescriptorSets() final;

	// --- Command buffers

	VkResult
		PrepareGraphicsCommandBuffers() final;

	struct Quad
	{
		std::vector<uint16_t> indices;
		std::vector<vec2> positions;
		std::vector<vec2> uvs;
	} m_quad;


	struct SWireframe
	{
		glm::vec3 position;
		glm::vec3 color;
	};

protected:
	VulkanImage::Image m_stagingImage;
	VulkanImage::Image m_displayImage;
	VulkanBuffer::StorageBuffer m_wireframeBVHVertices;
	VulkanBuffer::StorageBuffer m_wireframeBVHIndices;
	VulkanBuffer::StorageBuffer m_wireframeUniform;
	VkDescriptorSet m_wireframeDescriptorSet;
	VkDescriptorSetLayout m_wireframeDescriptorLayout;
	VkPipeline m_wireframePipeline;
	VkPipelineLayout m_wireframePipelineLayout;
	uint32_t m_wireframeIndexCount;

	Film m_film;

	std::array<std::thread, 16> m_threads;

	void
	PrepareResources();

	void
	GenerateWireframeBVHNodes();
};
