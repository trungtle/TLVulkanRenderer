#pragma once
#include "VulkanRenderer.h"
#include "VulkanBuffer.h"

#define VERTEX_BUFFER_BIND_ID 0

// Texture properties
#define TEX_DIM 2048
#define TEX_FILTER VK_FILTER_LINEAR

// Offscreen frame buffer properties
#define FB_DIM TEX_DIM

class VulkanHybridRenderer : public VulkanRenderer
{

public:
	VulkanHybridRenderer(
		GLFWwindow* window,
		Scene* scene,
		std::shared_ptr<std::map<string, string>> config
	);

	virtual void
		Update() final;

	virtual void
		Render() final;

	virtual ~VulkanHybridRenderer() final;

	void
	Prepare() final;

	VkResult
	PreparePipelines() final;

	VkResult
	PrepareVertexBuffers() final;

	// --- Descriptor

	VkResult
	PrepareDescriptorPool() final;

	VkResult
	PrepareDescriptorLayouts() final;

	VkResult
	PrepareDescriptorSets() final;

	// --- Command buffers

	VkResult
	BuildCommandBuffers() final;

protected:

	struct SInputTextures
	{
		VulkanImage::Texture m_colorMap;
		VulkanImage::Texture m_normalMap;
	};

	struct SFrameBuffer
	{
		uint32_t width, height;
		VkFramebuffer frameBuffer;
		VulkanImage::Image position, normal, albedo;
		VulkanImage::Image depth;
		VkRenderPass renderPass;
	};

	struct SVertexShaderUniforms
	{
		glm::mat4 m_projection;
		glm::mat4 m_model;
		glm::mat4 m_view;
	};

	struct SSceneLight
	{
		glm::vec4 position;
		glm::vec3 color;
		float radius;
	};

	struct SFragShaderUniforms
	{
		SSceneLight m_lights[6];
		glm::vec4	m_viewPos;
	};

	struct Quad
	{
		std::vector<uint16_t> indices;
		std::vector<vec2> positions;
		std::vector<vec2> uvs;
	} m_quad;

	void
	CreateAttachment(
		VkFormat format,
		VkImageUsageFlagBits usage,
		VulkanImage::Image& attachment
	);

	VkSampler m_colorSampler;
	SInputTextures m_vulkanTextures;

	// -----------
	// WIREFRAME
	// -----------

	struct Wireframe
	{
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorSet descriptorSet;
	} m_wireframe;

	// -----------
	// ON SCREEN
	// -----------

	struct Onscreen
	{
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorSet descriptorSet;
	} m_onscreen;

	// -----------
	// DEFERRED
	// -----------

	void
		PrepareDeferredDescriptorLayout();

	void
		PrepareDeferredDescriptorSet();

	void
		PrepareDeferredUniformBuffer();

	void
		PrepareDeferredAttachments();

	void
		PrepareDeferredPipeline();

	void
		BuildDeferredCommandBuffer();

	struct Deferred
	{
		SFrameBuffer framebuffer;
		VkCommandBuffer commandBuffer;
		VkSemaphore semaphore;
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorSet descriptorSet;

		SVertexShaderUniforms mvpUnif;

		VulkanBuffer::StorageBuffer mvpUnifStorage;
		VulkanBuffer::StorageBuffer lightsUnifStorage;
	} m_deferred;



	// -----------
	// RAY TRACING
	// -----------

	void
		PrepareComputeRaytrace();

	void
		PrepareComputeRaytraceDescriptorLayout();

	void
		PrepareComputeRaytraceDescriptorSet();

	void
		PrepareComputeRaytraceCommandPool();

	void
		PrepareComputeRaytraceStorageBuffer();

	void
		PrepareComputeRaytraceUniformBuffer();

	VkResult
		PrepareComputeRaytraceTextureResources();

	VkResult
		PrepareComputeRaytracePipeline();

	VkResult
		BuildComputeRaytraceCommandBuffer();



	struct Raytrace
	{
		// -- Compute compatible queue
		VkQueue queue;
		VkFence fence;

		// -- Descriptor
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorSet descriptorSet;

		// -- Pipeline
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;

		// -- Commands
		VkCommandPool commandPool;
		VkCommandBuffer commandBuffer;

		struct
		{
			// -- Uniform buffer
			VulkanBuffer::StorageBuffer uniform;
			VulkanBuffer::StorageBuffer stagingUniform;
			VulkanBuffer::StorageBuffer materials;

			// -- Shapes buffers
			VulkanBuffer::StorageBuffer indices;
			VulkanBuffer::StorageBuffer verticePositions;
			VulkanBuffer::StorageBuffer verticeNormals;

		} buffers;

		// -- Output storage image
		VulkanImage::Image storageRaytraceImage;

		// -- Uniforms
		struct UBO
		{ // Compute shader uniform block object
			glm::vec4	m_cameraPosition;
			SSceneLight m_lights[6];
			uint32_t	m_lightCount;
			uint32_t    m_materialCount;

			// toggle flags
			uint32_t	m_isBVH = false;
			uint32_t    m_isShadows = false;
			uint32_t    m_isTransparency = false;
			uint32_t    m_isReflection = false;
			uint32_t    m_isColorByRayBounces = false;

			// Padding to be 16 bytes aligned
		} ubo;

	} m_raytrace;
};
