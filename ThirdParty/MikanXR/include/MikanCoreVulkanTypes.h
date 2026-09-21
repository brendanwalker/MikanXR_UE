#pragma once

// The Vulkan client contract. Only a Vulkan client includes this header, so it is the one place
// the client SDK names a Vulkan type; it is deliberately outside the reflected wire types.

#include <vulkan/vulkan_core.h>

#include <stdint.h>

/** \brief What a Vulkan client hands Mikan_SetGraphicsDeviceInterface(MikanClientGraphicsApi_Vulkan, &iface).
 The device must be created with VK_KHR_external_memory_win32 enabled (Vulkan 1.1 core covers the rest).
 The pointer stays valid for the life of the client connection.
 */
struct MikanVulkanDeviceInterface
{
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	/// A graphics-capable queue the shared texture writer submits its copy on
	VkQueue queue;
	/// The family `queue` belongs to
	uint32_t queueFamilyIndex;
};

/** \brief What a Vulkan client passes as the texture pointer of the Write*RenderTargetTexture requests.
 The image is read in `layout` and returned to it. A depth request expects an R32_SFLOAT color image
 already holding depth, since a depth-format image cannot be copied to a color format.
 */
struct MikanVulkanTexture
{
	VkImage image;
	VkImageLayout layout;
	VkFormat format;
	uint32_t width;
	uint32_t height;
};
