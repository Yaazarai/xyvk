#pragma once
#ifndef __XYVK_BUFFER
#define __XYVK_BUFFER
	#include "./xyvk_engine.hpp"

	namespace XYVK_NAMESPACE {
		/// @brief GPU device Buffer for sending data to the render (GPU) device.
		class xyvk_buffer : public xyvk_disposable {
		public:
			xyvk_device& vkdevice;

			VkBuffer buffer = VK_NULL_HANDLE;
			VmaAllocation memory = VK_NULL_HANDLE;
			VmaAllocationInfo description;
			VkDeviceSize size;

            const XYVK_BUFFERTYPE bufferType;
			VkResult initialized = VK_ERROR_INITIALIZATION_FAILED;
			
			xyvk_buffer operator=(const xyvk_buffer&) = delete;
			xyvk_buffer(const xyvk_buffer&) = delete;
			~xyvk_buffer() { this->Dispose(); }

			void Disposable(bool waitIdle) {
				if (waitIdle) vkdevice.WaitIdle();
				if (buffer != VK_NULL_HANDLE) vmaDestroyBuffer(vkdevice.memoryAllocator, buffer, memory);
			}

			xyvk_buffer(xyvk_device& vkdevice, const XYVK_BUFFERTYPE bufferType, VkDeviceSize dataSize)
			: vkdevice(vkdevice), size(dataSize), bufferType(bufferType) {
				onDispose.hook(xyvk_callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));
				initialized = Initialize();
			}

			VkResult CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags) {
				VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = size, .usage = usage };
				VmaAllocationCreateInfo allocCreateInfo { .usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST, .flags = flags };
				return vmaCreateBuffer(vkdevice.memoryAllocator, &bufCreateInfo, &allocCreateInfo, &buffer, &memory, &description);
			}
			
			VkDescriptorBufferInfo GetDescriptorInfo(VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE) {
				return { buffer, offset, range };
			}
			
			inline static VkWriteDescriptorSet GetWriteDescriptor(uint32_t binding, uint32_t descriptorCount, const VkDescriptorBufferInfo* bufferInfo) {
				return { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .pBufferInfo = bufferInfo, .dstSet = 0, .dstBinding = binding, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = descriptorCount };
			}
            
            VkResult Initialize() {
                switch (bufferType) {
					case XYVK_BUFFERTYPE::VERTEX:
						return CreateBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
					break;
					case XYVK_BUFFERTYPE::UNIFORM:
						return CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
					break;
					default: case XYVK_BUFFERTYPE::STAGING:
						return CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
					break;
				}
            }
		};
	}
#endif