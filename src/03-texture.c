/*cxe{
    -pre { glslc 03-texture.vert -mfmt=c -o 03-texture.vert.h }
    -pre { glslc 03-texture.frag -mfmt=c -o 03-texture.frag.h }
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
#define SAMPLE_SOURCE "03-texture.c"
#endif

#include <math.h>
#include <time.h>

#define VX_IMPLEMENTATION
#define VX_USE_VOLK
#define VX_USE_GLFW3
#include <vulkan_express/vulkan_express.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

//------------------------------------------------------------------------------

void glfwKeyCallback(GLFWwindow* w, int key, int code, int action, int mod) {
    if (key == GLFW_KEY_ESCAPE && action) {
        glfwSetWindowShouldClose(w, GLFW_TRUE);
        return;
    }
}

//------------------------------------------------------------------------------

VkResult loadImage(
    const VxContext* pContext,
    const char* path,
    VkImage* pImage,
    VkImageView* pImageView,
    VkDeviceMemory* pImageMemory
) {
    int width, height, channels;
    const int desired_channels = 4;
    uint8_t* pixels = stbi_load(path, &width, &height, &channels, desired_channels);
    vxAssert(pixels);
    vxAssert(width > 0);
    vxAssert(height > 0);
    vxAssert(channels > 0);
    vxAssert(channels <= desired_channels);

    channels = desired_channels;

    const uint32_t imageSize = width * height * channels;

    const VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(
        pContext->physicalDevice,
        imageFormat,
        &formatProperties
    );

    // TODO: validate image format
    // https://i.gyazo.com/7f56cdef5023147ac7bacf7264ead096.png

    const VkExtent3D imageExtent = {
        .width  = (uint32_t)width,
        .height = (uint32_t)height,
        .depth  = 1u,
    };

    VkBuffer stagingBuffer;
    vxExpectSuccessOrReturn(
        vkCreateBuffer(
            pContext->device,
            VxInlinePtr(VkBufferCreateInfo){
                .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size        = imageSize,
                .usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            },
            pContext->pAllocator,
            &stagingBuffer
        ),{
            stbi_image_free(pixels);
        }
    );

    VkDeviceMemory stagingBufferMemory;
    vxExpectSuccessOrReturn(
        vxAllocateBufferMemory(
            pContext,
            stagingBuffer,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            nullptr,
            &stagingBufferMemory
        ),{
            vkDestroyBuffer(pContext->device, stagingBuffer, pContext->pAllocator);
            stbi_image_free(pixels);
        }
    );

    vxExpectSuccessOrReturn(
        vkBindBufferMemory(
            pContext->device,
            stagingBuffer,
            stagingBufferMemory,
            0
        ),{
            vkFreeMemory(pContext->device, stagingBufferMemory, pContext->pAllocator);
            vkDestroyBuffer(pContext->device, stagingBuffer, pContext->pAllocator);
            stbi_image_free(pixels);
        }
    );

    void* stagingBufferDst;
    vxExpectSuccessOrReturn(
        vkMapMemory(
            pContext->device,
            stagingBufferMemory,
            0, imageSize,
            0,
            &stagingBufferDst
        ),{
            vkFreeMemory(pContext->device, stagingBufferMemory, pContext->pAllocator);
            vkDestroyBuffer(pContext->device, stagingBuffer, pContext->pAllocator);
            stbi_image_free(pixels);
        }
    );
    memcpy(stagingBufferDst, pixels, imageSize);
    vkUnmapMemory(pContext->device, stagingBufferMemory);
    stbi_image_free(pixels);

    vxExpectSuccessOrReturn(
        vkCreateImage(
            pContext->device,
            VxInlinePtr(VkImageCreateInfo){
                .sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType   = VK_IMAGE_TYPE_2D,
                .format      = imageFormat,
                .extent      = imageExtent,
                .mipLevels   = 1,
                .arrayLayers = 1,
                .samples     = VK_SAMPLE_COUNT_1_BIT,
                .tiling      = VK_IMAGE_TILING_OPTIMAL,
                .usage
                    = VK_IMAGE_USAGE_TRANSFER_DST_BIT
                    | VK_IMAGE_USAGE_SAMPLED_BIT,
                .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 1,
                .pQueueFamilyIndices   = &pContext->graphicsQueueFamilyIndex,
                .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
            },
            pContext->pAllocator,
            pImage
        ),{
            vkFreeMemory(pContext->device, stagingBufferMemory, pContext->pAllocator);
            vkDestroyBuffer(pContext->device, stagingBuffer, pContext->pAllocator);
        }
    );

    vxExpectSuccessOrReturn(
        vxAllocateImageMemory(
            pContext,
            *pImage,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            nullptr,
            pImageMemory
        ),{
            vkDestroyImage(pContext->device, *pImage, pContext->pAllocator);
            vkFreeMemory(pContext->device, stagingBufferMemory, pContext->pAllocator);
            vkDestroyBuffer(pContext->device, stagingBuffer, pContext->pAllocator);
            *pImage = VK_NULL_HANDLE;
        }
    );

    vxExpectSuccessOrReturn(
        vkBindImageMemory(
            pContext->device,
            *pImage,
            *pImageMemory,
            0
        ),{
            vkFreeMemory(pContext->device, *pImageMemory, pContext->pAllocator);
            vkDestroyImage(pContext->device, *pImage, pContext->pAllocator);
            vkFreeMemory(pContext->device, stagingBufferMemory, pContext->pAllocator);
            vkDestroyBuffer(pContext->device, stagingBuffer, pContext->pAllocator);
            *pImageMemory = VK_NULL_HANDLE;
            *pImage = VK_NULL_HANDLE;
        }
    );

    VkImageSubresourceRange subresourceRange = {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel   = 0, .levelCount = 1,
        .baseArrayLayer = 0, .layerCount = 1,
    };

    vxExpectSuccessOrReturn(
        vkCreateImageView(
            pContext->device,
            VxInlinePtr(VkImageViewCreateInfo){
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = *pImage,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = imageFormat,
                .subresourceRange = subresourceRange,
            },
            pContext->pAllocator,
            pImageView
        ),{
            vkFreeMemory(pContext->device, *pImageMemory, pContext->pAllocator);
            vkDestroyImage(pContext->device, *pImage, pContext->pAllocator);
            vkFreeMemory(pContext->device, stagingBufferMemory, pContext->pAllocator);
            vkDestroyBuffer(pContext->device, stagingBuffer, pContext->pAllocator);
            *pImageMemory = VK_NULL_HANDLE;
            *pImage = VK_NULL_HANDLE;
        }
    );

    VkCommandBuffer commandBuffer;
    vxExpectSuccessOrReturn(
        vkAllocateCommandBuffers(
            pContext->device,
            VxInlinePtr(VkCommandBufferAllocateInfo){
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool        = pContext->commandPool,
                .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            },
            &commandBuffer
        ),{
            vkDestroyImageView(pContext->device, *pImageView, pContext->pAllocator);
            vkDestroyImage(pContext->device, *pImage, pContext->pAllocator);
            vkFreeMemory(pContext->device, *pImageMemory, pContext->pAllocator);
            vkDestroyBuffer(pContext->device, stagingBuffer, pContext->pAllocator);
            vkFreeMemory(pContext->device, stagingBufferMemory, pContext->pAllocator);
            *pImageMemory = VK_NULL_HANDLE;
            *pImageView = VK_NULL_HANDLE;
            *pImage = VK_NULL_HANDLE;
        }
    );

    vxExpectSuccessOrReturn(
        vkBeginCommandBuffer(
            commandBuffer,
            VxInlinePtr(VkCommandBufferBeginInfo){
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            }
        ),{
            vkFreeCommandBuffers(pContext->device, pContext->commandPool, 1, &commandBuffer);
            vkDestroyImageView(pContext->device, *pImageView, pContext->pAllocator);
            vkDestroyImage(pContext->device, *pImage, pContext->pAllocator);
            vkFreeMemory(pContext->device, *pImageMemory, pContext->pAllocator);
            vkDestroyBuffer(pContext->device, stagingBuffer, pContext->pAllocator);
            vkFreeMemory(pContext->device, stagingBufferMemory, pContext->pAllocator);
            *pImageMemory = VK_NULL_HANDLE;
            *pImageView = VK_NULL_HANDLE;
            *pImage = VK_NULL_HANDLE;
        }
    );

    vkCmdPipelineBarrier2(
        commandBuffer,
        VxInlinePtr(VkDependencyInfo){
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = VxInlinePtr(VkImageMemoryBarrier2){
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask        = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                .srcAccessMask       = 0,
                .dstStageMask        = VK_PIPELINE_STAGE_TRANSFER_BIT,
                .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = *pImage,
                .subresourceRange    = subresourceRange,
            }
        }
    );

    vkCmdCopyBufferToImage(
        commandBuffer,
        stagingBuffer,
        *pImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, VxInlinePtr(VkBufferImageCopy){
            .bufferOffset       = 0,
            .bufferRowLength    = 0,
            .bufferImageHeight  = 0,
            .imageSubresource   = {
                .aspectMask     = subresourceRange.aspectMask,
                .mipLevel       = subresourceRange.baseMipLevel,
                .baseArrayLayer = subresourceRange.baseArrayLayer,
                .layerCount     = subresourceRange.layerCount,
            },
            .imageOffset = { 0, 0, 0 },
            .imageExtent = imageExtent,
        }
    );

    vkCmdPipelineBarrier2(
        commandBuffer,
        VxInlinePtr(VkDependencyInfo){
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = VxInlinePtr(VkImageMemoryBarrier2){
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask        = VK_PIPELINE_STAGE_TRANSFER_BIT,
                .srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstStageMask        = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                .dstAccessMask       = VK_ACCESS_SHADER_READ_BIT,
                .oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = *pImage,
                .subresourceRange    = subresourceRange,
            }
        }
    );

    vkEndCommandBuffer(commandBuffer);

    vxExpectSuccessOrReturn(
        vkQueueSubmit(
            pContext->graphicsQueue,
            1, VxInlinePtr(VkSubmitInfo){
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .commandBufferCount = 1,
                .pCommandBuffers = &commandBuffer,
            },
            VK_NULL_HANDLE
        ),{
            vkFreeCommandBuffers(pContext->device, pContext->commandPool, 1, &commandBuffer);
            vkDestroyImageView(pContext->device, *pImageView, pContext->pAllocator);
            vkDestroyImage(pContext->device, *pImage, pContext->pAllocator);
            vkFreeMemory(pContext->device, *pImageMemory, pContext->pAllocator);
            vkDestroyBuffer(pContext->device, stagingBuffer, pContext->pAllocator);
            vkFreeMemory(pContext->device, stagingBufferMemory, pContext->pAllocator);
            *pImageMemory = VK_NULL_HANDLE;
            *pImageView = VK_NULL_HANDLE;
            *pImage = VK_NULL_HANDLE;
        }
    );

    vxExpectSuccessOrReturn(
        vkQueueWaitIdle(
            pContext->graphicsQueue
        ),{
            vkFreeCommandBuffers(pContext->device, pContext->commandPool, 1, &commandBuffer);
            vkDestroyImageView(pContext->device, *pImageView, pContext->pAllocator);
            vkDestroyImage(pContext->device, *pImage, pContext->pAllocator);
            vkFreeMemory(pContext->device, *pImageMemory, pContext->pAllocator);
            vkDestroyBuffer(pContext->device, stagingBuffer, pContext->pAllocator);
            vkFreeMemory(pContext->device, stagingBufferMemory, pContext->pAllocator);
            *pImageMemory = VK_NULL_HANDLE;
            *pImageView = VK_NULL_HANDLE;
            *pImage = VK_NULL_HANDLE;
        }
    );

    vkFreeCommandBuffers(pContext->device, pContext->commandPool, 1, &commandBuffer);
    vkDestroyBuffer(pContext->device, stagingBuffer, pContext->pAllocator);
    vkFreeMemory(pContext->device, stagingBufferMemory, pContext->pAllocator);
    return VK_SUCCESS;
}

//------------------------------------------------------------------------------

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

        vxInfo(
            "context.surfaceCapabilities.minImageCount: %u",
            context.surfaceCapabilities.minImageCount
        );
        vxInfo(
            "context.surfaceCapabilities.maxImageCount: %u",
            context.surfaceCapabilities.maxImageCount
        );
        vxInfo(
            "context.surfaceCapabilities.maxImageArrayLayers: %u",
            context.surfaceCapabilities.maxImageArrayLayers
        );

        vxInfo("context.surfaceFormatCount: %u", context.surfaceFormatCount);
        for (uint32_t i = 0; i < context.surfaceFormatCount; ++i) {
            vxInfo(
                "context.surfaceFormats[%u]: { %s, %s }", i,
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
        vxInfo(
            "using surfaceFormat { %s, %s }",
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

    VkImage image;
    VkImageView imageView;
    VkDeviceMemory imageMemory;
    VkSampler imageSampler;
    {
        vxAssertSuccess(
            loadImage(
                &context,
                "../../src/texcoordsRGB.png",
                &image,
                &imageView,
                &imageMemory
            )
        );

        vxAssertSuccess(
            vkCreateSampler(
                context.device,
                VxInlinePtr(VkSamplerCreateInfo){
                    .sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                    .magFilter        = VK_FILTER_LINEAR,
                    .minFilter        = VK_FILTER_LINEAR,
                    .mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                    .addressModeU     = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                    .addressModeV     = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                    .addressModeW     = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                    .anisotropyEnable = VK_TRUE,
                    .maxAnisotropy    = context
                        .physicalDeviceProperties
                        .limits
                        .maxSamplerAnisotropy,
                    .compareEnable           = VK_FALSE,
                    .compareOp               = VK_COMPARE_OP_ALWAYS,
                    .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                    .unnormalizedCoordinates = VK_FALSE,
                },
                context.pAllocator,
                &imageSampler
            )
        );
    }

    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSets[VX_MAX_CANVAS_FRAME_COUNT];
    {
        vxAssertSuccess(
            vkCreateDescriptorPool(
                context.device,
                VxInlinePtr(VkDescriptorPoolCreateInfo){
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                    .maxSets = swapchainImageCount,
                    .poolSizeCount = 1,
                    .pPoolSizes = VxInlineArray(VkDescriptorPoolSize){
                        {
                            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            .descriptorCount = swapchainImageCount,
                        },
                    },
                },
                context.pAllocator,
                &descriptorPool
            )
        );

        vxAssertSuccess(
            vkCreateDescriptorSetLayout(
                context.device,
                VxInlinePtr(VkDescriptorSetLayoutCreateInfo){
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                    .bindingCount = 1,
                    .pBindings = VxInlineArray(VkDescriptorSetLayoutBinding){
                        {
                            .binding = 0,
                            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            .descriptorCount = 1,
                            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                            .pImmutableSamplers = NULL,
                        },
                    }
                },
                context.pAllocator,
                &descriptorSetLayout
            )
        );

        vxAssertSuccess(
            vkAllocateDescriptorSets(
                context.device,
                VxInlinePtr(VkDescriptorSetAllocateInfo){
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                    .descriptorPool = descriptorPool,
                    .descriptorSetCount = swapchainImageCount,
                    .pSetLayouts = VxInlineArray(VkDescriptorSetLayout){
                        descriptorSetLayout,
                        descriptorSetLayout,
                    },
                },
                descriptorSets
            )
        );

        VkWriteDescriptorSet writeDescriptorSets[] = {
            { // 0
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = VxInlineArray(VkDescriptorImageInfo){
                    {
                        .sampler = imageSampler,
                        .imageView = imageView,
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    }
                },
                .pBufferInfo = NULL,
                .pTexelBufferView = NULL,
            },
        };

        VkCopyDescriptorSet copyDescriptorSets[] = {};

        for (uint32_t i = 0; i < swapchainImageCount; ++i) {
            writeDescriptorSets[0].dstSet = descriptorSets[i];
            vkUpdateDescriptorSets(
                context.device,
                vxLengthOf(writeDescriptorSets), writeDescriptorSets,
                vxLengthOf(copyDescriptorSets), copyDescriptorSets
            );
        }
    }

    VkPipelineLayout graphicsPipelineLayout;
    VkPipeline graphicsPipeline;
    {
        const uint32_t vert[] =
            #include "03-texture.vert.h"
            ;

        const uint32_t frag[] =
            #include "03-texture.frag.h"
            ;

        VkShaderModule vertShaderModule;
        vxCreateShaderModule(&context, sizeof(vert), vert, &vertShaderModule);

        VkShaderModule fragShaderModule;
        vxCreateShaderModule(&context, sizeof(frag), frag, &fragShaderModule);

        vxAssertSuccess(
            vkCreatePipelineLayout(
                context.device,
                VxInlinePtr(VkPipelineLayoutCreateInfo){
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                    .setLayoutCount = 1,
                    .pSetLayouts = &descriptorSetLayout,
                },
                context.pAllocator,
                &graphicsPipelineLayout
            )
        );

        vxAssertSuccess(
            vkCreateGraphicsPipelines(
                context.device,
                context.pipelineCache,
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

        vxInfo("canvas.frameCount: %u", canvas.frameCount);
    }

    VkClearColorValue clearColorValue = {{ 0.4f, 0.6f, 0.9f, 1.f }};
    float colorChannelStep[3] = { 0.002f, 0.003f, 0.0045f };

    time_t oldFrameTime; time(&oldFrameTime);
    uint32_t frameCounter = 0;
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

            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphicsPipelineLayout,
                0, 1, &descriptorSets[pFrame->frameIndex],
                0, NULL
            );

            vkCmdDraw(commandBuffer, 4, 1, 0, 0);
        }
        vkCmdEndRenderPass(commandBuffer);

        vxAssertSuccess(vxSubmitFrame(&context, &canvas, pFrame));
        vxAssertSuccess(vxPresentFrame(&context, &canvas, pFrame));

        ++frameCounter;
        time_t newFrameTime; time(&newFrameTime);
        if (newFrameTime > oldFrameTime) {
            uint32_t seconds = (uint32_t)(newFrameTime - oldFrameTime);
            uint32_t fps = frameCounter / seconds;
            char buffer[sizeof(title) + 32];
            snprintf(buffer, sizeof(buffer), "%s - %u fps", title, fps);
            glfwSetWindowTitle(window, buffer);
            frameCounter = 0;
            oldFrameTime = newFrameTime;
        }
    }

    vkDeviceWaitIdle(context.device);
    vkDestroyPipeline(context.device, graphicsPipeline, context.pAllocator);
    vkDestroyPipelineLayout(context.device, graphicsPipelineLayout, context.pAllocator);
    vkDestroyDescriptorSetLayout(context.device, descriptorSetLayout, context.pAllocator);
    vkDestroyDescriptorPool(context.device, descriptorPool, context.pAllocator);
    vkDestroySampler(context.device, imageSampler, context.pAllocator);
    vkDestroyImageView(context.device, imageView, context.pAllocator);
    vkDestroyImage(context.device, image, context.pAllocator);
    vkFreeMemory(context.device, imageMemory, context.pAllocator);
    vkDestroyRenderPass(context.device, renderPass, context.pAllocator);
    vxDestroyCanvas(&context, &canvas);
    vxDestroyContext(&context);
    puts("DONE");
    return 0;
}
