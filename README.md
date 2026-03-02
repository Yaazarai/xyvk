# XYVK
Simple 2D Vulkan engine relying entirely on dynamic state, dynamic rendering and push descriptors. The goal of this enigne was to simplify the rendering pipeline and rendering interface for the end-user to minimize overhead when rapidly prototyping new projects. All of the rendering order, shader binding index ordering, manually resizing images and render pass GPU timings are all handled by ther engine. This particular implementation leverages a simple iterative-only render graph to avoid circular depenencies.

### RENDER GRAPH
While VkGuide and Vulkan-Tutorial blogs are great for learning the Vulkan pipeline, they don't focus on how to write a proper Vulkan renderer. This allows support for auto-synchronized render passes via a **Render Graph,** which lets you create render passes, determine how they depend on each other and then automatically synchronize those render passes whenrendering and outputting to screen.

The rendering has one key issue you have to always be aware of: circular dependencies. This is when two or more render passes chain into themselves creating a infinite loop. The easy solution with **Render Graphs** is that each renderpass gets a unique iterative ID representing where it occurs in the graph and no renderpass can have dependencies that occur later in the graph.

This also works great with Timeline Semaphores, which we can use to assign each renderpass a place on a timeline and have that renderpass' execution wait until a specific point in the timeline is reached. This way each renderpass waits on a point in the timeline, executes, then updates the timeline for the next renderpass.

### COMPILING & API
Please read the xyvk_engine.hpp file for compiler/linker information. Compiling is extremely trivial relocate the source/library API paths for Vulkan, GLFW and VMA in `_BUILDENV.bat` to those on your local system--install where needed. Make sure that you're utilizing Vulkan `1.3.290.0` or later.

Open the `_BUILDENV.bat` in the project folder (or naviage to that folder via command prompt and execute it there) and then call any of the provided sciprts: for compiling the EXE call `_DEBUG`, `_PRODTEST`, `_RELEASE` and to compile shaders call: `_SHADERS _DEBUG`, `_SHADERS _PRODTEST`, `_SHADERS _RELEASE`. Vulkan by default comes packaged with `glslc` compiler for GLSL/SPIRV shaders.

Here are the following default options for VULKAN / VMA / GLFW / GLM:
```
* #define _DEBUG to enable console & validation a layers.
* #define _RELEASE for an optimized window GUI only build.
* #define _PRODTEST for window GUI + console out (with validation layers disabled).
* You can #define XYVK_NAMESPACE before including TinyVulkan to override the default [tinyvk] namespace.
* The XYVK_VALIDATION macros tell you true/false whether validation layers are enabled.
* GLM forces left-handle axis--top-left is 0,0 (GLM_FORCE_LEFT_HANDED).
* GLM forces depth range zero to one (GLM_FORCE_DEPTH_ZERO_TO_ONE).
* GLM forces memory aligned types (GLM_FORCE_DEFAULT_ALIGNED_GENTYPES).
* Library can be compiled with either native MSVC (Visual Studio) or clang-cl (LLVM MSVC).
```


### SAMPLE PROGRAM
A simple sample program is provided which does the following: load QOI image -> stage render data to GPU -> copy QOI image A to image B -> copy image B to screen image (swapchain iamge) and output to screen.

This requires the following valid sub-folders, compiled shaders and a sample QOI image of any sort. If compiling shaders with `_SHADERS.bat` the shaders will be compiled in these locations by default. No default QOI image is provided (one can be exported by the program Aseprite):
```
#define SHADER_PATH_VERTEX "./Shaders/default_output_vert.spv"
#define SHADER_PATH_FRAGMENT "./Shaders/default_output_frag.spv"
#define SHADER_PATH_TEXTUREOUT "./Shaders/texture_output_frag.spv"

#define QOI_IMPLEMENTATION
#include "./Vulkan2D-XYVK/Externals/qoi.h"
#define DEFAULT_QOI_IMAGE "./Images/icons_default.qoi"
```

The full implementation is exactly 100 lines long:
```C++
#include "./Vulkan2D-XYVK/xyvk_engine.hpp"
using namespace xyvk;
using namespace glm;

#define SHADER_PATH_VERTEX "./Shaders/default_output_vert.spv"
#define SHADER_PATH_FRAGMENT "./Shaders/default_output_frag.spv"
#define SHADER_PATH_TEXTUREOUT "./Shaders/texture_output_frag.spv"

#define QOI_IMPLEMENTATION
#include "./Vulkan2D-XYVK/Externals/qoi.h"
#define DEFAULT_QOI_IMAGE "./Images/icons_default.qoi"

int XYVK_WINDOWMAIN {
    // Define the entire Vulkan system and GLFW window context, then intialize the rendergraph:
    xyvk_window window("xyvk-engine", 1920, 1080, true, true, false);
    xyvk_device vkdevice(window);
    xyvk_cmdpool cmdpool(vkdevice);
    xyvk_graph renderGraph(vkdevice, &window, xyvk_surfaceinfo());

    // Declare our shaders and their descriptor binding layouts (note: binding indices are treated as continuously iterative from one shader to the next: vertex -> fragment):
    xyvk_shader vertexShader(vkdevice, XYVK_SHADERSTAGES::VERTEX, SHADER_PATH_VERTEX, { XYVK_DESCRIPTORTYPE::UNIFORM_BUFFER });
    xyvk_shader fragmentShader(vkdevice, XYVK_SHADERSTAGES::FRAGMENT, SHADER_PATH_TEXTUREOUT, { XYVK_DESCRIPTORTYPE::IMAGE_SAMPLER });
    xyvk_shader textureShader(vkdevice, XYVK_SHADERSTAGES::FRAGMENT, SHADER_PATH_TEXTUREOUT, { XYVK_DESCRIPTORTYPE::IMAGE_SAMPLER });
    
    // Declare render passes and set their dependencies:
    xyvk_subpass* transferPass = renderGraph.CreateSubpass(cmdpool, XYVK_SUBPASSTYPE::TRANSFER, {}, "Staging Data");
    xyvk_subpass* fragmentPass = renderGraph.CreateSubpass(cmdpool, XYVK_SUBPASSTYPE::FRAGMENT, { &vertexShader, &textureShader }, "Render Scene", { transferPass });
    xyvk_subpass* swapchainPass = renderGraph.CreateSubpass(cmdpool, XYVK_SUBPASSTYPE::PRESENT, { &vertexShader, &textureShader }, "Screen Output", { fragmentPass });
    
    // Create a rendewr target image for the fragmentPass and dynamically resize it with the window:
    xyvk_image targetImage(vkdevice, XYVK_IMAGETYPE::ATTACHEMENT, window.hwndWidth, window.hwndHeight);
    fragmentPass->SetTargetImage(&targetImage);
    renderGraph.ResizeImageWithSwapchain(&targetImage);

    // Import a sample QOI formatted image:
    qoi_desc sourceImageDesc;
    void* sourceImageData = qoi_read(DEFAULT_QOI_IMAGE, &sourceImageDesc, 4);
    xyvk_image sourceImage(vkdevice, XYVK_IMAGETYPE::ATTACHEMENT, sourceImageDesc.width, sourceImageDesc.height);
    
    // Create two 6-vertex (non-indexed) quads, one for the fragmentPass and one for swapchainPass:
    xyvk_vertexquad imageQuad(vec2(sourceImageDesc.width, sourceImageDesc.height), 1.0, vec2(0.0, 0.0), vec2(0.0, 0.0), vec2(0.0, 0.0), 0.0, vec4(0.0, 0.0, 1.0, 1.0));
    xyvk_vertexquad screenQuad(vec2(window.hwndWidth, window.hwndHeight), 1.0, vec2(0.0, 0.0), vec2(0.0, 0.0), vec2(0.0, 0.0), 0.0, vec4(0.0, 0.0, 1.0, 1.0));

    // Create the copy buffers for copying the camera projection matrix, render quads and images to the GPU:
    size_t sizeofQuads = imageQuad.SizeofQuad() + screenQuad.SizeofQuad();
    size_t sizeOfImage = sourceImageDesc.width * sourceImageDesc.height * sourceImageDesc.channels;
    xyvk_buffer vertexBuffer(vkdevice, XYVK_BUFFERTYPE::VERTEX, sizeofQuads);
    xyvk_buffer stagingBuffer(vkdevice, XYVK_BUFFERTYPE::STAGING, sizeofQuads + sizeof(glm::mat4) + sizeOfImage);
    xyvk_buffer cameraBuffer(vkdevice, XYVK_BUFFERTYPE::UNIFORM, sizeof(glm::mat4));

    // Define the camera matrix:
    glm::mat4 camera = xyvk_vertexmath::Project2D(window.hwndWidth, window.hwndHeight, 0.0, 0.0, 1.0, 0.0);
    
    // Create a render event callback for staging all of the information to the GPU for subsequent render passes: (NOTE: this is inefficient and may only need to be done once for static resources such as images loaded from disc)
    transferPass->renderEvent.hook(xyvk_renderevent([&](xyvk_subpass& renderPass, xyvk_renderobject& renderer, bool frameResized) {
        camera = xyvk_vertexmath::Project2D(window.hwndWidth, window.hwndHeight, 0.0, 0.0, 1.0, 0.0);
        screenQuad.Resize(vec2(window.hwndWidth, window.hwndHeight));
        std::vector<xyvk_vertex> quads = xyvk_vertexquad::GetVertexVector({ imageQuad.Vertices(), screenQuad.Vertices() });
        VkDeviceSize byteSize = sourceImageDesc.width * sourceImageDesc.height * sourceImageDesc.channels;

        renderer.StageBufferToBuffer(stagingBuffer, vertexBuffer, quads.data(), sizeofQuads);
        renderer.StageBufferToBuffer(stagingBuffer, cameraBuffer, &camera, sizeof(glm::mat4));
        renderer.StageBufferToImage(stagingBuffer, sourceImage, sourceImageData, { .extent = { sourceImageDesc.width, sourceImageDesc.height}, .offset = {0, 0} }, byteSize);
    }));
    
    // Create a render event callback to render a image + quad:
    fragmentPass->renderEvent.hook(xyvk_renderevent([&](xyvk_subpass& renderPass, xyvk_renderobject& renderer, bool frameResized) {
        renderer.PushBuffer(cameraBuffer);
        renderer.PushImage(sourceImage);
        renderer.BindAndDraw(vertexBuffer, 6, 1, 0, 0);
    }));

    // Create a render event callback to render a image + quad (for putting to screen):
    swapchainPass->renderEvent.hook(xyvk_renderevent([&](xyvk_subpass& renderPass, xyvk_renderobject& renderer, bool frameResized) {
        renderer.PushBuffer(cameraBuffer);
        renderer.PushImage(*renderPass.dependencies[0]->targetImage);
        renderer.BindAndDraw(vertexBuffer, 6, 1, 0, 0);
    }));
    
    // Perform rendering on a separate thread to avoid hanging when the user is interacting with the window (drag, resize, etc.):
    std::thread mythread([&])[] {
        while (window.ShouldExecute()) {
            renderGraph.RenderSwapChain();

            // If debugging output renderpass timings to console:
            #if XYVK_VALIDATION
                for(xyvk_subpass* pass : renderGraph.renderPasses)
                    pass->QueryTimeStampsOutput(renderGraph.frameCounter);
            #endif
        }
    });

    // Execute the entire program: have the main thread sleep when waiting on user input events to minimize CPU overhead (may not be optimal for games)
    window.WhileMain(XYVK_WINDOWEVENTS::WAIT_EVENTS);

    // Close out the program and wait on the GPU to finish executing before terminating:
    mythread.join();
    vkdevice.WaitIdle();
    return VK_SUCCESS;
};
```
