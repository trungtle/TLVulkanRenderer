#pragma once
#include "VulkanRenderer.h"
#include "VulkanBuffer.h"
#include "renderer/Film.h"

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

protected:
	VulkanImage::Image m_stagingImage;
	VulkanImage::Image m_displayImage;

	Film m_film;

	void
	PrepareResources();

	void 
	Raytrace();
};