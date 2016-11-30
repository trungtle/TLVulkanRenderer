#pragma once

#include <vulkan/vulkan.h>
#include <vector>

/**
* \brief Struct to store available swapchain support
*/


struct Swapchain
{
	/**
	* \brief Abstraction for an array of images (VkImage) to be presented to the screen surface.
	*		  Typically, one image is presented at a time while multiple others can be queued.
	* \ref https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/xhtml/vkspec.html#_wsi_swapchain
	* \seealso VkImage
	*/

	VkSwapchainKHR swapchain;

	/**
	* \brief Array of images queued in the swapchain
	*/
	std::vector<VkImage> images;

	/**
	* \brief Image format inside swapchain
	*/
	VkFormat imageFormat;

	/**
	* \brief Image extent inside swapchain
	*/
	VkExtent2D extent;

	/**
	 * \brief extent's width / height
	 */
	float aspectRatio;

	/**
	* \brief This is a view into the Vulkan
	* \ref https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#resources-image-views
	*/
	std::vector<VkImageView> imageViews;

	/**
	* \brief Swapchain framebuffers for each image view
	*/
	std::vector<VkFramebuffer> framebuffers;


	struct SwapchainSupport
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		std::vector<VkPresentModeKHR> presentModes;

		bool IsComplete() const {
			return !surfaceFormats.empty() && !presentModes.empty();
		}
	};

	static
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
	static 
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
	static 
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
	static 
	VkExtent2D
	SelectDesiredSwapchainExtent(
		const VkSurfaceCapabilitiesKHR surfaceCapabilities
		, bool useCurrentExtent = true
		, unsigned int desiredWidth = 0 /* unused if useCurrentExtent is true */
		, unsigned int desiredHeight = 0 /* unused if useCurrentExtent is true */
	);
};


