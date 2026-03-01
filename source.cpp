#include "./Vulkan2D-XYVK/xyvk_engine.hpp"
using namespace xyvk;
using namespace glm;

// NOTE: Binding indices are shared between vertex/fragment/compute shader pairs.
#define SHADER_PATH_VERTEX "./Shaders/default_output_vert.spv"
#define SHADER_PATH_FRAGMENT "./Shaders/default_output_frag.spv"
#define SHADER_PATH_TEXTUREOUT "./Shaders/texture_output_frag.spv"

#define QOI_IMPLEMENTATION
#include "./Vulkan2D-XYVK/Externals/qoi.h"
#define DEFAULT_QOI_IMAGE "./Images/icons_default.qoi"

int XYVK_WINDOWMAIN {
    xyvk_window window("xyvk-engine", 1920, 1080, true, true, false);
    xyvk_device vkdevice(window);
    xyvk_cmdpool cmdpool(vkdevice);
    xyvk_graph renderGraph(vkdevice, &window, xyvk_surfaceinfo());

    xyvk_shader vertexShader(vkdevice, XYVK_SHADERSTAGES::VERTEX, SHADER_PATH_VERTEX, { XYVK_DESCRIPTORTYPE::UNIFORM_BUFFER });
    xyvk_shader fragmentShader(vkdevice, XYVK_SHADERSTAGES::FRAGMENT, SHADER_PATH_TEXTUREOUT, { XYVK_DESCRIPTORTYPE::IMAGE_SAMPLER });
    xyvk_shader textureShader(vkdevice, XYVK_SHADERSTAGES::FRAGMENT, SHADER_PATH_TEXTUREOUT, { XYVK_DESCRIPTORTYPE::IMAGE_SAMPLER });
    
    xyvk_subpass* transferPass = renderGraph.CreateSubpass(cmdpool, XYVK_SUBPASSTYPE::TRANSFER, {}, "Staging Data");
    xyvk_subpass* fragmentPass = renderGraph.CreateSubpass(cmdpool, XYVK_SUBPASSTYPE::FRAGMENT, { &vertexShader, &textureShader }, "Render Scene", { transferPass });
    xyvk_subpass* swapchainPass = renderGraph.CreateSubpass(cmdpool, XYVK_SUBPASSTYPE::PRESENT, { &vertexShader, &textureShader }, "Screen Output", { fragmentPass });
    
    xyvk_image targetImage(vkdevice, XYVK_IMAGETYPE::ATTACHEMENT, window.hwndWidth, window.hwndHeight);
    fragmentPass->SetTargetImage(&targetImage);
    renderGraph.ResizeImageWithSwapchain(&targetImage);

    qoi_desc sourceImageDesc;
    void* sourceImageData = qoi_read(DEFAULT_QOI_IMAGE, &sourceImageDesc, 4);
    xyvk_image sourceImage(vkdevice, XYVK_IMAGETYPE::ATTACHEMENT, sourceImageDesc.width, sourceImageDesc.height);
    
    xyvk_vertexquad imageQuad(vec2(sourceImageDesc.width, sourceImageDesc.height), 1.0, vec2(0.0, 0.0), vec2(0.0, 0.0), vec2(0.0, 0.0), 0.0, vec4(0.0, 0.0, 1.0, 1.0));
    xyvk_vertexquad screenQuad(vec2(window.hwndWidth, window.hwndHeight), 1.0, vec2(0.0, 0.0), vec2(0.0, 0.0), vec2(0.0, 0.0), 0.0, vec4(0.0, 0.0, 1.0, 1.0));

    size_t sizeofQuads = imageQuad.SizeofQuad() + screenQuad.SizeofQuad();
    size_t sizeOfImage = sourceImageDesc.width * sourceImageDesc.height * sourceImageDesc.channels;
    xyvk_buffer vertexBuffer(vkdevice, XYVK_BUFFERTYPE::VERTEX, sizeofQuads);
    xyvk_buffer stagingBuffer(vkdevice, XYVK_BUFFERTYPE::STAGING, sizeofQuads + sizeof(glm::mat4) + sizeOfImage);
    xyvk_buffer cameraBuffer(vkdevice, XYVK_BUFFERTYPE::UNIFORM, sizeof(glm::mat4));

    glm::mat4 camera = xyvk_vertexmath::Project2D(window.hwndWidth, window.hwndHeight, 0.0, 0.0, 1.0, 0.0);
    transferPass->renderEvent.hook(xyvk_renderevent([&](xyvk_subpass& renderPass, xyvk_renderobject& renderer, bool frameResized) {
        camera = xyvk_vertexmath::Project2D(window.hwndWidth, window.hwndHeight, 0.0, 0.0, 1.0, 0.0);
        screenQuad.Resize(vec2(window.hwndWidth, window.hwndHeight));
        std::vector<xyvk_vertex> quads = xyvk_vertexquad::GetVertexVector({ imageQuad.Vertices(), screenQuad.Vertices() });
        VkDeviceSize byteSize = sourceImageDesc.width * sourceImageDesc.height * sourceImageDesc.channels;

        renderer.StageBufferToBuffer(stagingBuffer, vertexBuffer, quads.data(), sizeofQuads);
        renderer.StageBufferToBuffer(stagingBuffer, cameraBuffer, &camera, sizeof(glm::mat4));
        renderer.StageBufferToImage(stagingBuffer, sourceImage, sourceImageData, { .extent = { sourceImageDesc.width, sourceImageDesc.height}, .offset = {0, 0} }, byteSize);
    }));
    
    fragmentPass->renderEvent.hook(xyvk_renderevent([&](xyvk_subpass& renderPass, xyvk_renderobject& renderer, bool frameResized) {
        renderer.PushBuffer(cameraBuffer);
        renderer.PushImage(sourceImage);
        renderer.BindAndDraw(vertexBuffer, 6, 1, 0, 0);
    }));
    
    swapchainPass->renderEvent.hook(xyvk_renderevent([&](xyvk_subpass& renderPass, xyvk_renderobject& renderer, bool frameResized) {
        renderer.PushBuffer(cameraBuffer);
        renderer.PushImage(*renderPass.dependencies[0]->targetImage);
        renderer.BindAndDraw(vertexBuffer, 6, 1, 0, 0);
    }));
    
    std::thread mythread([&]() {
        while (window.ShouldExecute()) {
            renderGraph.RenderSwapChain();

            #if XYVK_VALIDATION
                for(xyvk_subpass* pass : renderGraph.renderPasses)
                    pass->QueryTimeStampsOutput(renderGraph.frameCounter);
            #endif
        }
    });

    window.WhileMain(XYVK_WINDOWEVENTS::WAIT_EVENTS);
    mythread.join();
    vkdevice.WaitIdle();
    return VK_SUCCESS;
};