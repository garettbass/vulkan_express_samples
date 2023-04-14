/*cxe{
    -pre { glslc 03-quads.vert -mfmt=c -o 03-quads.vert.h }
    -pre { glslc 03-quads.frag -mfmt=c -o 03-quads.frag.h }
    -pre { $CXE glfw3.c }
    -std=c11
    -Wall -Werror
    -Wno-unused-variable
    -Wno-unused-but-set-variable
    -ferror-limit=2
    -fsanitize=address
    -I$VULKAN_SDK/include
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
#define SAMPLE_SOURCE "03-quads.c"
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

    VxContext context;
    vxAssertSuccess(vxCreateContext(NULL, &context));

    printf(
        "context.surfaceCapabilities.minImageCount: %u\n",
         context.surfaceCapabilities.minImageCount
    );
    printf(
        "context.surfaceCapabilities.maxImageCount: %u\n",
         context.surfaceCapabilities.maxImageCount
    );
    printf(
        "context.surfaceCapabilities.maxImageArrayLayers: %u\n",
         context.surfaceCapabilities.maxImageArrayLayers
    );

    printf("context.surfaceFormatCount: %u\n", context.surfaceFormatCount);
    for (uint32_t i = 0; i < context.surfaceFormatCount; ++i) {
        printf(
            "context.surfaceFormats[%u]: { %s, %s }\n", i,
            vxFormatName(context.surfaceFormats[i].format),
            vxColorSpaceName(context.surfaceFormats[i].colorSpace)
        );
    }

    VkSurfaceFormatKHR surfaceFormat = context.surfaceFormats[0];
    for (uint32_t i = 0; i < context.surfaceFormatCount; ++i) {
        const VkFormat format = context.surfaceFormats[i].format;
        if (format == VK_FORMAT_B8G8R8A8_SRGB ||
            format == VK_FORMAT_R8G8B8A8_SRGB) {
            surfaceFormat = context.surfaceFormats[i]; // preferred format
            break;
        }
    }
    printf(
        "using surfaceFormat { %s, %s }\n",
        vxFormatName(surfaceFormat.format),
        vxColorSpaceName(surfaceFormat.colorSpace)
    );

    VkRenderPass renderPass;
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


    // graphics pipeline

    const uint32_t vert[] =
        #include "03-quads.vert.h"
        ;

    const uint32_t frag[] =
        #include "03-quads.frag.h"
        ;

    VkShaderModule vertShaderModule;
    vxCreateShaderModule(&context, sizeof(vert), vert, &vertShaderModule);

    VkShaderModule fragShaderModule;
    vxCreateShaderModule(&context, sizeof(frag), frag, &fragShaderModule);

    VkPipelineCache pipelineCache = VK_NULL_HANDLE;

    VkPipelineLayout graphicsPipelineLayout;
    vxAssertSuccess(
        vkCreatePipelineLayout(
            context.device,
            VxInlinePtr(VkPipelineLayoutCreateInfo){
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            },
            context.pAllocator,
            &graphicsPipelineLayout
        )
    );

    VkPipeline graphicsPipeline;
    vxAssertSuccess(
        vkCreateGraphicsPipelines(
            context.device,
            pipelineCache,
            1, VxInlinePtr(VkGraphicsPipelineCreateInfo){
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .stageCount = 2,
                .pStages = VxInlineArray(VkPipelineShaderStageCreateInfo){
                    {
                        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                        .stage  = VK_SHADER_STAGE_VERTEX_BIT,
                        .module = vertShaderModule,
                        .pName  = "main",
                    },
                    {
                        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                        .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
                        .module = fragShaderModule,
                        .pName  = "main",
                    },
                },
                .pVertexInputState = VxInlinePtr(VkPipelineVertexInputStateCreateInfo){
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                },
                .pInputAssemblyState = VxInlinePtr(VkPipelineInputAssemblyStateCreateInfo){
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
                },
                .pViewportState = VxInlinePtr(VkPipelineViewportStateCreateInfo){
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                    .viewportCount = 1,
                    .scissorCount  = 1,
                },
                .pRasterizationState = VxInlinePtr(VkPipelineRasterizationStateCreateInfo){
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                    .cullMode = VK_CULL_MODE_BACK_BIT,
                    .frontFace = VK_FRONT_FACE_CLOCKWISE,
                    .lineWidth = 1.0f,
                },
                .pMultisampleState = VxInlinePtr(VkPipelineMultisampleStateCreateInfo){
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                    .sampleShadingEnable = VK_FALSE,
                },
                .pColorBlendState = VxInlinePtr(VkPipelineColorBlendStateCreateInfo){
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                    .logicOpEnable = VK_FALSE,
                    .logicOp = VK_LOGIC_OP_COPY,
                    .attachmentCount = 1,
                    .pAttachments = VxInlineArray(VkPipelineColorBlendAttachmentState){
                        {
                            .blendEnable = VK_FALSE,
                            .colorWriteMask
                                = VK_COLOR_COMPONENT_R_BIT
                                | VK_COLOR_COMPONENT_G_BIT
                                | VK_COLOR_COMPONENT_B_BIT
                                | VK_COLOR_COMPONENT_A_BIT,
                        },
                    },
                },
                .pDynamicState = VxInlinePtr(VkPipelineDynamicStateCreateInfo){
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                    .dynamicStateCount = 2,
                    .pDynamicStates = VxInlineArray(VkDynamicState){
                        VK_DYNAMIC_STATE_VIEWPORT,
                        VK_DYNAMIC_STATE_SCISSOR,
                    },
                },
                .layout = graphicsPipelineLayout,
                .renderPass = renderPass,
                .subpass = 0,
                .basePipelineHandle = VK_NULL_HANDLE,
            },
            context.pAllocator,
            &graphicsPipeline
        )
    );
    vkDestroyShaderModule(context.device, vertShaderModule, context.pAllocator);
    vkDestroyShaderModule(context.device, fragShaderModule, context.pAllocator);

    // window & canvas

    const char title[] = "vulkan_express_samples/src/" SAMPLE_SOURCE;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1024, 768, title, NULL, NULL);
    glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_TRUE);
    glfwSetKeyCallback(window, glfwKeyCallback);

    VxCanvas canvas;
    VxCanvasCreateInfo canvasCreateInfo = {
        .windowHandleType = VX_WINDOW_HANDLE_TYPE_GLFW,
        .windowHandle     = window,
        .surfaceFormat    = surfaceFormat,
        .renderPass       = renderPass,
    };
    vxAssertSuccess(
        vxCreateCanvas(
            &context,
            &canvasCreateInfo,
            &canvas
        )
    );

    printf("canvas.frameCount: %u\n\n", canvas.frameCount);

    // update loop

    VkClearColorValue clearColorValue = {{ 0.4f, 0.6f, 0.9f, 1.f }};
    float colorChannelStep[3] = { 0.0004f, 0.0006f, 0.0009f };

    while (!(glfwPollEvents(),glfwWindowShouldClose(window))) {

        // cycle clear color
        for (int i = 0; i < 3; ++i) {
            clearColorValue.float32[i] = fmodf(
                clearColorValue.float32[i]
                + colorChannelStep[i],
                1.f
            );
        }

        VxCanvasFrame* pFrame;
        vxAssertSuccess(vxBeginFrame(&context, &canvas, ~0u, 0, 0, &pFrame));

        VkCommandBuffer commandBuffer = pFrame->commandBuffer;

        VkRenderPassBeginInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass  = canvas.renderPass,
            .framebuffer = pFrame->framebuffer,
            .renderArea  = {
                .extent  = canvas.extent,
            },
            .clearValueCount = 1,
            .pClearValues    = VxInlineArray(VkClearValue){
                { .color = clearColorValue },
            },
        };

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

            vkCmdSetViewport(commandBuffer, 0, 1, VxInlinePtr(VkViewport){
                .width    = (float)canvas.extent.width,
                .height   = (float)canvas.extent.height,
                .minDepth = 0.f,
                .maxDepth = 1.f,
            });

            vkCmdSetScissor(commandBuffer, 0, 1, VxInlinePtr(VkRect2D){
                .extent = canvas.extent,
            });

            vkCmdDraw(commandBuffer, 4, 1, 0, 0);
        }
        vkCmdEndRenderPass(commandBuffer);

        vxAssertSuccess(vxSubmitFrame(&context, &canvas, pFrame));
        vxAssertSuccess(vxPresentFrame(&context, &canvas, pFrame));

    }

    vkDeviceWaitIdle(context.device);
    vkDestroyPipeline(context.device, graphicsPipeline, context.pAllocator);
    vkDestroyPipelineLayout(context.device, graphicsPipelineLayout, context.pAllocator);
    vkDestroyRenderPass(context.device, renderPass, context.pAllocator);
    vxDestroyCanvas(&context, &canvas);
    vxDestroyContext(&context);
    puts("DONE");
    return 0;
}
