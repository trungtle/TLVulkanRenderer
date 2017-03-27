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

	void
		Update() final;

	void
		Render() final;

	virtual ~VulkanHybridRenderer() final;

	void
	Prepare() final;

	VkResult
	PrepareVertexBuffers() final;

	void
		PrepareTextures() final;

	// --- Descriptor

	VkResult
	PrepareDescriptorPool() final;

protected:

	struct SInputTextures
	{
		VulkanImage::Image m_colorMap;
		VulkanImage::Image m_normalMap;
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
	void
		PrepareOnscreen();

	void
		PrepareOnScreenQuadVertexBuffer();

	void
		PrepareOnscreenDescriptorLayout();

	void
		PrepareOnscreenDescriptorSet();

	void
		PrepareOnscreenUniformBuffer();

	void
		PrepareOnscreenPipeline();

	void
		BuildOnscreenCommandBuffer();

	struct Onscreen
	{
		VulkanBuffer::StorageBuffer unifBuffer;
		VulkanBuffer::VertexBuffer quadBuffer;
	} m_onscreen;

	// -----------
	// DEFERRED
	// -----------
	void
		CreateAttachment(
			VkFormat format,
			VkImageUsageFlagBits usage,
			VulkanImage::Image& attachment
		) const;



	void
		PrepareDeferred();

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

	void
		UpdateDeferredLightsUniform();


	struct SFrameBuffer
	{
		uint32_t width, height;
		VkFramebuffer vkFrameBuffer;
		VkRenderPass renderPass;
		VkSampler sampler;

		VulkanImage::Image position, normal, albedo;
		VulkanImage::Image depth;
	};

	struct Deferred
	{
		// -- Textures
		SInputTextures textures;

		// -- Framebuffer
		SFrameBuffer framebuffer;

		// -- Command
		VkCommandBuffer commandBuffer;
		VkSemaphore semaphore;

		// -- Pipeline
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;

		// -- Descriptor
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorSet descriptorSet;

		// -- Uniforms
		SVertexShaderUniforms mvpUnif;
		SFragShaderUniforms lightsUnif;

		struct {
			VulkanBuffer::StorageBuffer mvpUnifStorage;
			VulkanBuffer::StorageBuffer lightsUnifStorage;
		} buffers;
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
		PrepareComputeRaytraceStorageBuffer();

	void
		PrepareComputeRaytraceUniformBuffer();

	VkResult
		PrepareComputeRaytraceTextures();

	VkResult
		PrepareComputeRaytracePipeline();

	VkResult
		BuildComputeRaytraceCommandBuffer();

	void
		UpdateComputeRaytraceUniform();

	struct Raytrace
	{
		// -- Fence
		VkFence fence;

		// -- Descriptor
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorSet descriptorSet;

		// -- Pipeline
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;

		// -- Commands
		VkCommandBuffer commandBuffer;

		struct
		{
			VulkanBuffer::StorageBuffer uniform;
			VulkanBuffer::StorageBuffer stagingUniform;

			// -- Uniform buffer
			VulkanBuffer::StorageBuffer materials;

			// -- Shapes buffers
			VulkanBuffer::StorageBuffer indices;
			VulkanBuffer::StorageBuffer positions;
			VulkanBuffer::StorageBuffer normals;
			VulkanBuffer::StorageBuffer bvh;

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

	VulkanImage::Image m_stagingImage;
};
