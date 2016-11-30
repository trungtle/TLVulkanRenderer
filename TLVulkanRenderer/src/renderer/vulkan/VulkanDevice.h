#pragma once

#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanSwapchain.h"
#include "VulkanUtil.h"
#include <GLFW/glfw3.h>
#include <spdlog/logger.h>
#include "VulkanImage.h"

class VulkanDevice {

public:

	VulkanDevice(
		GLFWwindow* window,
		std::string name,
		std::shared_ptr<spdlog::logger> logger
	) :
		isEnableValidationLayers(true),
		debugCallback(nullptr),
		instance(nullptr),
		surfaceKHR(nullptr),
		physicalDevice(nullptr),
		device(nullptr),
		m_name(name),
		m_logger(logger) {
		Initialize(window);
	};

	~VulkanDevice() {
		Destroy();
	}

	/**
	* \brief If true, will include the validation layer
	*/
	bool isEnableValidationLayers;

	/**
	* \brief This is the callback for the debug report in the Vulkan validation extension
	*/
	VkDebugReportCallbackEXT debugCallback;

	/**
	* \brief Handle to the per-application Vulkan instance.
	*		 There is no global state in Vulkan, and the instance represents per-application state.
	*		 Creating an instance also initializes the Vulkan library.
	* \ref https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/xhtml/vkspec.html#initialization-instances
	*/
	VkInstance instance;

	/**
	* \brief Abstract for native platform surface or window object
	* \ref https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/xhtml/vkspec.html#_wsi_surface
	*/
	VkSurfaceKHR surfaceKHR;

	/**
	* \brief Handle to the actual GPU, or physical device. It is used for informational purposes only and maynot affect the Vulkan state itself.
	* \ref https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/xhtml/vkspec.html#devsandqueues-physical-device-enumeration
	*/
	VkPhysicalDevice physicalDevice;

	/**
	* \brief In Vulkan, this is called a logical device. It represents the logical connection
	*		  to physical device and how the application sees it.
	* \ref https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/xhtml/vkspec.html#devsandqueues-devices
	*/
	VkDevice device;

	Swapchain m_swapchain;

	VulkanImage::Image m_depthTexture;

	/**
	* \brief A struct to store queue family indices
	*/
	struct QueueFamilyIndices {
		int graphicsFamily = -1;
		int presentFamily = -1;
		int computeFamily = -1;
		int transferFamily = -1;

		bool IsComplete() const {
			return graphicsFamily >= 0 && presentFamily >= 0 && computeFamily >= 0 && transferFamily >= 0;
		}
	};

	/**
	* \brief Store the queue family indices
	*/
	QueueFamilyIndices queueFamilyIndices;


	// ================================================
	// Member functions
	// ================================================

	void
	Initialize(
		GLFWwindow* window
	);

	void
	Destroy();

	VkResult
	PrepareDepthResources(
		const VkQueue& queue,
		const VkCommandPool& commandPool
	);

	VkResult
	PrepareFramebuffers(
		const VkRenderPass& renderpass
	);

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
	MapMemory(
		void* data,
		VkDeviceMemory& memory,
		VkDeviceSize size,
		VkDeviceSize offset
	);

	void
	CreateBufferAndMemory(
		const VkDeviceSize size,
		const VkBufferUsageFlags usage,
		const VkMemoryPropertyFlags memoryProperties,
		VkBuffer& buffer,
		VkDeviceMemory& memory
	) const;

	void
	CopyBuffer(
		VkQueue queue,
		VkCommandPool commandPool,
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
		VkQueue queue,
		VkCommandPool commandPool,
		VkImage image,
		VkFormat format,
		VkImageAspectFlags aspectMask,
		VkImageLayout oldLayout,
		VkImageLayout newLayout
	);

	void
	CopyImage(
		VkQueue queue,
		VkCommandPool commandPool,
		VkImage dstImage,
		VkImage srcImage,
		uint32_t width,
		uint32_t height
	);

	// ================================================
	// Class functions
	// ================================================

	/**
	* \brief Check if this GPU is Vulkan compatible
	* \param VkPhysicalDevice to inspect
	* \return true if the GPU supports Vulkan
	*/
	static bool
	IsDeviceVulkanCompatible(
		const VkPhysicalDevice& physicalDeivce
		, const VkSurfaceKHR& surfaceKHR // For finding queue that can present image to our surface
	);

private:
	/**
	* \brief Name of the Vulkan application. This is the name of our whole application in general.
	*/
	std::string m_name;

	std::shared_ptr<spdlog::logger> m_logger;

	VkResult
	InitializeVulkanInstance();

	VkResult
	SetupDebugCallback();

	VkResult
	CreateWindowSurface(GLFWwindow* window);

	VkResult
	SelectPhysicalDevice();

	VkResult
	SetupLogicalDevice();

	VkResult
	PrepareSwapchain();

	VkCommandBuffer
	BeginSingleTimeCommands(
		VkCommandPool commandPool
	) const;

	void
	EndSingleTimeCommands(
		VkQueue queue,
		VkCommandPool commandPool,
		VkCommandBuffer commandBuffer
	) const;

};


bool
CheckValidationLayerSupport(
	const std::vector<const char*>& validationLayers
);

std::vector<const char*>
GetInstanceRequiredExtensions(
	bool enableValidationLayers
);

std::vector<const char*>
GetDeviceRequiredExtensions(
	const VkPhysicalDevice& physicalDevice
);

VulkanDevice::QueueFamilyIndices
FindQueueFamilyIndices(
	const VkPhysicalDevice& physicalDevicece
	, const VkSurfaceKHR& surfaceKHR // For finding queue that can present image to our surface
);
