#pragma once
#ifndef __XYVK_WINDOW
#define __XYVK_WINDOW
    #include "./xyvk_engine.hpp"

    namespace XYVK_NAMESPACE {
		class xyvk_window : public xyvk_disposable {
		public:
            bool hwndResizable, hwndMinSize, hwndBordered, hwndFullscreen;
			int hwndWidth, hwndHeight, hwndXpos, hwndYpos, minWidth, minHeight;
            std::string hwndTitle;
            GLFWwindow* hwndWindow;
			VkResult initialized = VK_ERROR_INITIALIZATION_FAILED;

			xyvk_invoker<std::atomic_bool&> onWhileMain;
			inline static xyvk_invoker<GLFWwindow*, int, int> onWindowResized;
			inline static xyvk_invoker<GLFWwindow*, int, int> onWindowPositionMoved;
			inline static xyvk_invoker<GLFWwindow*, int, int> onResizeFrameBuffer;
			
			xyvk_window(const xyvk_window&) = delete;
			xyvk_window& operator=(const xyvk_window&) = delete;
			~xyvk_window() { this->Dispose(); }

			void Disposable(bool waitIdle) {
				glfwDestroyWindow(hwndWindow);
				glfwTerminate();
			}

			xyvk_window(std::string title, int width, int height, bool resizable,  bool bordered = true, bool fullscreen = false, bool hasMinSize = false, int minWidth = 200, int minHeight = 200)
			: hwndResizable(resizable), hwndBordered(bordered), hwndFullscreen(fullscreen), hwndMinSize(hasMinSize), hwndWidth(width), hwndHeight(height), minWidth(minWidth), minHeight(minHeight), hwndTitle(title), hwndWindow(VK_NULL_HANDLE) {
				onDispose.hook(xyvk_callback<bool>([this](bool forceDispose){this->Disposable(forceDispose); }));
				onWindowResized.hook(xyvk_callback<GLFWwindow*, int, int>([this](GLFWwindow* hwnd, int width, int height) { if (hwnd != hwndWindow) return; hwndWidth = width; hwndHeight = height; }));
				initialized = Initialize();
			}

			inline static void OnFrameBufferNotifyReSizeCallback(GLFWwindow* hwnd, int width, int height) {
				onResizeFrameBuffer.invoke(hwnd, width, height);
				onWindowResized.invoke(hwnd, width, height);
			}

			void OnFrameBufferReSizeCallback(int& width, int& height) {
				width = 0;
				height = 0;

				while (width <= 0 || height <= 0)
					glfwGetFramebufferSize(hwndWindow, &width, &height);

				hwndWidth = width;
				hwndHeight = height;
			}
			
			bool ShouldExecute() {
                return glfwWindowShouldClose(hwndWindow) != GLFW_TRUE;
            }

			VkSurfaceKHR CreateWindowSurface(VkInstance instance) {
				VkSurfaceKHR wndSurface;
                VkResult result = glfwCreateWindowSurface(instance, hwndWindow, VK_NULL_HANDLE, &wndSurface);
				return (result == VK_SUCCESS)? wndSurface : VK_NULL_HANDLE;
			}
            
			inline static std::vector<const char*> QueryRequiredExtensions() {
				glfwInit();
				uint32_t glfwExtensionCount = 0;
				const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
				return std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);
			}

			void WhileMain(XYVK_WINDOWEVENTS eventType) {
				std::atomic_bool close = true;
				while (close = ShouldExecute()) {
					onWhileMain.invoke(close);
					if (eventType == XYVK_WINDOWEVENTS::POLL_EVENTS)
					{ glfwPollEvents(); } else { glfwWaitEvents(); }
				}
			}
			
			VkResult Initialize() {
				glfwInit();

                if (glfwVulkanSupported()) {
                    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
                    glfwWindowHint(GLFW_RESIZABLE, (hwndResizable) ? GLFW_TRUE : GLFW_FALSE);
					
					hwndWindow = glfwCreateWindow(hwndWidth, hwndHeight, hwndTitle.c_str(), VK_NULL_HANDLE, VK_NULL_HANDLE);
                    glfwSetWindowUserPointer(hwndWindow, this);
                    glfwSetFramebufferSizeCallback(hwndWindow, xyvk_window::OnFrameBufferNotifyReSizeCallback);
					glfwSetWindowAttrib(hwndWindow, GLFW_DECORATED, (hwndBordered) ? GLFW_TRUE : GLFW_FALSE);

                    if (hwndMinSize) glfwSetWindowSizeLimits(hwndWindow, minWidth, minHeight, GLFW_DONT_CARE, GLFW_DONT_CARE);
                    return VK_SUCCESS;
                }

                return VK_ERROR_INITIALIZATION_FAILED;
            }
        };
    }
#endif