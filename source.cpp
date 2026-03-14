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

struct myframe_createinfos { size_t camera, quad1, quad2, image_width, image_height, image_bitdepth; };

class myframe : public xyvk_disposable {
public:
    xyvk_device& vkdevice;
    xyvk_buffer* stagingBuffer = VK_NULL_HANDLE;
    xyvk_buffer* cameraBuffer1 = VK_NULL_HANDLE;
    xyvk_buffer* cameraBuffer2 = VK_NULL_HANDLE;
    xyvk_buffer* vertexBuffer = VK_NULL_HANDLE;
    xyvk_image* discImage = VK_NULL_HANDLE;

    myframe operator=(const myframe&) = delete;
    myframe(const myframe&) = delete;
    ~myframe() { this->Dispose(); }

    void Disposable(bool waitIdle) {
        if (waitIdle) vkdevice.WaitIdle();
        if (stagingBuffer != VK_NULL_HANDLE) delete stagingBuffer;
        if (cameraBuffer1 != VK_NULL_HANDLE) delete cameraBuffer1;
        if (cameraBuffer2 != VK_NULL_HANDLE) delete cameraBuffer2;
        if (vertexBuffer != VK_NULL_HANDLE) delete vertexBuffer;
        if (discImage != VK_NULL_HANDLE) delete discImage;
    }

    myframe(xyvk_device& vkdevice, myframe_createinfos createInfos) : vkdevice(vkdevice) {
        onDispose.hook(xyvk_callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));
        VkDeviceSize stagingSize = createInfos.camera + createInfos.camera + createInfos.quad1 + createInfos.quad2 + (createInfos.image_width * createInfos.image_height * createInfos.image_bitdepth);
        stagingBuffer = new xyvk_buffer(vkdevice, XYVK_BUFFERTYPE::STAGING, stagingSize);
        cameraBuffer1 = new xyvk_buffer(vkdevice, XYVK_BUFFERTYPE::UNIFORM, createInfos.camera);
        cameraBuffer2 = new xyvk_buffer(vkdevice, XYVK_BUFFERTYPE::UNIFORM, createInfos.camera);
        vertexBuffer = new xyvk_buffer(vkdevice, XYVK_BUFFERTYPE::VERTEX, createInfos.quad1 + createInfos.quad2);
        discImage = new xyvk_image(vkdevice, XYVK_IMAGETYPE::ATTACHEMENT, createInfos.image_width, createInfos.image_height);
    }
};

int XYVK_WINDOWMAIN {
    xyvk_window window("xyvk-engine", 1920, 1080, true, true, false);
    xyvk_device vkdevice(window);
    xyvk_graph renderGraph(vkdevice, &window, xyvk_surfaceinfo());

    xyvk_shader vertexShader(vkdevice, XYVK_SHADERSTAGES::VERTEX, SHADER_PATH_VERTEX, { XYVK_DESCRIPTORTYPE::UNIFORM_BUFFER });
    xyvk_shader fragmentShader(vkdevice, XYVK_SHADERSTAGES::FRAGMENT, SHADER_PATH_FRAGMENT, { });
    xyvk_shader textureShader(vkdevice, XYVK_SHADERSTAGES::FRAGMENT, SHADER_PATH_TEXTUREOUT, { XYVK_DESCRIPTORTYPE::IMAGE_SAMPLER, XYVK_DESCRIPTORTYPE::IMAGE_SAMPLER });
    
    xyvk_subpass* transferPass = renderGraph.CreateSubpass(XYVK_SUBPASSTYPE::TRANSFER, {}, "Staging Data 1");
    xyvk_subpass* fragmentPass = renderGraph.CreateSubpass(XYVK_SUBPASSTYPE::FRAGMENT, { &vertexShader, &fragmentShader }, "Render Scene", { transferPass }, 0, xyvk_dynamicstate(VK_FALSE, VK_TRUE));
    xyvk_subpass* transferPass2 = renderGraph.CreateSubpass(XYVK_SUBPASSTYPE::TRANSFER, {}, "Staging Data 2", { transferPass, fragmentPass });
    xyvk_subpass* fragmentPass2 = renderGraph.CreateSubpass(XYVK_SUBPASSTYPE::FRAGMENT, { &vertexShader, &fragmentShader }, "Render Scene", { transferPass }, 0, xyvk_dynamicstate(VK_FALSE, VK_TRUE));
    xyvk_subpass* swapchainPass = renderGraph.CreateSubpass(XYVK_SUBPASSTYPE::PRESENT, { &vertexShader, &textureShader }, "Screen Output", { fragmentPass, fragmentPass2, transferPass2 }, 0, xyvk_dynamicstate(VK_FALSE, VK_TRUE));

    qoi_desc sourceImageDesc;
    void* sourceImageData = qoi_read(DEFAULT_QOI_IMAGE, &sourceImageDesc, 4);
    xyvk_image targetImage(vkdevice, XYVK_IMAGETYPE::ATTACHEMENT, sourceImageDesc.width, sourceImageDesc.height);
    fragmentPass->SetTargetImage(&targetImage);
    xyvk_image targetImage2(vkdevice, XYVK_IMAGETYPE::ATTACHEMENT, sourceImageDesc.width, sourceImageDesc.height);
    fragmentPass2->SetTargetImage(&targetImage2);

    xyvk_vertexquad imageQuad(vec2(sourceImageDesc.width, sourceImageDesc.height), 1.0, glm::vec2(0.0, 0.0));
    xyvk_vertexquad screenQuad(vec2(window.hwndWidth, window.hwndHeight), 1.0, glm::vec2(0.0, 0.0));
    imageQuad.VerticesColor(vec4(1.0, 0.0, 1.0, 1.0));

    size_t sizeofQuads = imageQuad.SizeofQuad() + screenQuad.SizeofQuad();
    size_t sizeOfImage = sourceImageDesc.width * sourceImageDesc.height * sourceImageDesc.channels;

    myframe_createinfos frameCreateInfos = { sizeof(glm::mat4), imageQuad.SizeofQuad(), screenQuad.SizeofQuad(), sourceImageDesc.width, sourceImageDesc.height, sourceImageDesc.channels };
    xyvk_frame<myframe, myframe_createinfos> frameManager(renderGraph, frameCreateInfos);

    glm::mat4 camera = xyvk_vertexmath::Project2D(window.hwndWidth, window.hwndHeight, 0.0, 0.0);
    transferPass->renderEvent.hook(xyvk_renderevent([&](xyvk_subpass& renderPass, xyvk_renderobject& renderer, bool frameResized) {
        std::vector<xyvk_vertex> quads = xyvk_vertexquad::GetVertexVector({ imageQuad.Vertices(), screenQuad.Vertices() });
        VkDeviceSize byteSize = sourceImageDesc.width * sourceImageDesc.height * sourceImageDesc.channels;

        std::pair<myframe*, size_t> currentFrame = frameManager.GetCurrentFrame();
        xyvk_buffer& stagingBuffer = *currentFrame.first->stagingBuffer;
        xyvk_buffer& vertexBuffer = *currentFrame.first->vertexBuffer;
        xyvk_buffer& cameraBuffer1 = *currentFrame.first->cameraBuffer1;
        xyvk_buffer& cameraBuffer2 = *currentFrame.first->cameraBuffer2;
        xyvk_image& sourceImage = *currentFrame.first->discImage;
        
        renderer.StageBufferToBuffer(stagingBuffer, vertexBuffer, quads.data(), sizeofQuads);
        camera = xyvk_vertexmath::Project2D(sourceImageDesc.width, sourceImageDesc.height, 0.0, 0.0);
        renderer.StageBufferToBuffer(stagingBuffer, cameraBuffer1, &camera, sizeof(glm::mat4));
        camera = xyvk_vertexmath::Project2D(window.hwndWidth, window.hwndHeight, 0.0, 0.0);
        renderer.StageBufferToBuffer(stagingBuffer, cameraBuffer2, &camera, sizeof(glm::mat4));
        renderer.StageBufferToImage(stagingBuffer, sourceImage, sourceImageData, { .extent = { sourceImageDesc.width, sourceImageDesc.height}, .offset = {0, 0} }, byteSize);
    }));
    
    fragmentPass->renderEvent.hook(xyvk_renderevent([&](xyvk_subpass& renderPass, xyvk_renderobject& renderer, bool frameResized) {
        std::pair<myframe*, size_t> currentFrame = frameManager.GetCurrentFrame();
        xyvk_buffer& cameraBuffer = *currentFrame.first->cameraBuffer1;
        xyvk_buffer& vertexBuffer = *currentFrame.first->vertexBuffer;

        renderer.PushBuffer(cameraBuffer);
        renderer.BindAndDraw(vertexBuffer, 6, 1, 0, 0);
    }));
    
    fragmentPass2->renderEvent.hook(xyvk_renderevent([&](xyvk_subpass& renderPass, xyvk_renderobject& renderer, bool frameResized) {
        std::pair<myframe*, size_t> currentFrame = frameManager.GetCurrentFrame();
        xyvk_buffer& cameraBuffer = *currentFrame.first->cameraBuffer1;
        xyvk_buffer& vertexBuffer = *currentFrame.first->vertexBuffer;

        renderer.PushBuffer(cameraBuffer);
        renderer.BindAndDraw(vertexBuffer, 6, 1, 0, 0);
    }));
    
    transferPass2->renderEvent.hook(xyvk_renderevent([&](xyvk_subpass& renderPass, xyvk_renderobject& renderer, bool frameResized) {
        std::vector<xyvk_vertex> quads = xyvk_vertexquad::GetVertexVector({ imageQuad.Vertices(), screenQuad.Vertices() });
        VkDeviceSize byteSize = sourceImageDesc.width * sourceImageDesc.height * sourceImageDesc.channels;

        std::pair<myframe*, size_t> currentFrame = frameManager.GetCurrentFrame();
        xyvk_buffer& stagingBuffer = *currentFrame.first->stagingBuffer;
        xyvk_buffer& cameraBuffer1 = *currentFrame.first->cameraBuffer1;
        xyvk_buffer& cameraBuffer2 = *currentFrame.first->cameraBuffer2;
        xyvk_buffer& vertexBuffer = *currentFrame.first->vertexBuffer;
        xyvk_image& sourceImage = *currentFrame.first->discImage;
        
        renderer.StageBufferToBuffer(stagingBuffer, vertexBuffer, quads.data(), sizeofQuads);
        camera = xyvk_vertexmath::Project2D(sourceImageDesc.width, sourceImageDesc.height, 0.0, 0.0);
        renderer.StageBufferToBuffer(stagingBuffer, cameraBuffer1, &camera, sizeof(glm::mat4));
        camera = xyvk_vertexmath::Project2D(window.hwndWidth, window.hwndHeight, 0.0, 0.0);
        renderer.StageBufferToBuffer(stagingBuffer, cameraBuffer2, &camera, sizeof(glm::mat4));
        renderer.StageBufferToImage(stagingBuffer, sourceImage, sourceImageData, { .extent = { sourceImageDesc.width, sourceImageDesc.height}, .offset = {0, 0} }, byteSize);
    }));
    
    swapchainPass->renderEvent.hook(xyvk_renderevent([&](xyvk_subpass& renderPass, xyvk_renderobject& renderer, bool frameResized) {
        std::pair<myframe*, size_t> currentFrame = frameManager.GetCurrentFrame();
        xyvk_buffer& cameraBuffer = *currentFrame.first->cameraBuffer2;
        xyvk_buffer& vertexBuffer = *currentFrame.first->vertexBuffer;
        xyvk_image& sourceImage = *currentFrame.first->discImage;

        renderer.PushBuffer(cameraBuffer);
        renderer.PushImage(sourceImage);
        renderer.PushImage(targetImage);
        renderer.BindAndDraw(vertexBuffer, 6, 1, 0, 0);
    }));
    
    std::thread mythread([&]() {
        while (window.ShouldExecute()) {
            renderGraph.RenderSwapChain();
            
            //#if XYVK_VALIDATION
            //    for(xyvk_subpass* pass : renderGraph.renderPasses)
            //       pass->QueryTimeStampsOutput(renderGraph.frameCounter);
            //#endif
        }
    });
    
    window.WhileMain(XYVK_WINDOWEVENTS::WAIT_EVENTS);
    mythread.join();
    vkdevice.WaitIdle();
    return VK_SUCCESS;
};