#pragma once
#ifndef __XYVK_DEVICE
#define __XYVK_DEVICE
	#include "./xyvk_engine.hpp"
	
	namespace XYVK_NAMESPACE {
		class xyvk_device : public xyvk_disposable {
		public:
			std::vector<const char*> deviceExtensions = {
				VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
				VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
				VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
				VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
				VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
				VK_EXT_COLOR_WRITE_ENABLE_EXTENSION_NAME,
				VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME,
				VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
				VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME,
				VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
			}, validationLayers = {}, instanceExtensions = {};
			VkPhysicalDeviceFeatures deviceFeatures = {};
            VkPhysicalDeviceProperties2 deviceProperties = {};

            xyvk_window& window;
			VkInstance instance = VK_NULL_HANDLE;
			VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
			VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
			VkDevice logicalDevice = VK_NULL_HANDLE;
            VmaAllocator memoryAllocator = VK_NULL_HANDLE;
			VkSurfaceKHR presentSurface = VK_NULL_HANDLE;
			xyvk_queuefamily queueFamilyIndices = {};
            VkResult initialized = VK_ERROR_INITIALIZATION_FAILED;

			xyvk_device(const xyvk_device&) = delete;
			xyvk_device operator=(const xyvk_device&) = delete;
			~xyvk_device() { this->Dispose(); }
			
			void Disposable(bool waitIdle) {
				if (waitIdle) WaitIdle();
				if (memoryAllocator != VK_NULL_HANDLE) vmaDestroyAllocator(memoryAllocator);
				if (logicalDevice != VK_NULL_HANDLE) vkDestroyDevice(logicalDevice, VK_NULL_HANDLE);
				if (presentSurface != VK_NULL_HANDLE) vkDestroySurfaceKHR(instance, presentSurface, VK_NULL_HANDLE);
				if (debugMessenger != VK_NULL_HANDLE) DestroyDebugUtilsMessengerEXT(instance, debugMessenger, VK_NULL_HANDLE);
				if (instance != VK_NULL_HANDLE) vkDestroyInstance(instance, VK_NULL_HANDLE);
			}

			xyvk_device(xyvk_window& window, VkPhysicalDeviceFeatures deviceFeatures = { .multiDrawIndirect = VK_TRUE, .fillModeNonSolid = VK_TRUE })
			: window(window), deviceFeatures(deviceFeatures) {
				onDispose.hook(xyvk_callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));
				initialized = Initialize();
			}

			VkResult WaitIdle() {
				return vkDeviceWaitIdle(logicalDevice);
			}
			
			VkResult CreateVkInstance() {
				for (const auto& extension : window.QueryRequiredExtensions())
					instanceExtensions.push_back(extension);
				
				#if XYVK_VALIDATION
					instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
					validationLayers.push_back(VK_LAYER_KHRONOS_EXTENSION_NAME);
				#endif

				const VkDebugUtilsMessengerCreateInfoEXT defaultDebugCreateInfo {
					.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
					.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
					.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
					.pfnUserCallback = DebugCallback,
					.pUserData = VK_NULL_HANDLE
				};

				const VkApplicationInfo defaultAppInfo {
					.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
					.pApplicationName = XYVK_ENGINE_NAME,
					.applicationVersion = XYVK_ENGINE_VERSION,
					.engineVersion = XYVK_ENGINE_VERSION,
					.apiVersion = XYVK_ENGINE_VERSION,
					.pEngineName = XYVK_ENGINE_NAME
				};
				
				VkInstanceCreateInfo createInfo {
					.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
					.enabledLayerCount = static_cast<uint32_t>(validationLayers.size()), .ppEnabledLayerNames = validationLayers.data(),
					.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()), .ppEnabledExtensionNames = instanceExtensions.data(),
					.pApplicationInfo = &defaultAppInfo, .pNext = &defaultDebugCreateInfo
				};

				VkResult result = vkCreateInstance(&createInfo, VK_NULL_HANDLE, &instance);
				presentSurface = window.CreateWindowSurface(instance);
				if (result != VK_SUCCESS) return result;

                #if XYVK_VALIDATION
				    return CreateDebugUtilsMessengerEXT(instance, &defaultDebugCreateInfo, VK_NULL_HANDLE, &debugMessenger);
                #endif
                return result;
			}

			VkResult CreatePhysicalDevice() {
				std::vector<VkPhysicalDevice> suitableDevices;
				QueryPhysicalDevices(instance, suitableDevices);
				std::sort(suitableDevices.begin(), suitableDevices.end(), [this](VkPhysicalDevice A, VkPhysicalDevice B) { return QueryPhysicalDeviceRankByHeapSize(A) >= QueryPhysicalDeviceRankByHeapSize(B); });
				physicalDevice = (suitableDevices.size() > 0)? suitableDevices.front() : VK_NULL_HANDLE;
				return (physicalDevice == VK_NULL_HANDLE)? VK_ERROR_DEVICE_LOST : VK_SUCCESS;
			}

			VkResult CreateLogicalDevice() {
				if (physicalDevice == VK_NULL_HANDLE) return VK_ERROR_DEVICE_LOST;

				std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
				queueFamilyIndices = QueryPhysicalDeviceQueueFamilies(physicalDevice, presentSurface);
				std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices.graphicsFamily, queueFamilyIndices.presentFamily };
                if (!queueFamilyIndices.hasGraphicsFamily || !queueFamilyIndices.hasPresentFamily) return VK_ERROR_INITIALIZATION_FAILED;

				float queuePriority = 1.0f;
				for (uint32_t queueFamily : uniqueQueueFamilies)
					queueCreateInfos.push_back({ .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, .queueCount = 1, .queueFamilyIndex = queueFamily, .pQueuePriorities = &queuePriority });
				
				const VkPhysicalDeviceTimelineSemaphoreFeatures defaultTimelineSemaphoreFeatures {
					.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES,
					.timelineSemaphore = VK_TRUE
				};

				const VkPhysicalDeviceDynamicRenderingFeatures defaultDynamicRenderingCreateInfo {
					.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
					.dynamicRendering = VK_TRUE,
					.pNext = const_cast<VkPhysicalDeviceTimelineSemaphoreFeatures*>(&defaultTimelineSemaphoreFeatures)
				};

				const VkPhysicalDeviceColorWriteEnableFeaturesEXT dynamicColorWriteFeatures = {
					.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COLOR_WRITE_ENABLE_FEATURES_EXT,
					.colorWriteEnable = VK_TRUE,
					.pNext = const_cast<VkPhysicalDeviceDynamicRenderingFeatures*>(&defaultDynamicRenderingCreateInfo),
				};
				
				const VkPhysicalDeviceExtendedDynamicState2FeaturesEXT dynamicFeautures2 = {
					.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT,
					.extendedDynamicState2 = VK_TRUE,
					.pNext = const_cast<VkPhysicalDeviceColorWriteEnableFeaturesEXT*>(&dynamicColorWriteFeatures)
				};

				const VkPhysicalDeviceExtendedDynamicState3FeaturesEXT dynamicFeautures3 = {
					.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT,
					.extendedDynamicState3ColorBlendEnable = VK_TRUE,
					.extendedDynamicState3ColorBlendEquation = VK_TRUE,
					.extendedDynamicState3ColorWriteMask = VK_TRUE,
					.extendedDynamicState3DepthClampEnable = VK_TRUE,
					.extendedDynamicState3PolygonMode = VK_TRUE,
					.pNext = const_cast<VkPhysicalDeviceExtendedDynamicState2FeaturesEXT*>(&dynamicFeautures2)
				};

				const VkPhysicalDeviceShaderObjectFeaturesEXT shaderObjectFeatures = {
					.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT,
					.shaderObject = VK_TRUE,
					.pNext = const_cast<VkPhysicalDeviceExtendedDynamicState3FeaturesEXT*>(&dynamicFeautures3)
				};
				
				VkDeviceCreateInfo createInfo {
					.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
					.pQueueCreateInfos = queueCreateInfos.data(), .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
					.ppEnabledExtensionNames = deviceExtensions.data(), .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
					.pEnabledFeatures = &deviceFeatures, .pNext = &shaderObjectFeatures
				};

				return vkCreateDevice(physicalDevice, &createInfo, VK_NULL_HANDLE, &logicalDevice);
			}

			VkResult CreateMemoryAllocator() {
				VmaAllocatorCreateInfo allocatorCreateInfo { .vulkanApiVersion = XYVK_ENGINE_VERSION, .physicalDevice = physicalDevice, .device = logicalDevice, .instance = instance };
				return vmaCreateAllocator(&allocatorCreateInfo, &memoryAllocator);
			}
			
			VkResult Initialize() {
				VkResult result = VK_SUCCESS;
				if ((result = CreateVkInstance()) != VK_SUCCESS) return result;
				if ((result = vkCmdRenderingGetCallbacks(instance)) != VK_SUCCESS) return result;
				if ((result = CreatePhysicalDevice()) != VK_SUCCESS) return result;
				if ((result = CreateLogicalDevice()) != VK_SUCCESS) return result;
				result = CreateMemoryAllocator();

				#if XYVK_VALIDATION
					VkPhysicalDevicePushDescriptorPropertiesKHR pushDescriptorProperties = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR };
					deviceProperties = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &pushDescriptorProperties };
					vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties);
					std::cout << "xyvk-engine: GPU Device Info" << std::endl;
					std::cout << "\tValid Logical Device:    " << (result == VK_SUCCESS?"True":"False") << std::endl;
					std::cout << "\tPhysical Device Name:    " << deviceProperties.properties.deviceName << std::endl;
					std::cout << "\tDevice Rank / Heap Size: " << (QueryPhysicalDeviceRankByHeapSize(physicalDevice) / 1000000000) << " GB" << std::endl;
					std::cout << "\tPush Constant Memory:    " << deviceProperties.properties.limits.maxPushConstantsSize << " Bytes" << std::endl;
					std::cout << "\tPush Descriptor Memory:  " << pushDescriptorProperties.maxPushDescriptors << " Count" << std::endl;
				#endif
				return result;
			}
        };
    }

#endif