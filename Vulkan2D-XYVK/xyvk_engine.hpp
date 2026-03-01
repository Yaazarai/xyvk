#pragma once
#ifndef __XYVK_ENGINE
#define __XYVK_ENGINE

    ///   XYVK VULKAN2D LIBRARY DEPENDENCIES: VULKAN, GLFW and VMA compiled library binaries.
    /// 
    ///    C/C++ | Code Configuration | Runtime Libraries:
    ///        RELEASE: /MD
    ///        DEBUG  : /mtD
    ///    C/C++ | General | Additional Include Directories: (DEBUG & RELEASE)
    ///        ..\VulkanSDK\1.3.239.0\Include;...\glfw-3.3.7.bin.WIN64\glfw-3.3.7.bin.WIN64\include;
    ///    Linker | General | Additional Library Directories: (DEBUG & RELEASE)
    ///        ...\glfw-3.3.7.bin.WIN64\lib-vc2022;$(VULKAN_SDK)\Lib; // C:\VulkanSDK\1.3.239.0\Lib
    ///    Linker | Input | Additional Dependencies:
    ///        RELEASE: vulkan-1.lib;glfw3.lib;
    ///        DEBUG  : vulkan-1.lib;glfw3_mt.lib;
    ///    Linker | System | SubSystem
    ///        RELEASE: Windows (/SUBSYSTEM:WINDOWS)
    ///        RELEASE: Console (/SUBSYSTEM:CONSOLE)
    ///        DEBUG  : Console (/SUBSYSTEM:CONSOLE)
    /// 
    ///    * If Debug mode is enabled the console window will be visible, but not on Release.
    ///    * If compiled as RELEASE with console enabled compile with /subsystem:console.
    ///    * Make sure you've installed the SDK bionaries for Vulkan, GLFW and VMA.
    ///    
    ///    VULKAN DEVICE EXTENSIONS:
    ///         Dynamic Rendering Dependency.
    ///              VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME
    ///         Depth fragment testing (discard fragments if fail depth test, if enabled in the graphics pipeline).
    ///              VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME
    ///     	   Allows for rendering without framebuffers and render passes.
    ///              VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
    ///         Allows for writing descriptors directly into a command buffer rather than allocating from sets / pools.
    ///              VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME
    ///         Swapchain support for buffering frame images with the device driver to reduce tearing.
    ///         Only gets added if a window is added to the VkInstance on create via call to glfwGetRequiredInstanceExtensions.
    ///            * VK_KHR_SWAPCHAIN_EXTENSION_NAME

    #define GLFW_INCLUDE_VULKAN
    #if defined (_WIN32)
        #define GLFW_EXPOSE_NATIVE_WIN32
        #define VK_USE_PLATFORM_WIN32_KHR
    #endif
    #include <GLFW/glfw3.h>
    #include <GLFW/glfw3native.h>
    #include <vulkan/vulkan.h>
    
    #ifdef _DEBUG
        #define VK_LAYER_KHRONOS_EXTENSION_NAME "VK_LAYER_KHRONOS_validation"
        #define XYVK_WINDOWMAIN main(int argc, char* argv[])
        #define XYVK_VALIDATION VK_TRUE
    #else
        #define XYVK_VALIDATION VK_FALSE
        #ifdef _RELEASE
			/// COMPILING WITH CLANG-CL / LLVM
            #define XYVK_WINDOWMAIN __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
        #else
            /// For debugging w/ Console on Release change from /WINDOW to /CONSOLE: Linker -> System -> Subsystem.
            #define XYVK_WINDOWMAIN main(int argc, char* argv[])
        #endif
    #endif
    
    #ifndef c
        #define XYVK_NAMESPACE xyvk
        namespace XYVK_NAMESPACE {}
    #endif
    #define XYVK_MAKE_VERSION(major, minor, patch) VK_MAKE_API_VERSION(0, major, minor, patch)
    #define XYVK_ENGINE_VERSION XYVK_MAKE_VERSION(1, 0, 0)
    #define XYVK_ENGINE_NAME "XYVK_ENGINE"
    
    ///
    /// Enables VMA automated GPU memory management.
    ///
    #define VMA_IMPLEMENTATION
    #define VMA_DEBUG_GLOBAL_MUTEX VK_TRUE
    #define VMA_USE_STL_CONTAINERS VK_TRUE
    #define VMA_RECORDING_ENABLED XYVK_VALIDATION
    #include <vma/vk_mem_alloc.h>

    ///
    /// Enables GLM (Standardized Math via OpenGL Mathematics, allows for byte aligned memory).
    ///
    #define GLM_FORCE_RADIANS
    #define GLM_FORCE_LEFT_HANDED
    #define GLM_FORCE_DEPTH_ZERO_TO_ONE
    #define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
    #include <glm/glm.hpp>
    #include <glm/ext.hpp>
    
    ///
    /// General include libraries (data-structs, for-each search, etc.).
    ///
    #include <mutex>
    #include <fstream>
    #include <iostream>
    #include <vector>
    #include <array>
    #include <set>
    #include <string>
    #include <algorithm>
    #include <functional>
    #include <utility>

    #pragma region XYVK_SUPPORT
        #include "./xyvk_enums.hpp"
        #include "./xyvk_support.hpp"
        #include "./xyvk_timedlock.hpp"
        #include "./xyvk_callbacks.hpp"
        #include "./xyvk_disposable.hpp"
    #pragma endregion
    #pragma region XYVK_INITIALIZATION
        #include "./xyvk_window.hpp"
        #include "./xyvk_device.hpp"
        #include "./xyvk_cmdpool.hpp"
    #pragma endregion
    #pragma region XYVK_RENDERING
        #include "./xyvk_buffer.hpp"
        #include "./xyvk_image.hpp"
        #include "./xyvk_swapchain.hpp"
    #pragma endregion
    #pragma region XYVK_RENDERGRAPH
        #include "./xyvk_shader.hpp"
        #include "./xyvk_subpass.hpp"
        #include "./xyvk_vertexquad.hpp"
        #include "./xyvk_graph.hpp"
    #pragma endregion
#endif