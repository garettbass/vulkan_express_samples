/*cxe{
    -pre { $CXE glfw3.c }
    -std=c11
    -Wall -Werror
    -ferror-limit=2
    -fsanitize=address
    -I$VULKAN_SDK/Include
    -I../inc
    -if (--target=[darwin]) {
        -pre { mkdir -p ../bin/macos }
        -o../bin/macos/$CXE_SRC_NAME
        -MMD -MF ../bin/macos/$CXE_SRC_NAME.dep
        -framework Cocoa
        -framework IOKit
        -framework OpenGL
        -L../lib/macos
    }
    -if (--target=[linux]) {
        -pre { mkdir -p ../bin/linux }
        -o../bin/linux/$CXE_SRC_NAME
        -MMD -MF ../bin/linux/$CXE_SRC_NAME.dep
        -L../lib/linux
    }
    -if (--target=[windows]) {
        -pre { mkdir -p ../bin/windows }
        -o../bin/windows/$CXE_SRC_NAME.exe
        -MMD -MF ../bin/windows/$CXE_SRC_NAME.dep
        -lgdi32
        -lshell32
        -luser32
        -L../lib/windows
    }
    -if (--target=[msvc]) {
        -D_CRT_SECURE_NO_WARNINGS
    }
    -lglfw3
    -post { $CXE $CXE_SRC_NAME++.cpp }
}*/

#ifndef SAMPLE_SOURCE
#define SAMPLE_SOURCE "01-clear.c"
#endif

#include <math.h>

#define VX_IMPLEMENTATION
#define VX_USE_VOLK
#define VX_USE_GLFW3
#include <vulkan_express/vulkan_express.h>

void glfwKeyCallback(GLFWwindow* w, int key, int code, int action, int mod) {
    if (key == GLFW_KEY_ESCAPE && action) {
        glfwSetWindowShouldClose(w, GLFW_TRUE);
        return;
    }
}

int main(const int argc, const char* argv[]) {
    glfwInit();
    if (!glfwVulkanSupported()) {
        puts("GLFW3: Vulkan not supported");
        return 1;
    }

    VkSurfaceFormatKHR surfaceFormat;
    VxContext context;
    {
        vxAssertSuccess(vxCreateContext(NULL, &context));

        vxInfof(
            "context.surfaceCapabilities.minImageCount: %u\n",
            context.surfaceCapabilities.minImageCount
        );
        vxInfof(
            "context.surfaceCapabilities.maxImageCount: %u\n",
            context.surfaceCapabilities.maxImageCount
        );
        vxInfof(
            "context.surfaceCapabilities.maxImageArrayLayers: %u\n",
            context.surfaceCapabilities.maxImageArrayLayers
        );

        vxInfof("context.surfaceFormatCount: %u\n", context.surfaceFormatCount);
        for (uint32_t i = 0; i < context.surfaceFormatCount; ++i) {
            vxInfof(
                "context.surfaceFormats[%u]: { %s, %s }\n", i,
                vxFormatName(context.surfaceFormats[i].format),
                vxColorSpaceName(context.surfaceFormats[i].colorSpace)
            );
        }

        surfaceFormat = context.surfaceFormats[0];
        for (uint32_t i = 0; i < context.surfaceFormatCount; ++i) {
            const VkFormat format = context.surfaceFormats[i].format;
            if (format == VK_FORMAT_B8G8R8A8_SRGB ||
                format == VK_FORMAT_R8G8B8A8_SRGB) {
                surfaceFormat = context.surfaceFormats[i]; // preferred format
                break;
            }
        }
        vxInfof(
            "using surfaceFormat { %s, %s }\n",
            vxFormatName(surfaceFormat.format),
            vxColorSpaceName(surfaceFormat.colorSpace)
        );
    }

    const uint32_t swapchainImageCount = 2;

    VkRenderPass renderPass;
    {
        vxAssertSuccess(
            vkCreateRenderPass(
                context.device,
                VxInlinePtr(VkRenderPassCreateInfo){
                    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                    .attachmentCount = 1,
                    .pAttachments = VxInlineArray(VkAttachmentDescription){
                        { // 0 - swapchain image
                            .format         = surfaceFormat.format,
                            .samples        = VK_SAMPLE_COUNT_1_BIT,
                            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
                            .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                        },
                    },
                    .subpassCount = 1,
                    .pSubpasses = VxInlinePtr(VkSubpassDescription){
                        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                        .colorAttachmentCount = 1,
                        .pColorAttachments = VxInlineArray(VkAttachmentReference){
                            { // 0 - swapchain image
                                .attachment = 0,
                                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            },
                        },
                    },
                    .dependencyCount = 1,
                    .pDependencies = VxInlinePtr(VkSubpassDependency){
                        .srcSubpass    = VK_SUBPASS_EXTERNAL,
                        .dstSubpass    = 0,
                        .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        .srcAccessMask = 0,
                        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    },
                },
                context.pAllocator,
                &renderPass
            )
        );
    }

    const char title[] = "vulkan_express_samples/src/" SAMPLE_SOURCE;
    GLFWwindow* window;
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(1024, 768, title, NULL, NULL);
        glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_TRUE);
        glfwSetKeyCallback(window, glfwKeyCallback);
    }

    VxCanvas canvas;
    {
        vxAssertSuccess(
            vxCreateCanvas(
                &context,
                VxInlinePtr(VxCanvasCreateInfo){
                    .windowHandleType    = VX_WINDOW_HANDLE_TYPE_GLFW,
                    .windowHandle        = window,
                    .surfaceFormat       = surfaceFormat,
                    .swapchainImageCount = swapchainImageCount,
                    .preferredPresentModeCount = 1,
                    .preferredPresentModes     = {
                        VK_PRESENT_MODE_FIFO_KHR,
                    },
                    .renderPass = renderPass,
                },
                &canvas
            )
        );

        vxInfof("canvas.frameCount: %u", canvas.frameCount);
    }

    VkClearColorValue clearColorValue = {{ 0.4f, 0.6f, 0.9f, 1.f }};
    float colorChannelStep[3] = { 0.002f, 0.003f, 0.0045f };

    while (!(glfwPollEvents(),glfwWindowShouldClose(window))) {

        // cycle clear color
        for (int i = 0; i < 3; ++i) {
            clearColorValue.float32[i] = fmodf(
                clearColorValue.float32[i]
                + colorChannelStep[i],
                1.f
            );
        }

        VxCanvasFrame* pFrame = vxBeginFrame(&context, &canvas);
        vxAssert(pFrame);

        VkCommandBuffer commandBuffer = pFrame->commandBuffer;

        VkImageSubresourceRange subresourceRange = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0, .levelCount = 1,
            .baseArrayLayer = 0, .layerCount = 1,
        };

        vkCmdPipelineBarrier(
            commandBuffer,
            /* srcStageMask             */ VK_PIPELINE_STAGE_TRANSFER_BIT,
            /* dstStageMask             */ VK_PIPELINE_STAGE_TRANSFER_BIT,
            /* dependencyFlags          */ 0,
            /* memoryBarrierCount       */ 0,
            /* pMemoryBarriers          */ NULL,
            /* bufferMemoryBarrierCount */ 0,
            /* pBufferMemoryBarriers    */ NULL,
            /* imageMemoryBarrierCount  */ 1,
            /* pImageMemoryBarriers     */ VxInlinePtr(VkImageMemoryBarrier){
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask       = VK_ACCESS_MEMORY_READ_BIT,
                .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .srcQueueFamilyIndex = context.graphicsQueueFamilyIndex,
                .dstQueueFamilyIndex = context.graphicsQueueFamilyIndex,
                .image               = pFrame->swapchainImage,
                .subresourceRange    = subresourceRange,
            }
        );

        vkCmdClearColorImage(
            commandBuffer,
            pFrame->swapchainImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            &clearColorValue,
            1,
            &subresourceRange
        );

        vkCmdPipelineBarrier(
            commandBuffer,
            /* srcStageMask             */ VK_PIPELINE_STAGE_TRANSFER_BIT,
            /* dstStageMask             */ VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            /* dependencyFlags          */ 0,
            /* memoryBarrierCount       */ 0,
            /* pMemoryBarriers          */ NULL,
            /* bufferMemoryBarrierCount */ 0,
            /* pBufferMemoryBarriers    */ NULL,
            /* imageMemoryBarrierCount  */ 1,
            /* pImageMemoryBarriers     */ VxInlinePtr(VkImageMemoryBarrier){
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask       = VK_ACCESS_MEMORY_READ_BIT,
                .oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcQueueFamilyIndex = context.graphicsQueueFamilyIndex,
                .dstQueueFamilyIndex = context.graphicsQueueFamilyIndex,
                .image               = pFrame->swapchainImage,
                .subresourceRange    = subresourceRange,
            }
        );

        vxAssertSuccess(vxSubmitFrame(&context, &canvas, pFrame));
        vxAssertSuccess(vxPresentFrame(&context, &canvas, pFrame));

    }

    vkDeviceWaitIdle(context.device);
    vkDestroyRenderPass(context.device, renderPass, context.pAllocator);
    vxDestroyCanvas(&context, &canvas);
    vxDestroyContext(&context);
    puts("DONE");
    return 0;
}
