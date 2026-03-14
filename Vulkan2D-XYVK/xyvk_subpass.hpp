#pragma once
#ifndef __XYVK_SUBPASS
#define __XYVK_SUBPASS
	#include "./xyvk_engine.hpp"

	namespace XYVK_NAMESPACE {
		struct xyvk_vertexdescr {
		public:
			const VkVertexInputBindingDescription2EXT binding;
			const std::vector<VkVertexInputAttributeDescription2EXT> attributes;

			xyvk_vertexdescr(VkVertexInputBindingDescription2EXT binding = {}, const std::vector<VkVertexInputAttributeDescription2EXT> attributes = {})
			: binding(binding), attributes(attributes) {}
		};

		class xyvk_vertex {
		public:
			glm::vec2 texcoord;
            glm::vec3 position;
            glm::vec4 color;

            xyvk_vertex() : texcoord(glm::vec2(0.0)), position(glm::vec3(0.0)), color(glm::vec4(1.0)) {};
			xyvk_vertex(glm::vec2 tex, glm::vec3 pos, glm::vec4 col) : texcoord(tex), position(pos), color(col) {}

            static xyvk_vertexdescr GetVertexDescription() {
                return xyvk_vertexdescr(GetBindingDescription(), GetAttributeDescriptions());
            }

			static const VkVertexInputBindingDescription2EXT GetBindingDescription() {
				return {
					.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT,
					.binding = 0, .stride = sizeof(xyvk_vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX, .divisor = 1, .pNext = VK_NULL_HANDLE
				};
			}

			static const std::vector<VkVertexInputAttributeDescription2EXT> GetAttributeDescriptions() {
                return {
					{ .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, .pNext = VK_NULL_HANDLE, .binding = 0, .location = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(xyvk_vertex, texcoord) },
					{ .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, .pNext = VK_NULL_HANDLE, .binding = 0, .location = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(xyvk_vertex, position) },
                	{ .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, .pNext = VK_NULL_HANDLE, .binding = 0, .location = 2, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(xyvk_vertex, color) }
				};
			}
        };

        class xyvk_renderobject {
        public:
            VkPipelineLayout& pipelineLayout;
			XYVK_SUBPASSTYPE subpassType;
			std::pair<VkCommandBuffer, int32_t>& executionBuffer;
			uint32_t bindingIndex;
			VkDeviceSize stagingOffset;

			xyvk_renderobject operator=(const xyvk_renderobject&) = delete;
			xyvk_renderobject(const xyvk_renderobject&) = delete;

			xyvk_renderobject(VkPipelineLayout& pipelineLayout, XYVK_SUBPASSTYPE subpassType, std::pair<VkCommandBuffer, int32_t>& commandBuffer)
            : pipelineLayout(pipelineLayout), executionBuffer(commandBuffer), bindingIndex(0), stagingOffset(0) {}

			void StageBufferToBuffer(xyvk_buffer& stageBuffer, xyvk_buffer& destBuffer, void* sourceData, VkDeviceSize byteSize) {
				stageBuffer.TransitionBarrier(executionBuffer.first, XYVK_CMDBUFFERSTAGE::PIPE_TOP);
				destBuffer.TransitionBarrier(executionBuffer.first, XYVK_CMDBUFFERSTAGE::PIPE_TOP);
				
				void* stagedOffset = static_cast<int8_t*>(stageBuffer.description.pMappedData) + stagingOffset;
				memcpy(stagedOffset, sourceData, (size_t) byteSize);
				VkBufferCopy copyRegion { .srcOffset = stagingOffset, .dstOffset = 0, .size = byteSize };
				vkCmdCopyBuffer(executionBuffer.first, stageBuffer.buffer, destBuffer.buffer, 1, &copyRegion);
				
				destBuffer.TransitionBarrier(executionBuffer.first, XYVK_CMDBUFFERSTAGE::PIPE_BOT);
				stageBuffer.TransitionBarrier(executionBuffer.first, XYVK_CMDBUFFERSTAGE::PIPE_BOT);
				stagingOffset += byteSize;
			}

			void StageBufferToImage(xyvk_buffer& stageBuffer, xyvk_image& destImage, void* sourceData, VkRect2D rect, VkDeviceSize byteSize) {
				void* stagedOffset = static_cast<int8_t*>(stageBuffer.description.pMappedData) + stagingOffset;
				memcpy(stagedOffset, sourceData, (size_t)byteSize);
				
				stageBuffer.TransitionBarrier(executionBuffer.first, XYVK_CMDBUFFERSTAGE::PIPE_TOP);
				destImage.TransitionLayoutBarrier(executionBuffer.first, XYVK_CMDBUFFERSTAGE::PIPE_TOP, XYVK_IMAGELAYOUT::TRANSFER_DST);
				
				VkBufferImageCopy region = {
					.imageSubresource.aspectMask = destImage.aspectFlags, .bufferOffset = 0, .bufferRowLength = 0, .bufferImageHeight = 0,
					.imageSubresource.mipLevel = 0, .imageSubresource.baseArrayLayer = 0, .imageSubresource.layerCount = 1,
					.imageExtent = { static_cast<uint32_t>((rect.extent.width == 0)?destImage.width:rect.extent.width),
							static_cast<uint32_t>((rect.extent.height == 0)?destImage.height:rect.extent.height), 1 },
					.imageOffset = { rect.offset.x, rect.offset.y, 0 },
					.bufferOffset = stagingOffset,
				};
				vkCmdCopyBufferToImage(executionBuffer.first, stageBuffer.buffer, destImage.imageSource, (VkImageLayout) destImage.imageLayout, 1, &region);
				
				destImage.TransitionLayoutBarrier(executionBuffer.first, XYVK_CMDBUFFERSTAGE::PIPE_BOT, XYVK_IMAGELAYOUT::SHADER_READONLY);
				stageBuffer.TransitionBarrier(executionBuffer.first, XYVK_CMDBUFFERSTAGE::PIPE_BOT);
				stagingOffset += byteSize;
			}

			void PushBuffer(xyvk_buffer& uniformBuffer) {
				VkDescriptorBufferInfo bufferDescriptor = uniformBuffer.GetDescriptorInfo();
				VkWriteDescriptorSet bufferDescriptorSet = uniformBuffer.GetWriteDescriptor(bindingIndex++, 1, &bufferDescriptor);
				vkCmdPushDescriptorSetKHRXYVK(executionBuffer.first, (subpassType == XYVK_SUBPASSTYPE::COMPUTE)?VK_PIPELINE_BIND_POINT_COMPUTE:VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1U, &bufferDescriptorSet);
			}

			void PushImage(xyvk_image& uniformImage) {
				VkDescriptorImageInfo imageDescriptor = uniformImage.GetDescriptorInfo();
				VkWriteDescriptorSet imageDescriptorSet = uniformImage.GetWriteDescriptor(bindingIndex++, 1, &imageDescriptor);
				vkCmdPushDescriptorSetKHRXYVK(executionBuffer.first, (subpassType == XYVK_SUBPASSTYPE::COMPUTE)?VK_PIPELINE_BIND_POINT_COMPUTE:VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &imageDescriptorSet);
			}

			void BindVertices(xyvk_buffer& vertexBuffer) {
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(executionBuffer.first, 0, 1, &vertexBuffer.buffer, offsets);
			}
			
			void DrawInstances(VkDeviceSize vertexCount, VkDeviceSize instanceCount, VkDeviceSize firstVertex, VkDeviceSize firstInstance) {
				vkCmdDraw(executionBuffer.first, vertexCount, instanceCount, firstVertex, firstInstance);
			}

			void BindAndDraw(xyvk_buffer& vertexBuffer, VkDeviceSize vertexCount, VkDeviceSize instanceCount, VkDeviceSize firstVertex, VkDeviceSize firstInstance) {
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(executionBuffer.first, 0, 1, &vertexBuffer.buffer, offsets);
				vkCmdDraw(executionBuffer.first, vertexCount, instanceCount, firstVertex, firstInstance);
			}
		};

		struct xyvk_dynamicstate {
		public:
			VkBool32 blending;
			VkBool32 clearOnLoad;
			VkClearValue clearColor;
			VkPolygonMode polygonMode;
			VkPrimitiveTopology vertexTopology;
			xyvk_vertexdescr vertexDescription;

			xyvk_dynamicstate(VkBool32 blending = VK_TRUE, VkBool32 clearOnLoad = VK_TRUE, VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f }, VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL,
				VkPrimitiveTopology vertexTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, xyvk_vertexdescr vertexDescription = xyvk_vertex::GetVertexDescription())
			: blending(blending), clearOnLoad(clearOnLoad), clearColor(clearColor), polygonMode(polygonMode), vertexTopology(vertexTopology), vertexDescription(vertexDescription) {}
		};

		class xyvk_subpass : public xyvk_disposable {
		public:
            xyvk_device& vkdevice;
            
			std::vector<xyvk_shader*> shaderStages;
            std::vector<VkShaderEXT> shaderPasses;
			std::vector<VkShaderStageFlagBits> shaderStageBits;
            VkDescriptorSetLayout descriptorLayout = VK_NULL_HANDLE;
            xyvk_invoker<xyvk_subpass&, xyvk_renderobject&, bool> renderEvent;
            std::vector<xyvk_subpass*> dependencies;
			
			xyvk_cmdpool* targetCmdPool = VK_NULL_HANDLE;
			xyvk_image* targetImage = VK_NULL_HANDLE;
            VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
			VkQueue submitQueue = VK_NULL_HANDLE;
			xyvk_dynamicstate dynamicState;
			XYVK_SUBPASSTYPE subpassType;
			
            const std::string title;
			const VkDeviceSize subpassIndex;
			const VkDeviceSize localSubpassIndex;
			
            VkDeviceSize timelineWait = 0;
            uint32_t maxTimestamps, timestampIterator;
            VkQueryPool timestampQueryPool = VK_NULL_HANDLE;
            VkResult initialized = VK_ERROR_INITIALIZATION_FAILED;
			
			xyvk_subpass operator=(const xyvk_subpass&) = delete;
			xyvk_subpass(const xyvk_subpass&) = delete;
			~xyvk_subpass() { this->Dispose(); }

			void Disposable(bool waitIdle) {
				if (waitIdle) { vkQueueWaitIdle(submitQueue); vkdevice.WaitIdle(); }

				for(VkShaderEXT shaderObject : shaderPasses)
					vkDestroyShaderEXTXYVK(vkdevice.logicalDevice, shaderObject, VK_NULL_HANDLE);

				if (timestampQueryPool != VK_NULL_HANDLE) vkDestroyQueryPool(vkdevice.logicalDevice, timestampQueryPool, VK_NULL_HANDLE);
				if (pipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(vkdevice.logicalDevice, pipelineLayout, VK_NULL_HANDLE);
				if (descriptorLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(vkdevice.logicalDevice, descriptorLayout, VK_NULL_HANDLE);
			}
			
			xyvk_subpass(xyvk_device& vkdevice, XYVK_SUBPASSTYPE subpassType, std::vector<xyvk_shader*> shaderStages, std::string title, VkDeviceSize subpassIndex, VkDeviceSize localSubpassIndex, xyvk_dynamicstate dynamicState = xyvk_dynamicstate())
            : vkdevice(vkdevice), subpassType(subpassType), shaderStages(shaderStages), title(title), subpassIndex(subpassIndex), localSubpassIndex(localSubpassIndex), dynamicState(dynamicState), maxTimestamps(8U) {
                onDispose.hook(xyvk_callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));
				
				std::sort(this->shaderStages.begin(), this->shaderStages.end(),
                    [](xyvk_shader* A, xyvk_shader* B){ return A->stage < B->stage; });
				
				for(xyvk_shader* shader : shaderStages)
					shaderStageBits.push_back(static_cast<VkShaderStageFlagBits>(shader->stage));
                
				switch(subpassType) {
					case XYVK_SUBPASSTYPE::COMPUTE:
					case XYVK_SUBPASSTYPE::FRAGMENT:
					case XYVK_SUBPASSTYPE::TRANSFER:
						vkGetDeviceQueue(vkdevice.logicalDevice, vkdevice.queueFamilyIndices.graphicsFamily, 0, &submitQueue);
					break;
					case XYVK_SUBPASSTYPE::PRESENT:
						vkGetDeviceQueue(vkdevice.logicalDevice, vkdevice.queueFamilyIndices.presentFamily, 0, &submitQueue);
					break;
				}

				#if XYVK_VALIDATION
					switch(subpassType) {
						case XYVK_SUBPASSTYPE::FRAGMENT:
						std::cout << "xyvk-engine: [" << subpassIndex << "] Created graphics pass [" << title << "]" << std::endl;
						break;
						case XYVK_SUBPASSTYPE::PRESENT:
						std::cout << "xyvk-engine: [" << subpassIndex <<  "] Created present pass [" << title << "]" << std::endl;
						break;
						case XYVK_SUBPASSTYPE::TRANSFER:
						std::cout << "xyvk-engine: [" << subpassIndex << "] Created transfer only pass [" << title << "]" << std::endl;
						break;
					}
				#endif
				
				initialized = Initialize();
            }

			VkResult AddDependency(xyvk_subpass* dependency) {
				if (subpassIndex <= dependency->subpassIndex) {
					#if XYVK_VALIDATION
						std::cout << "xyvk-engine: Tried to create cyclical renderpass dependency: " << subpassIndex << " ID depends " << dependency->subpassIndex << " ID" << std::endl;
						std::cout << "\t\tRender passes cannot have dependency passes initialized before them (self/equal or lower IDs)." << std::endl;
					#endif
					return VK_ERROR_NOT_PERMITTED_KHR;
				}
				
				dependencies.push_back(dependency);
				timelineWait = std::max(timelineWait, dependency->subpassIndex);
				return VK_SUCCESS;
			}

			void SetTargetImage(xyvk_image* targetImage) {
				this->targetImage = targetImage;
			}

            void SetTargetCmdPool(xyvk_cmdpool* targetCmdPool) {
				this->targetCmdPool = targetCmdPool;
			}
			
			std::vector<float> QueryTimeStamps() {
				std::vector<float> frametimes;
				#if XYVK_VALIDATION
					if (timestampIterator > 0) {
						std::vector<VkDeviceSize> timestamps(timestampIterator);
						vkGetQueryPoolResults(vkdevice.logicalDevice, timestampQueryPool, 0, timestamps.size(), timestamps.size() * sizeof(VkDeviceSize), timestamps.data(), sizeof(VkDeviceSize), VK_QUERY_RESULT_64_BIT);
						
						for(int i = 0; i < timestampIterator; i += 2) {
							float deltams = float(timestamps[i+1] - timestamps[i]) * (vkdevice.deviceProperties.properties.limits.timestampPeriod / 1000000.0f);
							frametimes.push_back(deltams);
						}
					}
				#endif
				return frametimes;
			}

			void QueryTimeStampsOutput(int64_t frameCounter) {
				std::vector<float> timestamps = QueryTimeStamps();
				for(float time : timestamps)
					std::cout << " - [" << frameCounter << "] " << subpassIndex << " : " << title << " - " << time << " ms" << std::endl;
				
				for(xyvk_subpass* dependency : dependencies)
					std::cout << "\t wait: " << dependency->title << " (" << dependency->subpassIndex << ")" << std::endl;
			}
			
			std::pair<VkCommandBuffer,int32_t> BeginCmdBuffer() {
				std::pair<VkCommandBuffer, int32_t> bufferIndexPair = targetCmdPool->Lease(false);
				VkCommandBufferBeginInfo beginInfo { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT };
				VkResult result = vkBeginCommandBuffer(bufferIndexPair.first, &beginInfo);
				
				if (result == VK_SUCCESS) {
					#if XYVK_VALIDATION
						vkCmdResetQueryPool(bufferIndexPair.first, timestampQueryPool, timestampIterator, 2);
						vkCmdWriteTimestamp(bufferIndexPair.first, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, timestampQueryPool, timestampIterator);
						timestampIterator ++;
					#endif
					
					if (subpassType == XYVK_SUBPASSTYPE::FRAGMENT || subpassType == XYVK_SUBPASSTYPE::PRESENT) {
						targetImage->TransitionLayoutBarrier(bufferIndexPair.first, XYVK_CMDBUFFERSTAGE::PIPE_TOP, XYVK_IMAGELAYOUT::COLOR_ATTACHMENT);
						
						VkViewport dynamicViewportKHR { .x = 0, .y = 0, .minDepth = 0.0f, .maxDepth = 1.0f, .width = static_cast<float>(targetImage->width), .height = static_cast<float>(targetImage->height) };
						VkRect2D renderAreaKHR = { .offset = { .x = 0, .y = 0 } , .extent = { .width = static_cast<uint32_t>(targetImage->width), .height = static_cast<uint32_t>(targetImage->height) } };
						
						VkRenderingAttachmentInfoKHR colorAttachmentInfo { .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
							.clearValue = dynamicState.clearColor, .loadOp = ((dynamicState.clearOnLoad)?VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE), .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
							.imageView = targetImage->imageView, .imageLayout = (VkImageLayout) targetImage->imageLayout
						};
						VkRenderingInfoKHR dynamicRenderInfo { .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR, .colorAttachmentCount = 1, .pColorAttachments = &colorAttachmentInfo, .renderArea = renderAreaKHR, .layerCount = 1 };
						VkColorBlendEquationEXT defaultBlending = {
							.colorBlendOp = VK_BLEND_OP_ADD, .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA, .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
							.alphaBlendOp = VK_BLEND_OP_ADD, .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
						};
						VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
						VkBool32 colorWrites = VK_TRUE;
						VkBool32 defaultBoolFlag = VK_FALSE;
						VkSampleMask sampleMasks = VK_SAMPLE_COUNT_1_BIT;
						
						vkCmdBeginRenderingKHRXYVK(bufferIndexPair.first, &dynamicRenderInfo);
						vkCmdBindShadersEXTXYVK(bufferIndexPair.first, shaderStages.size(), shaderStageBits.data(), shaderPasses.data());
						vkCmdSetViewportWithCountEXTXYVK(bufferIndexPair.first, 1, &dynamicViewportKHR);
						vkCmdSetScissorWithCountEXTXYVK(bufferIndexPair.first, 1, &renderAreaKHR);
						vkCmdSetColorWriteEnableEXTXYVK(bufferIndexPair.first, 1, &colorWrites);
						vkCmdSetColorBlendEnableEXTXYVK(bufferIndexPair.first, 0, 1, &dynamicState.blending);
						vkCmdSetColorBlendEquationEXTXYVK(bufferIndexPair.first, 0, 1, &defaultBlending);
						vkCmdSetColorWriteMaskEXTXYVK(bufferIndexPair.first, 0, 1, &colorWriteMask);
						vkCmdSetDepthWriteEnableEXTXYVK(bufferIndexPair.first, defaultBoolFlag);
						vkCmdSetDepthTestEnableEXTXYVK(bufferIndexPair.first, defaultBoolFlag);
						vkCmdSetDepthClampEnableEXTXYVK(bufferIndexPair.first, defaultBoolFlag);
						vkCmdSetDepthBiasEnableEXTXYVK(bufferIndexPair.first, defaultBoolFlag);
						vkCmdSetStencilTestEnableEXTXYVK(bufferIndexPair.first, VK_FALSE);
						vkCmdSetVertexInputEXTXYVK(bufferIndexPair.first, 1, &dynamicState.vertexDescription.binding, dynamicState.vertexDescription.attributes.size(), dynamicState.vertexDescription.attributes.data());
						vkCmdSetPrimitiveTopologyEXTXYVK(bufferIndexPair.first, dynamicState.vertexTopology);
						vkCmdSetRasterizationSamplesEXTXYVK(bufferIndexPair.first, VK_SAMPLE_COUNT_1_BIT);
						vkCmdSetSampleMaskEXTXYVK(bufferIndexPair.first, VK_SAMPLE_COUNT_1_BIT, &sampleMasks);
						vkCmdSetAlphaToCoverageEnableEXTXYVK(bufferIndexPair.first, defaultBoolFlag);
						vkCmdSetPrimitiveRestartEnableEXTXYVK(bufferIndexPair.first, defaultBoolFlag);
						vkCmdSetCullModeEXTXYVK(bufferIndexPair.first, VK_CULL_MODE_NONE);
						vkCmdSetFrontFaceEXTXYVK(bufferIndexPair.first, VK_FRONT_FACE_CLOCKWISE);
						vkCmdSetPolygonModeEXTXYVK(bufferIndexPair.first, VK_POLYGON_MODE_FILL);
						vkCmdSetRasterizerDiscardEnableEXTXYVK(bufferIndexPair.first, VK_FALSE);
					} else if (subpassType == XYVK_SUBPASSTYPE::COMPUTE) {
						vkCmdBindShadersEXTXYVK(bufferIndexPair.first, shaderStages.size(), shaderStageBits.data(), shaderPasses.data());
					}
				} else { targetCmdPool->Return(bufferIndexPair); }
				return bufferIndexPair;
			}

			void EndCmdBuffer(std::pair<VkCommandBuffer,int32_t> bufferIndexPair) {
				if (subpassType == XYVK_SUBPASSTYPE::FRAGMENT || subpassType == XYVK_SUBPASSTYPE::PRESENT) {
					vkCmdEndRenderingKHRXYVK(bufferIndexPair.first);
					targetImage->TransitionLayoutBarrier(bufferIndexPair.first, XYVK_CMDBUFFERSTAGE::PIPE_BOT,
						(targetImage->sourceType == XYVK_IMAGETYPE::SWAPCHAIN)?
							XYVK_IMAGELAYOUT::PRESENT_SRCKHR : XYVK_IMAGELAYOUT::SHADER_READONLY);
				}

				#if XYVK_VALIDATION
					vkCmdWriteTimestamp(bufferIndexPair.first, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, timestampQueryPool, timestampIterator);
					timestampIterator ++;
				#endif

				vkEndCommandBuffer(bufferIndexPair.first);
			}
			
			VkResult Initialize() {
                std::vector<VkShaderCreateInfoEXT> shaderCreateInfos;
                size_t shader_count = shaderStages.size();

				#if XYVK_VALIDATION
					VkQueryPoolCreateInfo queryCreateInfo = { .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO, .queryType = VK_QUERY_TYPE_TIMESTAMP, .queryCount = 2U * maxTimestamps * XYVK_VALIDATION, .flags = 0 };
					vkCreateQueryPool(vkdevice.logicalDevice, &queryCreateInfo, VK_NULL_HANDLE, &timestampQueryPool);
				#endif

				if (shader_count <= 0) return VK_SUCCESS;

				std::vector<VkDescriptorSetLayoutBinding> pbindings;
                uint32_t bindingIndexIter = 0;
				for(size_t i = 0; i < shader_count; i++) {
					for(VkDescriptorSetLayoutBinding binding : shaderStages[i]->descriptorBindings) {
						binding.binding = bindingIndexIter++;
						pbindings.push_back(binding);
					}
				}

				VkDescriptorSetLayoutCreateInfo descriptorCreateInfo {
					.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
					.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
					.bindingCount = static_cast<uint32_t>(pbindings.size()),
					.pBindings = pbindings.data()
				};
				vkCreateDescriptorSetLayout(vkdevice.logicalDevice, &descriptorCreateInfo, VK_NULL_HANDLE, &descriptorLayout);
                
                VkPipelineLayoutCreateInfo pipelineLayoutInfo {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                    .pushConstantRangeCount = 0U,
                    .pPushConstantRanges = VK_NULL_HANDLE,
                    .setLayoutCount = 1, .pSetLayouts = &descriptorLayout
                };
                VkResult result = vkCreatePipelineLayout(vkdevice.logicalDevice, &pipelineLayoutInfo, VK_NULL_HANDLE, &pipelineLayout);
                if (result != VK_SUCCESS) return result;

                for(size_t i = 0; i < shader_count; i++) {
                    VkShaderCreateInfoEXT createInfo = {
                        .sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
                        .stage = static_cast<VkShaderStageFlagBits>(shaderStages[i]->stage),
                        .nextStage = static_cast<VkShaderStageFlags>((i < shader_count - 1)?shaderStages[i+1]->stage : XYVK_SHADERSTAGES::UNDEFINED),
                        .codeType = VkShaderCodeTypeEXT::VK_SHADER_CODE_TYPE_SPIRV_EXT,
                        .codeSize = shaderStages[i]->spirv.size() * sizeof(uint32_t),
                        .pCode = shaderStages[i]->spirv.data(),
                        .pName = "main",
                        .setLayoutCount = 1,
                        .pSetLayouts = &descriptorLayout,
                        .pushConstantRangeCount = 0,
                        .pPushConstantRanges = VK_NULL_HANDLE,
                        .pNext = VK_NULL_HANDLE
                    };
                    shaderCreateInfos.push_back(createInfo);
                }
                
				shaderPasses.resize(shader_count);
				return vkCreateShadersEXTXYVK(vkdevice.logicalDevice, static_cast<uint32_t>(shaderCreateInfos.size()), shaderCreateInfos.data(), VK_NULL_HANDLE, shaderPasses.data());
            }
		};
	}
#endif