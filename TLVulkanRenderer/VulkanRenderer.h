/**
 * Several of the code structure here is referenced at:
 *  - https://vulkan-tutorial.com/ by Alexander Overvoorde
 *  - WSI Tutorial by Chris Hebert
 *  - https://github.com/SaschaWillems/Vulkan by Sascha Willems
 */

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <vector>
#include <string>
#include <set>
using namespace std;

/**
 * \brief 
 */
typedef struct QueueFamilyIndicesTyp {
	int graphicsFamily = -1;
	int presentFamily = -1;

	bool IsComplete() const {
		return graphicsFamily >= 0 && presentFamily >= 0;
	}
} QueueFamilyIndices;

/**
 * \brief 
 */
typedef struct SwapchainSupportTyp {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentModes;

	bool IsComplete() const {
		return !surfaceFormats.empty() && !presentModes.empty();
	}
} SwapchainSupport;

/**
 * \brief 
 */
class VulkanRenderer
{
public:
	VulkanRenderer(
		GLFWwindow* window
		);
	~VulkanRenderer();

private:
	VkResult 
	InitVulkan();
	
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

	void
	Render();

	/**
	* \brief The window handle from glfw
	*/
	GLFWwindow* m_window;

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
 * \brief The surface format specifies color channel and types, and the color space
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