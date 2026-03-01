#pragma once
#ifndef __XYVK_SWAPCHAIN
#define __XYVK_SWAPCHAIN
	#include "./xyvk_engine.hpp"

	namespace XYVK_NAMESPACE {
		class xyvk_swapchain {
			public:
			/// @brief Checks the VkPhysicalDevice for swap-chain availability.
			static inline xyvk_swapchaininfo QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR presentSurface) {
				xyvk_swapchaininfo details;
				vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, presentSurface, &details.capabilities);

				uint32_t formatCount, presentModeCount;
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, presentSurface, &formatCount, VK_NULL_HANDLE);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, presentSurface, &presentModeCount, VK_NULL_HANDLE);
				
				details.formats.resize(formatCount);
				details.presentModes.resize(presentModeCount);

				vkGetPhysicalDeviceSurfaceFormatsKHR(device, presentSurface, &formatCount, (formatCount > 0)? details.formats.data() : VK_NULL_HANDLE);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, presentSurface, &presentModeCount, (presentModeCount > 0)? details.presentModes.data() : VK_NULL_HANDLE);
				return details;
			}

			/// @brief Gets the swap-chain surface format.
			static inline VkSurfaceFormatKHR QuerySwapSurfaceFormat(xyvk_surfaceinfo presentDetails, const std::vector<VkSurfaceFormatKHR>& availableFormats) {
				for (const auto& availableFormat : availableFormats)
					if (availableFormat.format == presentDetails.dataFormat && availableFormat.colorSpace == presentDetails.colorSpace)
						return availableFormat;
				return availableFormats[0];
			}

			/// @brief Select the swap-chains active present mode.
			static inline VkPresentModeKHR QuerySwapPresentMode(xyvk_surfaceinfo presentDetails, const std::vector<VkPresentModeKHR>& availablePresentModes) {
				for (const auto& availablePresentMode : availablePresentModes)
					if (availablePresentMode == presentDetails.idealPresentMode)
						return availablePresentMode;
				return VK_PRESENT_MODE_FIFO_KHR;
			}

			/// @brief Select swap-chain extent (swap-chain surface resolution).
			static inline VkExtent2D QuerySwapExtent(xyvk_window& window, const VkSurfaceCapabilitiesKHR& capabilities) {
				int width, height;
				window.OnFrameBufferReSizeCallback(width, height);

				VkExtent2D extent = {
					std::min(std::max((uint32_t)width, capabilities.minImageExtent.width), capabilities.maxImageExtent.width),
					std::min(std::max((uint32_t)height, capabilities.minImageExtent.height), capabilities.maxImageExtent.height)
				};
				return { std::max(1u, extent.width), std::max(1u, extent.height) };
			}

			/// @brief Acquires the next image from the swap chain and returns out that image index.
			static inline VkResult QueryNextSwapChainImage(xyvk_device& vkdevice, VkSwapchainKHR& swapChain, uint32_t &swapFrameIndex, VkFence imageAcquiredF = VK_NULL_HANDLE, VkSemaphore imageAcquiredS = VK_NULL_HANDLE) {
				return vkAcquireNextImageKHR(vkdevice.logicalDevice, swapChain, UINT64_MAX, imageAcquiredS, imageAcquiredF, &swapFrameIndex);
			}

			/// @brief Create the Vulkan surface swap-chain images and imageviews.
			static inline VkResult CreateSwapChainImages(xyvk_device& vkdevice, xyvk_window& window, xyvk_surfaceinfo& presentDetails, VkSwapchainKHR& swapChain, std::vector<xyvk_image*>& swapChainImages) {
				xyvk_swapchaininfo swapChainSupport = QuerySwapChainSupport(vkdevice.physicalDevice, vkdevice.presentSurface);
				VkSurfaceFormatKHR surfaceFormat = QuerySwapSurfaceFormat(presentDetails, swapChainSupport.formats);
				VkPresentModeKHR presentMode = QuerySwapPresentMode(presentDetails, swapChainSupport.presentModes);
				VkExtent2D extent = QuerySwapExtent(window, swapChainSupport.capabilities);
				uint32_t imageCount = std::min(swapChainSupport.capabilities.maxImageCount, std::max(swapChainSupport.capabilities.minImageCount, static_cast<uint32_t>(XYVK_BUFFERINGMODE::DOUBLE)));

				if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
					imageCount = swapChainSupport.capabilities.maxImageCount;

				VkSwapchainCreateInfoKHR createInfo {};
				createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
				createInfo.surface = vkdevice.presentSurface;
				createInfo.minImageCount = imageCount;
				createInfo.imageFormat = surfaceFormat.format;
				createInfo.imageColorSpace = surfaceFormat.colorSpace;
				createInfo.imageExtent = extent;
				createInfo.imageArrayLayers = 1; // Change when developing VR or other 3D stereoscopic applications
				createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
				
				if (!vkdevice.queueFamilyIndices.hasGraphicsFamily || !vkdevice.queueFamilyIndices.hasPresentFamily)
					return VK_ERROR_INITIALIZATION_FAILED;
				
				uint32_t queueFamilyIndices[] = { vkdevice.queueFamilyIndices.graphicsFamily, vkdevice.queueFamilyIndices.presentFamily };

				if (vkdevice.queueFamilyIndices.graphicsFamily != vkdevice.queueFamilyIndices.presentFamily) {
					createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
					createInfo.queueFamilyIndexCount = 2;
					createInfo.pQueueFamilyIndices = queueFamilyIndices;
				} else {
					createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
					createInfo.queueFamilyIndexCount = 0; // Optional
					createInfo.pQueueFamilyIndices = VK_NULL_HANDLE; // Optional
				}

				createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
				createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
				createInfo.presentMode = presentMode;
				createInfo.clipped = VK_TRUE;
				createInfo.oldSwapchain = swapChain;

				if (vkCreateSwapchainKHR(vkdevice.logicalDevice, &createInfo, VK_NULL_HANDLE, &swapChain) != VK_SUCCESS)
					return VK_ERROR_INITIALIZATION_FAILED;

				vkGetSwapchainImagesKHR(vkdevice.logicalDevice, swapChain, &imageCount, VK_NULL_HANDLE);
				std::vector<VkImage> newSwapImages;
				newSwapImages.resize(imageCount);
				vkGetSwapchainImagesKHR(vkdevice.logicalDevice, swapChain, &imageCount, newSwapImages.data());

				swapChainImages.resize(imageCount);
				for(uint32_t i = 0; i < imageCount; i++) {
					swapChainImages[i] = new xyvk_image(vkdevice, XYVK_IMAGETYPE::SWAPCHAIN, extent.width, extent.height, presentDetails.dataFormat, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, false, newSwapImages[i], VK_NULL_HANDLE, VK_NULL_HANDLE);
					swapChainImages[i]->Initialize();
				}
				
				return VK_SUCCESS;
			}

			/// @brief Create the image views for rendering to images (including those in the swap-chain).
			static inline void CreateSwapChainImageViews(xyvk_device& vkdevice, xyvk_surfaceinfo& presentDetails, std::vector<xyvk_image*>& swapChainImages) {
				xyvk_swapchaininfo swapChainSupport = QuerySwapChainSupport(vkdevice.physicalDevice, vkdevice.presentSurface);
				VkSurfaceFormatKHR surfaceFormat = QuerySwapSurfaceFormat(presentDetails, swapChainSupport.formats);
				
				for (size_t i = 0; i < swapChainImages.size(); i++) {
					VkImageViewCreateInfo createInfo {
						.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
						.image = swapChainImages[i]->imageSource, .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = surfaceFormat.format,
						.components.r = VK_COMPONENT_SWIZZLE_IDENTITY, .components.g = VK_COMPONENT_SWIZZLE_IDENTITY, .components.b = VK_COMPONENT_SWIZZLE_IDENTITY, .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
						.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .subresourceRange.baseMipLevel = 0, .subresourceRange.levelCount = 1, .subresourceRange.baseArrayLayer = 0, .subresourceRange.layerCount = 1,
					};
					vkCreateImageView(vkdevice.logicalDevice, &createInfo, VK_NULL_HANDLE, &swapChainImages[i]->imageView);
				}
			}

			/// @brief Waits on VkFence to be signaled before resetting (GPU -> CPU synchronization).
			static inline void WaitResetFences(xyvk_device& vkdevice, VkFence* fence) {
				vkWaitForFences(vkdevice.logicalDevice, 1, fence, VK_TRUE, UINT64_MAX);
				vkResetFences(vkdevice.logicalDevice, 1, fence);
			}
			
			/// @brief Submits the current acquired swapchain image for presentation.
			static VkResult QueuePresent(VkQueue presentQueue, VkSwapchainKHR swapchain, VkSemaphore imageFinished, uint32_t swapImageIndex) {
				VkPresentInfoKHR presentInfo {
					.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
					.waitSemaphoreCount = 1, .pImageIndices = &swapImageIndex, .pWaitSemaphores = &imageFinished,
					.swapchainCount = 1, .pSwapchains = &swapchain
				};
				return vkQueuePresentKHR(presentQueue, &presentInfo);
			}
			
			/// @brief Returns the current resource synchronized frame index.
			static inline void PushPresentMode(xyvk_surfaceinfo& presentDetails, VkPresentModeKHR presentMode, std::atomic_bool& refreshFlag) {
				if (presentMode != presentDetails.idealPresentMode) {
					presentDetails = { presentDetails.dataFormat, presentDetails.colorSpace, presentMode };
					refreshFlag = true;
				}
			}
		};
	}
#endif