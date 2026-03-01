#pragma once
#ifndef __XYVK_IMAGE
#define __XYVK_IMAGE
	#include "./xyvk_engine.hpp"

	namespace XYVK_NAMESPACE {
		/// @brief GPU device image for sending images to the render (GPU) device.
		class xyvk_image : public xyvk_disposable {
		public:
			xyvk_device& vkdevice;
			VmaAllocation imageMemory = VK_NULL_HANDLE;
			VkImage imageSource = VK_NULL_HANDLE;
			VkImageView imageView = VK_NULL_HANDLE;
			VkSampler imageSampler = VK_NULL_HANDLE;
			
			XYVK_IMAGETYPE sourceType;
            XYVK_IMAGELAYOUT imageLayout;
            VkDeviceSize width, height;
            bool lerpFilter;
            VkFormat rgbaFormat;
			VkImageAspectFlags aspectFlags;
			VkSamplerAddressMode addressMode;
			VkResult initialized = VK_ERROR_INITIALIZATION_FAILED;
			
			xyvk_image operator=(const xyvk_image&) = delete;
			xyvk_image(const xyvk_image&) = delete;
			~xyvk_image() { this->Dispose(); }

			void Disposable(bool waitIdle) {
				if (waitIdle) vkdevice.WaitIdle();
				if (sourceType != XYVK_IMAGETYPE::SWAPCHAIN) {
					if (imageSampler != VK_NULL_HANDLE) vkDestroySampler(vkdevice.logicalDevice, imageSampler, VK_NULL_HANDLE);
					if (imageView != VK_NULL_HANDLE) vkDestroyImageView(vkdevice.logicalDevice, imageView, VK_NULL_HANDLE);
					if (imageSource != VK_NULL_HANDLE) vmaDestroyImage(vkdevice.memoryAllocator, imageSource, imageMemory);
				}
			}

            xyvk_image(xyvk_device& vkdevice, const XYVK_IMAGETYPE sourceType, VkDeviceSize width, VkDeviceSize height, VkFormat rgbaFormat = VK_FORMAT_B8G8R8A8_UNORM, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, bool lerpFilter = false, VkImage imageSource = VK_NULL_HANDLE, VkImageView imageView = VK_NULL_HANDLE, VkSampler imageSampler = VK_NULL_HANDLE)
            : vkdevice(vkdevice), sourceType(sourceType), width(width), height(height), rgbaFormat(rgbaFormat), addressMode(addressMode), lerpFilter(lerpFilter), imageSource(imageSource), imageView(imageView), imageSampler(imageSampler) {
                onDispose.hook(xyvk_callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));
				initialized = Initialize();
            }

			VkResult CreateImage(XYVK_IMAGETYPE sourceType, VkDeviceSize width, VkDeviceSize height, VkFormat rgbaFormat = VK_FORMAT_R16G16B16A16_UNORM, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, bool lerpFilter = false) {
				if (sourceType == XYVK_IMAGETYPE::SWAPCHAIN)
					return VK_ERROR_INITIALIZATION_FAILED;

				VkImageCreateInfo imgCreateInfo = {
					.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, .extent.width = static_cast<uint32_t>(width), .extent.height = static_cast<uint32_t>(height),
					.extent.depth = 1, .mipLevels = 1, .arrayLayers = 1, .format = rgbaFormat, .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, .imageType = VK_IMAGE_TYPE_2D,
					.tiling = VK_IMAGE_TILING_OPTIMAL, .samples = VK_SAMPLE_COUNT_1_BIT, .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
				};
				
				this->sourceType = sourceType;
				this->width = width;
				this->height = height;
				this->imageLayout = XYVK_IMAGELAYOUT::UNINITIALIZED;
				this->aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
				this->lerpFilter = lerpFilter;
				this->addressMode = addressMode;
				this->rgbaFormat = rgbaFormat;

				VmaAllocationCreateInfo allocCreateInfo {
					.priority = 1.0f, .usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
					.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
				};
				
				VkResult result = vmaCreateImage(vkdevice.memoryAllocator, &imgCreateInfo, &allocCreateInfo, &imageSource, &imageMemory, VK_NULL_HANDLE);
				if (result != VK_SUCCESS) return result;
				
				VkPhysicalDeviceProperties properties {};
				vkGetPhysicalDeviceProperties(vkdevice.physicalDevice, &properties);

				VkFilter filter = (lerpFilter)? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
				VkSamplerMipmapMode mipmapMode = (lerpFilter)? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
				float lerpFilterWeight = (lerpFilter)? VK_LOD_CLAMP_NONE : 0.0f;

				VkSamplerCreateInfo samplerInfo {
					.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
					.magFilter = filter, .minFilter = filter,
					.anisotropyEnable = VK_FALSE, .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
					.addressModeU = addressMode, .addressModeV = addressMode, .addressModeW = addressMode, .unnormalizedCoordinates = VK_FALSE,
					.compareEnable = VK_FALSE, .compareOp = VK_COMPARE_OP_ALWAYS,
					.mipmapMode = mipmapMode, .mipLodBias = 0.0f, .minLod = 0.0f, .maxLod = lerpFilterWeight,
					.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
				};

				result = vkCreateSampler(vkdevice.logicalDevice, &samplerInfo, VK_NULL_HANDLE, &imageSampler);
				if (result != VK_SUCCESS) return result;

				VkImageViewCreateInfo createInfo {
					.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					.image = imageSource, .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = rgbaFormat, .components = { VK_COMPONENT_SWIZZLE_IDENTITY },
					.subresourceRange = { .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1, .aspectMask = aspectFlags, }
				};

				return vkCreateImageView(vkdevice.logicalDevice, &createInfo, VK_NULL_HANDLE, &imageView);
			}
			
			void GetPipelineBarrierStages(XYVK_IMAGELAYOUT layout, XYVK_CMDBUFFERSTAGE cmdBufferStage, VkPipelineStageFlags& srcStage, VkPipelineStageFlags& dstStage, VkAccessFlags& srcAccessMask, VkAccessFlags& dstAccessMask) {
				if (cmdBufferStage == XYVK_CMDBUFFERSTAGE::PIPE_TOP) {
					switch (layout) {
						case XYVK_IMAGELAYOUT::COLOR_ATTACHMENT:
						case XYVK_IMAGELAYOUT::PRESENT_SRCKHR:
						case XYVK_IMAGELAYOUT::UNINITIALIZED:
							dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
							dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
						break;
						case XYVK_IMAGELAYOUT::TRANSFER_SRC:
							dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
							dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
						break;
						case XYVK_IMAGELAYOUT::TRANSFER_DST:
							dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
							dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						break;
						case XYVK_IMAGELAYOUT::SHADER_READONLY:
							dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
							dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
						break;
					}

					srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					srcAccessMask = VK_ACCESS_NONE;
				}
				
				if (cmdBufferStage == XYVK_CMDBUFFERSTAGE::PIPE_BOT) {
					switch (layout) {
						case XYVK_IMAGELAYOUT::COLOR_ATTACHMENT:
						case XYVK_IMAGELAYOUT::PRESENT_SRCKHR:
						case XYVK_IMAGELAYOUT::UNINITIALIZED:
							srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
							srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
						break;
						case XYVK_IMAGELAYOUT::TRANSFER_SRC:
							srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
							srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
						break;
						case XYVK_IMAGELAYOUT::TRANSFER_DST:
							srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
							srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						break;
						case XYVK_IMAGELAYOUT::SHADER_READONLY:
							srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
							srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
						break;
					}

					dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
					dstAccessMask = VK_ACCESS_NONE;
				}
				
				if (cmdBufferStage == XYVK_CMDBUFFERSTAGE::PIPE_ALL) {
					srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
					srcAccessMask = VK_ACCESS_NONE;
					dstAccessMask = VK_ACCESS_NONE;
				}
			}

			VkImageMemoryBarrier GetPipelineBarrier(XYVK_IMAGELAYOUT newLayout, XYVK_CMDBUFFERSTAGE cmdBufferStage, VkPipelineStageFlags& srcStage, VkPipelineStageFlags& dstStage) {
				VkImageMemoryBarrier pipelineBarrier = {
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.oldLayout = (VkImageLayout) imageLayout, .newLayout = (VkImageLayout) newLayout,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1, },
					.image = imageSource,
				};

				VkAccessFlags srcAccessMask, dstAccessMask;
				GetPipelineBarrierStages(newLayout, cmdBufferStage, srcStage, dstStage, srcAccessMask, dstAccessMask);
				pipelineBarrier.srcAccessMask = srcAccessMask;
				pipelineBarrier.dstAccessMask = dstAccessMask;
				return pipelineBarrier;
			}
			
			void TransitionLayoutBarrier(VkCommandBuffer cmdBuffer, XYVK_CMDBUFFERSTAGE cmdBufferStage, XYVK_IMAGELAYOUT newLayout) {
				VkPipelineStageFlags srcStage, dstStage;
				VkImageMemoryBarrier pipelineBarrier = GetPipelineBarrier(newLayout, cmdBufferStage, srcStage, dstStage);
				imageLayout = newLayout;
				aspectFlags = pipelineBarrier.subresourceRange.aspectMask;
				vkCmdPipelineBarrier(cmdBuffer, srcStage, dstStage, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &pipelineBarrier);
			}
			
			VkDescriptorImageInfo GetDescriptorInfo() {
                return { imageSampler, imageView, (VkImageLayout) imageLayout };
            }
            
			inline static VkWriteDescriptorSet GetWriteDescriptor(uint32_t binding, uint32_t descriptorCount, const VkDescriptorImageInfo* imageInfo) {
				return { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .pImageInfo = imageInfo, .dstSet = 0, .dstBinding = binding, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = descriptorCount };
			}
			
			VkResult Initialize() {
				if (sourceType == XYVK_IMAGETYPE::SWAPCHAIN) {
					imageLayout = XYVK_IMAGELAYOUT::UNINITIALIZED;
					aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
					rgbaFormat = VK_FORMAT_B8G8R8A8_UNORM;
					return (imageSource == VK_NULL_HANDLE)? VK_ERROR_INITIALIZATION_FAILED : VK_SUCCESS;
				} else {
					return CreateImage(sourceType, width, height, rgbaFormat, addressMode);
				}
			}
		};
	}
#endif