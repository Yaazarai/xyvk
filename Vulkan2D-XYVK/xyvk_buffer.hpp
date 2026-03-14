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
			VkAccessFlags accessMask = VK_ACCESS_NONE;
			VkPipelineStageFlags accessStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
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
			
			void GetPipelineBarrierStages(XYVK_CMDBUFFERSTAGE cmdBufferStage, VkPipelineStageFlags& nextStage, VkAccessFlags& nextAccessMask) {
				switch (cmdBufferStage) {
					case XYVK_CMDBUFFERSTAGE::PIPE_TOP:
						nextStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
						nextAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
					break;
					case XYVK_CMDBUFFERSTAGE::PIPE_BOT:
						nextStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
						nextAccessMask = VK_ACCESS_SHADER_READ_BIT;
					break;
				}
			}
			
			VkBufferMemoryBarrier GetPipelineBarrier(XYVK_CMDBUFFERSTAGE cmdBufferStage, VkPipelineStageFlags& srcStage, VkPipelineStageFlags& dstStage) {
				VkBufferMemoryBarrier pipelineBarrier = {
					.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.offset = 0, .size = this->size, .buffer = this->buffer
				};
				
				VkAccessFlags dstAccessMask;
				srcStage = accessStage;
				GetPipelineBarrierStages(cmdBufferStage, dstStage, dstAccessMask);
				pipelineBarrier.srcAccessMask = accessMask;
				pipelineBarrier.dstAccessMask = dstAccessMask;
				return pipelineBarrier;
			}
			
			void TransitionBarrier(VkCommandBuffer cmdBuffer, XYVK_CMDBUFFERSTAGE cmdBufferStage) {
				VkPipelineStageFlags srcStage, dstStage;
				VkBufferMemoryBarrier pipelineBarrier = GetPipelineBarrier(cmdBufferStage, srcStage, dstStage);
				
				vkCmdPipelineBarrier(cmdBuffer, srcStage, dstStage, 0, 0, VK_NULL_HANDLE, 1, &pipelineBarrier, 0, VK_NULL_HANDLE);
				accessMask = pipelineBarrier.dstAccessMask;
				accessStage = dstStage;
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