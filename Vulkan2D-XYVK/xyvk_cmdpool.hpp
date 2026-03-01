#pragma once
#ifndef __XYVK_CMDPOOL
#define __XYVK_CMDPOOL
	#include "./xyvk_engine.hpp"

	namespace XYVK_NAMESPACE {
		class xyvk_cmdpool : public xyvk_disposable {
		public:
			xyvk_device& vkdevice;
			VkCommandPool commandPool;
			size_t bufferCount;
			std::vector<std::pair<VkCommandBuffer, VkBool32>> commandBuffers;
			VkResult initialized = VK_ERROR_INITIALIZATION_FAILED;

			xyvk_cmdpool(const xyvk_cmdpool&) = delete;
			xyvk_cmdpool operator=(const xyvk_cmdpool&) = delete;
			~xyvk_cmdpool() { this->Dispose(); }

			void Disposable(bool waitIdle) {
				if (waitIdle) vkdevice.WaitIdle();
				if (commandPool != VK_NULL_HANDLE) vkDestroyCommandPool(vkdevice.logicalDevice, commandPool, VK_NULL_HANDLE);
			}
			
			xyvk_cmdpool(xyvk_device& vkdevice, size_t bufferCount = 32UL) : vkdevice(vkdevice), bufferCount(bufferCount) {
				onDispose.hook(xyvk_callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));
				initialized = Initialize();
			}

			VkResult CreatePool() {
				xyvk_queuefamily queueFamily = QueryPhysicalDeviceQueueFamilies(vkdevice.physicalDevice, vkdevice.presentSurface);
				VkCommandPoolCreateInfo poolInfo { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, .queueFamilyIndex = queueFamily.graphicsFamily };
                return vkCreateCommandPool(vkdevice.logicalDevice, &poolInfo, VK_NULL_HANDLE, &commandPool);
			}

			VkResult CreateBuffers(size_t bufferCount = 1) {
				VkCommandBufferAllocateInfo allocInfo {
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
					.commandPool = commandPool, .commandBufferCount = static_cast<uint32_t>(bufferCount)
				};
				
                std::vector<VkCommandBuffer> temporary(bufferCount);
				VkResult result = vkAllocateCommandBuffers(vkdevice.logicalDevice, &allocInfo, temporary.data());

				if (result == VK_SUCCESS)
					std::for_each(temporary.begin(), temporary.end(),
                        [&buffers = commandBuffers](VkCommandBuffer cmdBuffer) { buffers.push_back(std::pair(cmdBuffer, static_cast<VkBool32>(false)));
                    });
                return result;
			}

			std::pair<VkCommandBuffer,int32_t> Lease(bool resetCmdBuffer = false) {
				size_t index = 0;
				for(auto& cmdBuffer : commandBuffers)
					if (!cmdBuffer.second) {
						cmdBuffer.second = true;
						if (resetCmdBuffer)
                            vkResetCommandBuffer(cmdBuffer.first, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
						return std::pair(cmdBuffer.first, index++);
					}
				return std::pair<VkCommandBuffer,int32_t>(VK_NULL_HANDLE,-1);
			}

			VkResult Return(std::pair<VkCommandBuffer, int32_t> bufferIndexPair) {
				if (bufferIndexPair.second < 0 || bufferIndexPair .second >= commandBuffers.size())
					return VK_ERROR_NOT_PERMITTED_KHR;
				commandBuffers[bufferIndexPair.second].second = false;
				return VK_SUCCESS;
			}

			VkResult ReturnAll() {
				VkResult result = vkResetCommandPool(vkdevice.logicalDevice, commandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
				if (result == VK_SUCCESS)
                    for(auto& cmdBuffer : commandBuffers)
                        cmdBuffer.second = false;
                return result;
			}

			VkResult Initialize() {
                VkResult result = CreatePool();
                if (result != VK_SUCCESS) return result;
                return CreateBuffers(bufferCount);
            }
		};
	}
#endif