#pragma once
#ifndef __XYVK_RENDERGRAPH
#define __XYVK_RENDERGRAPH
	#include "./xyvk_engine.hpp"

	namespace XYVK_NAMESPACE {
		using xyvk_renderevent = xyvk_callback<xyvk_subpass&, xyvk_renderobject&, bool>;

        class xyvk_graph : public xyvk_disposable {
		public:
			xyvk_device& vkdevice;
			xyvk_window* window;

			VkFence swapImageInFlight;
			VkSemaphore swapImageAvailable, swapImageFinished, swapImageTimeline;
            
			std::timed_mutex swapChainMutex;
			xyvk_surfaceinfo swapChainPresentDetails;
			VkQueue swapChainPresentQueue;
			VkSwapchainKHR swapChain;
			uint32_t swapFrameIndex;
			std::vector<xyvk_image*> swapChainImages;
			std::vector<xyvk_image*> resizableImages;

			std::atomic_int64_t frameCounter, renderPassCounter;
			std::atomic_bool presentable, refreshable, frameResized;
			std::vector<xyvk_subpass*> renderPasses;
			VkResult initialized = VK_ERROR_INITIALIZATION_FAILED;
			
			xyvk_graph operator=(const xyvk_graph&) = delete;
			xyvk_graph(const xyvk_graph&) = delete;
			~xyvk_graph() { this->Dispose(); }

			void Disposable(bool waitIdle) {
				if (waitIdle) vkdevice.WaitIdle();

				for(xyvk_image* swapImage : swapChainImages) {
					vkDestroyImageView(vkdevice.logicalDevice, swapImage->imageView, VK_NULL_HANDLE);
					delete swapImage;
				}
				
				for(xyvk_subpass* pass : renderPasses) delete pass;

				if (swapChain != VK_NULL_HANDLE) vkDestroySwapchainKHR(vkdevice.logicalDevice, swapChain, VK_NULL_HANDLE);
				if (swapImageAvailable != VK_NULL_HANDLE) vkDestroySemaphore(vkdevice.logicalDevice, swapImageAvailable, VK_NULL_HANDLE);
				if (swapImageFinished != VK_NULL_HANDLE) vkDestroySemaphore(vkdevice.logicalDevice, swapImageFinished, VK_NULL_HANDLE);
				if (swapImageInFlight != VK_NULL_HANDLE) vkDestroyFence(vkdevice.logicalDevice, swapImageInFlight, VK_NULL_HANDLE);
				if (swapImageTimeline != VK_NULL_HANDLE) vkDestroySemaphore(vkdevice.logicalDevice, swapImageTimeline, VK_NULL_HANDLE);
			}

			xyvk_graph(xyvk_device& vkdevice, xyvk_window* window, xyvk_surfaceinfo swapChainPresentDetails = xyvk_surfaceinfo()) : vkdevice(vkdevice), window(window), swapChainPresentDetails(swapChainPresentDetails), presentable(true), refreshable(false), frameResized(false), swapChain(VK_NULL_HANDLE), renderPassCounter(0), frameCounter(0), swapFrameIndex(0) {
				onDispose.hook(xyvk_callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));
				initialized = Initialize();
			}
			
			xyvk_subpass* CreateSubpass(xyvk_cmdpool& cmdpool, XYVK_SUBPASSTYPE subpassType, std::vector<xyvk_shader*> shaderStages, std::string title, std::vector<xyvk_subpass*> dependencies = {}, VkDeviceSize localSubpassIndex = 0ULL, xyvk_dynamicstate dynamicState = xyvk_dynamicstate()) {
				xyvk_subpass* subpass = new xyvk_subpass(vkdevice, cmdpool, subpassType, shaderStages, title, renderPassCounter ++, localSubpassIndex, dynamicState);
				
				for(xyvk_subpass* dependency : dependencies)
					subpass->dependencies.push_back(dependency);
				
				renderPasses.push_back(subpass);
				return subpass;
			}
			
			void ResizeImageWithSwapchain(xyvk_image* resizableImage) {
				bool hasImage = false;

				for(xyvk_image* image : swapChainImages)
					if (image == resizableImage) hasImage = true;

				for(xyvk_image* image : resizableImages)
					if (image == resizableImage) hasImage = true;
				
				if (resizableImage->sourceType != XYVK_IMAGETYPE::SWAPCHAIN && hasImage == false)
					resizableImages.push_back(resizableImage);
			}
			
			void ResizeFrameBuffer(GLFWwindow* hwndWindow, int width, int height) {
				if (width == 0 || height == 0) return;

				for(xyvk_image* swapImage : swapChainImages) {
					vkDestroyImageView(vkdevice.logicalDevice, swapImage->imageView, VK_NULL_HANDLE);
					delete swapImage;
				}

				#if XYVK_VALIDATION
					std::cout << "Resizing Window: " << window->hwndWidth << " : " << window->hwndHeight << " -> " << width << " : " << height << std::endl;
				#endif

				for(xyvk_image* resizableImage : resizableImages) {
					#if XYVK_VALIDATION
						std::cout << "\t" << "Resizing Image: " << resizableImage->width << " : " << resizableImage->height << " -> " << width << " : " << height << std::endl;
					#endif
					resizableImage->Disposable(false);
					resizableImage->CreateImage(resizableImage->sourceType, width, height, resizableImage->rgbaFormat, resizableImage->addressMode, resizableImage->lerpFilter);
				}
				
				VkSwapchainKHR oldSwapChain = swapChain;
				xyvk_swapchain::CreateSwapChainImages(vkdevice, *window, swapChainPresentDetails, swapChain, swapChainImages);
				xyvk_swapchain::CreateSwapChainImageViews(vkdevice, swapChainPresentDetails, swapChainImages);
				vkDestroySwapchainKHR(vkdevice.logicalDevice, oldSwapChain, VK_NULL_HANDLE);

				presentable = true;
				refreshable = false;
				frameResized = true;
			}
			
			VkResult ExecuteRenderGraph() {
				for(xyvk_subpass* pass : renderPasses) {
					pass->cmdpool.ReturnAll();
					pass->timestampIterator = 0;
				}

				VkResult result = VK_SUCCESS;
				for(int32_t i = 0; i < renderPasses.size(); i++) {
					if (renderPasses[i]->subpassType == XYVK_SUBPASSTYPE::PRESENT)
						renderPasses[i]->targetImage = swapChainImages[swapFrameIndex];

					////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					std::pair<VkCommandBuffer, int32_t> cmdbufferPair = renderPasses[i]->BeginCmdBuffer();
					xyvk_renderobject executionObject(renderPasses[i]->pipelineLayout, renderPasses[i]->subpassType, cmdbufferPair);
					renderPasses[i]->renderEvent.invoke(*renderPasses[i], executionObject, static_cast<bool>(frameResized));
					renderPasses[i]->EndCmdBuffer(cmdbufferPair);
					////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					
					VkDeviceSize frameWait = frameCounter * 100;
					VkDeviceSize waitValue = frameWait + renderPasses[i]->timelineWait;
					VkDeviceSize signalValue = frameWait + renderPasses[i]->subpassIndex;
					VkSemaphore initialWaits[] = { swapImageAvailable };
					VkSemaphore dependencyWaits[] { swapImageTimeline };
					VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
					bool isInitialPass = i == 0 || renderPasses[i]->dependencies.size() == 0;

					VkTimelineSemaphoreSubmitInfo timelineInfo = { .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
						.waitSemaphoreValueCount = 1, .pWaitSemaphoreValues = &waitValue, .signalSemaphoreValueCount = 1, .pSignalSemaphoreValues = &signalValue };
					
					VkSubmitInfo submitInfo { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1U, .pCommandBuffers = &cmdbufferPair.first,
						.pWaitDstStageMask = waitStages, .waitSemaphoreCount = static_cast<uint32_t>(isInitialPass), .pWaitSemaphores = (isInitialPass)? initialWaits : dependencyWaits, .pNext = &timelineInfo };
					
					if (renderPasses[i]->subpassType == XYVK_SUBPASSTYPE::PRESENT) {
						VkSemaphore signalSemaphores[] = { swapImageFinished };
						submitInfo.signalSemaphoreCount = 1;
						submitInfo.pSignalSemaphores = signalSemaphores;
						result = vkQueueSubmit(swapChainPresentQueue, 1, &submitInfo, swapImageInFlight);
					} else {
						result = vkQueueSubmit(renderPasses[i]->submitQueue, 1, &submitInfo, VK_NULL_HANDLE);
					}
				}

				return result;
			}

			VkResult RenderSwapChain() {
				VkResult result = VK_NOT_READY;
				if (!presentable || refreshable) {
					ResizeFrameBuffer(window->hwndWindow, window->hwndWidth, window->hwndHeight);
				} else {
					// Double wait to avoid synch-overlap causing command buffers to still be in use from last present...
					xyvk_swapchain::WaitResetFences(vkdevice, &swapImageInFlight);
					VkResult result = xyvk_swapchain::QueryNextSwapChainImage(vkdevice, swapChain, swapFrameIndex, swapImageInFlight, swapImageAvailable);
					xyvk_swapchain::WaitResetFences(vkdevice, &swapImageInFlight);
					
					if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)
						result = ExecuteRenderGraph();
					
					if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)
						result = xyvk_swapchain::QueuePresent(swapChainPresentQueue, swapChain, swapImageFinished, swapFrameIndex);
					
					presentable = (result == VK_SUCCESS);
					frameResized = false;
					frameCounter ++;
				}

				return result;
			}
			
			VkResult Initialize() {
				if (window != VK_NULL_HANDLE) {
					if (!vkdevice.queueFamilyIndices.hasPresentFamily) return VK_ERROR_INITIALIZATION_FAILED;
					vkGetDeviceQueue(vkdevice.logicalDevice, vkdevice.queueFamilyIndices.presentFamily, 0, &swapChainPresentQueue);
					
					xyvk_swapchain::CreateSwapChainImages(vkdevice, *window, swapChainPresentDetails, swapChain, swapChainImages);
					xyvk_swapchain::CreateSwapChainImageViews(vkdevice, swapChainPresentDetails, swapChainImages);

					/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					VkSemaphoreCreateInfo semaphoreCreateInfo { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
					VkFenceCreateInfo fenceCreateInfo { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };

					vkCreateSemaphore(vkdevice.logicalDevice, &semaphoreCreateInfo, VK_NULL_HANDLE, &swapImageAvailable);
					vkCreateSemaphore(vkdevice.logicalDevice, &semaphoreCreateInfo, VK_NULL_HANDLE, &swapImageFinished);
					vkCreateFence(vkdevice.logicalDevice, &fenceCreateInfo, VK_NULL_HANDLE, &swapImageInFlight);
					/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

					VkSemaphoreTypeCreateInfo timelineCreateInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO, .pNext = NULL, .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE, .initialValue = 0 };
					VkSemaphoreCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = &timelineCreateInfo, .flags = 0 };
					vkCreateSemaphore(vkdevice.logicalDevice, &createInfo, NULL, &swapImageTimeline);
				}

				return VK_SUCCESS;
			}
		};
	}
#endif