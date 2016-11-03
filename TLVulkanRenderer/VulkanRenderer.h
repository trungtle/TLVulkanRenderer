/**
 * The basic of this is heavily referenced at:
 *  - Majority of this application was modified from Vulkan Tutorial(https://vulkan-tutorial.com/) by Alexander Overvoorde. [Github](https://github.com/Overv/VulkanTutorial). 
 *  - WSI Tutorial by Chris Hebert.
 *  - https://github.com/SaschaWillems/Vulkan by Sascha Willems.
 */

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include "thirdparty/spdlog/spdlog.h"
#include "Renderer.h"
#include "VulkanUtil.h"

using namespace tlVulkanUtil;

// ===================
// VULKAN RENDERER
// ===================

/**
 * \brief 
 */
class VulkanRenderer : public Renderer
{
public:
	VulkanRenderer(
		GLFWwindow* window,
		Scene* scene
		);
	virtual 
    ~VulkanRenderer() final;

	virtual void
	Update() final;

    virtual void
    Render() final;

private:
	VkResult 
	CreateInstance();
	
	VkResult 
	SetupDebugCallback();
	
	VkResult
	CreateWindowSurface();

	VkResult
	SelectPhysicalDevice();
	
	VkResult
	SetupLogicalDevice();

	VkResult
	CreateSwapchain();

    VkResult
    CreateImageViews();

	VkResult
	CreateRenderPass();

	VkResult
	CreateDescriptorSetLayout();

	/**
	 * \brief The graphics pipeline are often fixed. Create a new pipeline if we need a different pipeline settings
	 * \return 
	 */
	VkResult
	CreateGraphicsPipeline();

	VkResult
	CreateShaderModule(
		const std::vector<char>& code
		, VkShaderModule& shaderModule
		);

	VkResult
	CreateFramebuffers();

	/**
	 * \brief Vulkan commands are created in advance and submitted to the queue, 
	 *        instead of using direct function calls.
	 * \return 
	 */
	VkResult
	CreateCommandPool();

	VkResult
	CreateDepthResources();

	VkResult
	CreateVertexBuffer();

	VkResult
	CreateUniformBuffer();

	VkResult
	CreateDescriptorPool();

	VkResult
	CreateDescriptorSet();

	VkResult
	CreateCommandBuffers();

	VkResult
	CreateSemaphores();

	/**
	* \brief Helper to determine the memory type to allocate from our graphics card
	* \param typeFilter
	* \param propertyFlags
	* \return
	*/
	uint32_t
	GetMemoryType(
		uint32_t typeFilter
		, VkMemoryPropertyFlags propertyFlags
	) const;


	void
	CreateBuffer(
		const VkDeviceSize size,
		const VkBufferUsageFlags usage,
		VkBuffer& buffer
	) const;

	void
	CreateMemory(
		const VkMemoryPropertyFlags memoryProperties,
		const VkBuffer& buffer,
		VkDeviceMemory& memory
	) const;

	void
	CopyBuffer(
		VkBuffer dstBuffer,
		VkBuffer srcBuffer,
		VkDeviceSize size
	) const;

	void
	CreateImage(
		uint32_t width,
		uint32_t height,
		uint32_t depth,
		VkImageType imageType,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory& imageMemory
	);

	void
	CreateImageView(
		const VkImage& image,
		VkImageViewType viewType,
		VkFormat format,
		VkImageAspectFlags aspectFlags,
		VkImageView& imageView
	);

	void
	TransitionImageLayout(
		VkImage image,
		VkFormat format,
		VkImageAspectFlags aspectMask,
		VkImageLayout oldLayout,
		VkImageLayout newLayout
		);

	void
	CopyImage(
		VkImage dstImage,
		VkImage srcImage,
		uint32_t width,
		uint32_t height
		);

	VkCommandBuffer 
	BeginSingleTimeCommands() const;

	void 
	EndSingleTimeCommands(
		VkCommandBuffer commandBuffer
		) const;

	/**
	* \brief Handle to the per-application Vulkan instance. 
	*		 There is no global state in Vulkan, and the instance represents per-application state.
	*		 Creating an instance also initializes the Vulkan library.
	* \ref https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/xhtml/vkspec.html#initialization-instances
	*/
	VkInstance m_instance;

	/**
	 * \brief This is the callback for the debug report in the Vulkan validation extension
	 */
	VkDebugReportCallbackEXT m_debugCallback;
	
	/**
	 * \brief Abstract for native platform surface or window object
	 * \ref https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/xhtml/vkspec.html#_wsi_surface
	 */
	VkSurfaceKHR m_surfaceKHR;

	/**
	 * \brief Handle to the actual GPU, or physical device. It is used for informational purposes only and maynot affect the Vulkan state itself.
	 * \ref https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/xhtml/vkspec.html#devsandqueues-physical-device-enumeration
	 */
	VkPhysicalDevice m_physicalDevice;

	/**
	 * \brief In Vulkan, this is called a logical device. It represents the logical connection
	 *		  to physical device and how the application sees it.
	 * \ref https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/xhtml/vkspec.html#devsandqueues-devices
	 */
	VkDevice m_device;

	/**
	* \brief Handles to the Vulkan graphics queue. This may or may not be the same as the present queue
	* \ref https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/xhtml/vkspec.html#devsandqueues-queues
	*/
	VkQueue m_graphicsQueue;

	/**
	* \brief Handles to the Vulkan present queue. This may or may not be the same as the graphics queue
	* \ref https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/xhtml/vkspec.html#devsandqueues-queues
	*/
	VkQueue m_presentQueue;

	/**
	 * \brief Abstraction for an array of images (VkImage) to be presented to the screen surface. 
	 *		  Typically, one image is presented at a time while multiple others can be queued.
	 * \ref https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/xhtml/vkspec.html#_wsi_swapchain
	 * \seealso VkImage
	 */
	VkSwapchainKHR m_swapchain;

    /**
     * \brief Array of images queued in the swapchain
     */
    std::vector<VkImage> m_swapchainImages;

    /**
     * \brief Image format inside swapchain
     */
    VkFormat m_swapchainImageFormat;

    /**
     * \brief Image extent inside swapchain
     */
    VkExtent2D m_swapchainExtent;

	/**
	* \brief This is a view into the Vulkan
	* \ref https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#resources-image-views
	*/
	std::vector<VkImageView> m_swapchainImageViews;

	/**
	* \brief Swapchain framebuffers for each image view
	*/
	std::vector<VkFramebuffer> m_swapchainFramebuffers;

	/**
	* \brief Name of the Vulkan application. This is the name of our whole application in general.
	*/
	string m_name;

	/**
	* \brief If true, will include the validation layer
	*/
	bool m_isEnableValidationLayers;

	/**
	 * \brief Store the queue family indices
	 */
	QueueFamilyIndices m_queueFamilyIndices;

	/**
	 * \brief Descriptor set layout to decribe our resource binding (ex. UBO)
	 */
	VkDescriptorSetLayout m_descriptorSetLayout;

	/**
	 * \brief Descriptor pool for our resources
	 */
	VkDescriptorPool m_descriptorPool;

	/**
	 * \brief Descriptor set for our resources
	 */
	VkDescriptorSet m_descriptorSet;

	/**
	 * \brief This describes the uniforms inside shaders
	 */
	VkPipelineLayout m_pipelineLayout;

	/**
	 * \brief Holds the renderpass object. This also represents the framebuffer attachments
	 */
	VkRenderPass m_renderPass;

	VkImage m_depthImage;
	VkImageView m_depthImageView;
	VkDeviceMemory m_depthImageMemory;

	/**
	 * \brief Handle to the vertex buffers
	 */
	VkBuffer m_vertexBuffer;

	/**
	 * \brief Byte offsets for vertex attributes and resource buffers into our unified buffer
	 */
	BufferLayout m_bufferLayout;

	/**
	 * \brief Handle to the device memory
	 */
	VkDeviceMemory m_vertexBufferMemory;

	/**
	 * \brief Uniform buffers
	 */
	VkBuffer m_uniformStagingBuffer;
	VkBuffer m_uniformBuffer;
	VkDeviceMemory m_uniformStagingBufferMemory;
	VkDeviceMemory m_uniformBufferMemory;

	/**
	 * \brief Graphics pipeline
	 */
	VkPipeline m_graphicsPipeline;

	/**
	 * \brief Command pool
	 */
	VkCommandPool m_graphicsCommandPool;

	/**
	 * \brief Command buffers to record our commands
	 */
	std::vector<VkCommandBuffer> m_commandBuffers;

	/**
	 * \brief Semaphores to signal when to acquire and present swapchain images
	 */
	VkSemaphore m_imageAvailableSemaphore;
	VkSemaphore m_renderFinishedSemaphore;

    /**
     * \brief Logger
     */
    std::shared_ptr<spdlog::logger> m_logger;
};


