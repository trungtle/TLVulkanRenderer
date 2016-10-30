/**
 * The basic of this is heavily referenced at:
 *  - Majority of this application was modified from Vulkan Tutorial(https://vulkan-tutorial.com/) by Alexander Overvoorde. [Github](https://github.com/Overv/VulkanTutorial). 
 *  - WSI Tutorial by Chris Hebert.
 *  - https://github.com/SaschaWillems/Vulkan by Sascha Willems.
 */

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <array>
#include "thirdparty/spdlog/spdlog.h"
#include "Renderer.h"

 // ===================
 // QUEUE
 // ===================

/**
 * \brief A struct to store queue family indices
 */
typedef struct QueueFamilyIndicesTyp {
	int graphicsFamily = -1;
	int presentFamily = -1;

	bool IsComplete() const {
		return graphicsFamily >= 0 && presentFamily >= 0;
	}
} QueueFamilyIndices;

// ===================
// SWAPCHAIN SUPPORT
// ===================

/**
 * \brief Struct to store available swapchain support
 */
typedef struct SwapchainSupportTyp {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentModes;

	bool IsComplete() const {
		return !surfaceFormats.empty() && !presentModes.empty();
	}
} SwapchainSupport;

// ===================
// VERTEX
// ===================

typedef struct VertexInputBindingDescriptionsTyp
{

	VkVertexInputBindingDescription position;
	VkVertexInputBindingDescription normal;
	VkVertexInputBindingDescription texcoord;

	std::array<VkVertexInputBindingDescription, 3>
		ToArray() const
	{
		std::array<VkVertexInputBindingDescription, 3> bindingDesc = {
			position,
			normal,
			texcoord
		};

		return bindingDesc;
	}

} VertexInputBindingDescriptions;

typedef struct VertexAttributeDescriptionsTyp {

	VkVertexInputAttributeDescription position;
	VkVertexInputAttributeDescription normal;
	VkVertexInputAttributeDescription texcoord;

	std::array<VkVertexInputAttributeDescription, 3>
	ToArray() const 
	{
		std::array<VkVertexInputAttributeDescription, 3> attribDesc = {
			position,
			normal,
			texcoord
		};

		return attribDesc;
	}

} VertexAttributeDescriptions;

namespace TLVertex {
	static
		std::array<VkVertexInputBindingDescription, 3>
		GetVertexInputBindingDescription(const VertexAttributes& vertexAttrib)
	{
		VertexInputBindingDescriptions bindingDesc;
		bindingDesc.position.binding = 0; // Which index of the array of VkBuffer for vertices
		bindingDesc.position.stride = vertexAttrib.positionByteStride;
		bindingDesc.position.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		bindingDesc.normal.binding = 1; // Which index of the array of VkBuffer for vertices
		bindingDesc.normal.stride = vertexAttrib.normalByteStride;
		bindingDesc.normal.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		bindingDesc.texcoord.binding = 0; // Which index of the array of VkBuffer for vertices
		bindingDesc.texcoord.stride = vertexAttrib.positionByteStride; vertexAttrib.texcoordByteStride;
		bindingDesc.texcoord.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDesc.ToArray();
	}

	static
		std::array<VkVertexInputAttributeDescription, 3>
		GetAttributeDescriptions(const VertexAttributes& vertexAttrib) 
	{
		VertexAttributeDescriptions attributeDesc;
		
		// Position attribute
		attributeDesc.position.binding = 0;
		attributeDesc.position.location = 0;
		attributeDesc.position.format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDesc.position.offset = vertexAttrib.positionByteOffset;

		// Normal attribute
		attributeDesc.normal.binding = 1;
		attributeDesc.normal.location = 1;
		attributeDesc.normal.format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDesc.normal.offset = vertexAttrib.normalByteOffset;

		// Color attribute
		attributeDesc.texcoord.binding = 0;
		attributeDesc.texcoord.location = 2;
		attributeDesc.texcoord.format = VK_FORMAT_R32G32_SFLOAT;
		attributeDesc.texcoord.offset = vertexAttrib.texcoordByteOffset;

		return attributeDesc.ToArray();
	}
};

// ===================
// UNIFORM
// ===================

typedef struct UniformBufferObjectTyp {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
} UniformBufferObject;

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
	CreateVertexBuffer();

	VkResult
	CreateVertexNormalBuffer();

	VkResult
	CreateIndexBuffer();

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
		const VkMemoryRequirements memoryRequirements,
		const VkMemoryPropertyFlags memoryProperties,
		VkDeviceMemory& memory
	) const;

	void
	CopyBuffer(
		VkBuffer dstBuffer,
		VkBuffer srcBuffer,
		VkDeviceSize size
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

	/**
	 * \brief Handle to the vertex buffers
	 */
	VkBuffer m_vertexBuffer;
	VkBuffer m_vertexNormalBuffer;
	VkBuffer m_indexBuffer;

	/**
	 * \brief Handle to the device memory
	 */
	VkDeviceMemory m_vertexBufferMemory;
	VkDeviceMemory m_vertexNormalBufferMemory;
	VkDeviceMemory m_indexBufferMemory;

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

bool 
CheckValidationLayerSupport(
	const vector<const char*>& validationLayers
	);

std::vector<const char*> 
GetInstanceRequiredExtensions(
	bool enableValidationLayers
	);

std::vector<const char*>
GetDeviceRequiredExtensions(
	const VkPhysicalDevice& physicalDevice
	);


/**
 * \brief Check if this GPU is Vulkan compatible
 * \param VkPhysicalDevice to inspect
 * \return true if the GPU supports Vulkan
 */
bool IsDeviceVulkanCompatible(
	const VkPhysicalDevice& physicalDeivce
	, const VkSurfaceKHR& surfaceKHR // For finding queue that can present image to our surface
);

QueueFamilyIndices
FindQueueFamilyIndices(
	const VkPhysicalDevice& physicalDevicece
	, const VkSurfaceKHR& surfaceKHR // For finding queue that can present image to our surface
);

SwapchainSupport
QuerySwapchainSupport(
	const VkPhysicalDevice& physicalDevice 
	, const VkSurfaceKHR& surface
	);

/**
 * \brief The surface format specifies color channel and types, and the texcoord space
 * \param availableFormats 
 * \return the VkSurfaceFormatKHR that we desire.
 * \ref https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/xhtml/vkspec.html#VkSurfaceFormatKHR

 */
VkSurfaceFormatKHR
SelectDesiredSwapchainSurfaceFormat(
	const std::vector<VkSurfaceFormatKHR> availableFormats
	);

/**
 * \brief This is the most important setting for the swap chain. This is how we select
 *		  the condition when the image gets presented to the screen. There are four modes:
 *		  VK_PRESENT_MODE_IMMEDIATE_KHR: 
 *				Image gets presented right away without vertical blanking. Tearing could occur. 
 *				No internal queuing requests required.
 *				
 *		  VK_PRESENT_MODE_MAILBOX_KHR:
 *				Waits for the next vertical blank. No tearing. 
 *				A single-entry internal queue is used to hold the pending request.
 *				It doesn't block the application when the queue is full, instead
 *				new request replaces the old one inside the queue.
 *				One request per vertical blank interval.
 *				This can be used to implement tripple buffering with less latency and no tearing
 *				than the standard vertical sync with double buffering.
 *				
 *		  VK_PRESENT_MODE_FIFO_KHR (guaranteed to be supported):
 *				Waits for the next vertical blank. No tearing. 
 *				An internal queue is used to hold the pending requests. 
 *				New requests are queued at the end. If the queue is full,
 *				this will block the application.
 *				One request per vertical blank interval.
 *				Most similar to vertical sync in modern games.
 *				
 *		  
 *		  VK_PRESENT_MODE_FIFO_RELAXED_KHR:
 *				This generally waits for a vertical blank. But if one has occurs
 *				and the image arrives late, then it will present the next image right away.
 *				This mode also uses an internal queue to hold the pending requests.
 *				Use this when the images stutter.
 *				Tearing might be visible.
 *				
 * \param availablePresentModes 
 * \return the VkPresentModeKHR that we desire
 * \ref https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/xhtml/vkspec.html#VkPresentModeKHR
 */
VkPresentModeKHR
SelectDesiredSwapchainPresentMode(
	const std::vector<VkPresentModeKHR> availablePresentModes
	);

/**
 * \brief 
 * \param surfaceCapabilities 
 * \param useCurrentExtent: this should be true for most cases
 * \param desiredWidth: we can differ from the current extent if chosen so 
 * \param desiredHeight: we can differ from the current extent if chosen so 
 * \return 
 * \ref https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/xhtml/vkspec.html#VkSurfaceCapabilitiesKHR
 */
VkExtent2D
SelectDesiredSwapchainExtent(
	const VkSurfaceCapabilitiesKHR surfaceCapabilities
	, bool useCurrentExtent = true
	, unsigned int desiredWidth = 0 /* unused if useCurrentExtent is true */
	, unsigned int desiredHeight = 0 /* unused if useCurrentExtent is true */
);
