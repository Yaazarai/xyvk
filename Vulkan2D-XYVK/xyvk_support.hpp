#pragma once
#ifndef __XYVK_SUPPORT
#define __XYVK_SUPPORT
	#include "./xyvk_engine.hpp"

	namespace XYVK_NAMESPACE {
		#pragma region VULKAN_DEBUG_UTILITIES

		VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
			#if XYVK_VALIDATION
				auto create = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
				if (create != VK_NULL_HANDLE)
					return create(instance, pCreateInfo, pAllocator, pDebugMessenger);
				return VK_ERROR_INITIALIZATION_FAILED;
			#endif
			return VK_SUCCESS;
		}

		VkResult DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
			#if XYVK_VALIDATION
				auto destroy = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
				destroy(instance, debugMessenger, pAllocator);
				if (destroy != VK_NULL_HANDLE)
					return VK_SUCCESS;
				return VK_ERROR_INITIALIZATION_FAILED;
			#endif
			return VK_SUCCESS;
		}

		VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
			#if XYVK_VALIDATION
				std::cout << "xyvk-engine: Validation Layer: " << pCallbackData->pMessage << std::endl;
				return (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)? VK_TRUE : VK_FALSE;
			#endif
			return VK_FALSE;
		}

		#pragma endregion
		#pragma region VULKAN_DYNAMIC_RENDERING_FUNCTIONS

		#define VKDECLARE_EXTFN(FN,SFX) PFN_##FN FN##SFX = VK_NULL_HANDLE
		#define VKIMPORTS_EXTFN(FN,SFX) FN##SFX = (PFN_##FN) vkGetInstanceProcAddr(instance, #FN); \
			if (FN##SFX == VK_NULL_HANDLE) { std::cout << "xyvk-engine: Failed to load VK_KHR_dynamic_rendering EXT function: PFN_"#FN << std::endl; return VK_ERROR_FEATURE_NOT_PRESENT; }

		VKDECLARE_EXTFN(vkCmdBeginRenderingKHR,XYVK);
		VKDECLARE_EXTFN(vkCmdEndRenderingKHR,XYVK);
		VKDECLARE_EXTFN(vkCmdPushDescriptorSetKHR,XYVK);
		VKDECLARE_EXTFN(vkCreateShadersEXT, XYVK);
		VKDECLARE_EXTFN(vkDestroyShaderEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdBindShadersEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdSetColorWriteEnableEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdSetColorBlendEnableEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdSetColorBlendEquationEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdSetColorWriteMaskEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdSetDepthWriteEnableEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdSetDepthTestEnableEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdSetDepthClampEnableEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdSetDepthBiasEnableEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdSetVertexInputEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdSetPrimitiveTopologyEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdSetPolygonModeEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdSetCullModeEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdSetFrontFaceEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdSetRasterizerDiscardEnableEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdSetStencilTestEnableEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdSetRasterizationSamplesEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdSetSampleMaskEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdSetAlphaToCoverageEnableEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdSetPrimitiveRestartEnableEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdSetViewportWithCountEXT,XYVK);
		VKDECLARE_EXTFN(vkCmdSetScissorWithCountEXT,XYVK);
		
		VkResult vkCmdRenderingGetCallbacks(VkInstance instance) {
			VKIMPORTS_EXTFN(vkCmdBeginRenderingKHR,XYVK);
			VKIMPORTS_EXTFN(vkCmdEndRenderingKHR,XYVK);
			VKIMPORTS_EXTFN(vkCmdPushDescriptorSetKHR,XYVK);
			VKIMPORTS_EXTFN(vkCreateShadersEXT, XYVK);
			VKIMPORTS_EXTFN(vkDestroyShaderEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdBindShadersEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdSetColorWriteEnableEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdSetColorBlendEnableEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdSetColorBlendEquationEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdSetColorWriteMaskEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdSetDepthWriteEnableEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdSetDepthTestEnableEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdSetDepthClampEnableEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdSetDepthBiasEnableEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdSetVertexInputEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdSetPrimitiveTopologyEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdSetPolygonModeEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdSetCullModeEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdSetFrontFaceEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdSetRasterizerDiscardEnableEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdSetStencilTestEnableEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdSetRasterizationSamplesEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdSetSampleMaskEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdSetAlphaToCoverageEnableEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdSetPrimitiveRestartEnableEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdSetViewportWithCountEXT,XYVK);
			VKIMPORTS_EXTFN(vkCmdSetScissorWithCountEXT,XYVK);
			return VK_SUCCESS;
		}

		#pragma endregion
        #pragma region VULKAN_INTERFACE SUPPORT

		/// @brief Vulkan Queue Family flags.
		struct xyvk_queuefamily {
			uint32_t graphicsFamily, presentFamily;
			bool hasGraphicsFamily, hasPresentFamily;

			xyvk_queuefamily() : graphicsFamily(0), presentFamily(0), hasGraphicsFamily(false), hasPresentFamily(false) {}
			void SetGraphicsFamily(uint32_t queueFamily) { graphicsFamily = queueFamily; hasGraphicsFamily = true; }
			void SetPresentFamily(uint32_t queueFamily) { presentFamily = queueFamily; hasPresentFamily = true; }
		};

		/// @brief Description of the SwapChain Rendering format.
		struct xyvk_swapchaininfo {
		public:
			VkSurfaceCapabilitiesKHR capabilities = {};
			std::vector<VkSurfaceFormatKHR> formats = {};
			std::vector<VkPresentModeKHR> presentModes = {};
		};

		/// @brief Description of the Rendering Surface format.
		struct xyvk_surfaceinfo {
		public:
			VkFormat dataFormat = VK_FORMAT_B8G8R8A8_UNORM;
			VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			VkPresentModeKHR idealPresentMode = VK_PRESENT_MODE_FIFO_KHR;
		};

        #pragma endregion
		#pragma region VULKAN_ENUMERATE_HELPER_FUNCTIONS
		
		VkDeviceSize QueryPhysicalDeviceRankByHeapSize(VkPhysicalDevice device) {
			VkPhysicalDeviceMemoryProperties2 memoryProperties { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2 };
			vkGetPhysicalDeviceMemoryProperties2(device, &memoryProperties);
			for(VkMemoryHeap heap : memoryProperties.memoryProperties.memoryHeaps)
				if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) return heap.size;
			return static_cast<VkDeviceSize>(0);
		}
		
		VkResult QueryPhysicalDevices(VkInstance instance, std::vector<VkPhysicalDevice>& devices) {
			uint32_t deviceCount;
			VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, VK_NULL_HANDLE);
			devices.resize(deviceCount);
			if (result == VK_SUCCESS)
				result = vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
			return result;
		}

		VkResult QueryQueueFamilyProperties(VkPhysicalDevice pdevice, std::vector<VkQueueFamilyProperties>& queueFamilies) {
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &queueFamilyCount, VK_NULL_HANDLE);
			queueFamilies.resize(queueFamilyCount);
			if (queueFamilyCount > 0) {
				vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &queueFamilyCount, queueFamilies.data());
				return VK_SUCCESS;
			}
			return VK_ERROR_DEVICE_LOST;
		}

		xyvk_queuefamily QueryPhysicalDeviceQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR presentSurface) {
			xyvk_queuefamily indices = {};
			if (device != VK_NULL_HANDLE) {
				std::vector<VkQueueFamilyProperties> queueFamilies;
				QueryQueueFamilyProperties(device, queueFamilies);
				for (int i = 0; i < queueFamilies.size(); i++) {
					VkBool32 presentSupport = false;
					vkGetPhysicalDeviceSurfaceSupportKHR(device, i, presentSurface, &presentSupport);
					if (!indices.hasGraphicsFamily && !indices.hasPresentFamily && presentSupport && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && queueFamilies[i].timestampValidBits) {
						indices.SetGraphicsFamily(i);
						indices.SetPresentFamily(i);
					}
				}
			}
			return indices;
		}

		#pragma endregion
	}
#endif