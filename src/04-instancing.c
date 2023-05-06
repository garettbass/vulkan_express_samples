/*cxe{
    -pre { glslc 04-instancing.vert -mfmt=c -o 04-instancing.vert.h }
    -pre { glslc 04-instancing.frag -mfmt=c -o 04-instancing.frag.h }
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
#define SAMPLE_SOURCE "04-instancing.c"
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

#ifdef _WIN32
    #include <direct.h> // _chdir
    #define chdir _chdir
    const char pathSeparator = '\\';
#else
    #include <unistd.h> // chdir
    const char pathSeparator = '/';
#endif

void setWorkingDirectory(char* const argv0) {
    const size_t pathLen = strlen(argv0);
    char* ritr = argv0 + pathLen;
    for (; ritr --> argv0;) {
        if (*ritr == pathSeparator) {
            *ritr = 0; // trim path at first path separator
            printf("chdir(\"%s\")\n", argv0);
            chdir(argv0);
            *ritr = pathSeparator;
            break;
        }
    }
}

//------------------------------------------------------------------------------

void glfwKeyCallback(GLFWwindow* w, int key, int code, int action, int mod) {
    if (key == GLFW_KEY_ESCAPE && action) {
        glfwSetWindowShouldClose(w, GLFW_TRUE);
        return;
    }
}

//------------------------------------------------------------------------------

const char* vxMemoryPropertyFlagNames(VkMemoryPropertyFlags flags) {
    static char buffer[sizeof(
        "DEVICE_LOCAL_BIT | "
        "HOST_VISIBLE_BIT | "
        "HOST_COHERENT_BIT | "
        "HOST_CACHED_BIT | "
        "LAZILY_ALLOCATED_BIT | "
        "PROTECTED_BIT | "
        "DEVICE_COHERENT_BIT_AMD | "
        "DEVICE_UNCACHED_BIT_AMD | "
        "RDMA_CAPABLE_BIT_NV | "
        "\0"
    )] = "";

    char* itr = buffer;
    strcpy(itr, "0");
    if (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
        itr += strlen(strcpy(itr, "DEVICE_LOCAL_BIT | "));
    }
    if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        itr += strlen(strcpy(itr, "HOST_VISIBLE_BIT | "));
    }
    if (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
        itr += strlen(strcpy(itr, "HOST_COHERENT_BIT | "));
    }
    if (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
        itr += strlen(strcpy(itr, "HOST_CACHED_BIT | "));
    }
    if (flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
        itr += strlen(strcpy(itr, "LAZILY_ALLOCATED_BIT | "));
    }
    if (flags & VK_MEMORY_PROPERTY_PROTECTED_BIT) {
        itr += strlen(strcpy(itr, "PROTECTED_BIT | "));
    }
    if (flags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD) {
        itr += strlen(strcpy(itr, "DEVICE_COHERENT_BIT_AMD | "));
    }
    if (flags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD) {
        itr += strlen(strcpy(itr, "DEVICE_UNCACHED_BIT_AMD | "));
    }
    if (flags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV) {
        itr += strlen(strcpy(itr, "RDMA_CAPABLE_BIT_NV | "));
    }
    vxAssert(itr < buffer + sizeof(buffer));
    if (itr > buffer) {
        itr[-3] = 0; // trim trailing " | "
    }
    return buffer;
}

const char* vxMemoryHeapFlagNames(VkMemoryHeapFlags flags) {
    static char buffer[] = {
        "DEVICE_LOCAL_BIT | "
        "MULTI_INSTANCE_BIT | "
        "\0"
    };
    char* itr = buffer;
    strcpy(itr, "0");
    if (flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
        itr += strlen(strcpy(itr, "DEVICE_LOCAL_BIT | "));
    }
    if (flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT) {
        itr += strlen(strcpy(itr, "MULTI_INSTANCE_BIT | "));
    }
    vxAssert(itr < buffer + sizeof(buffer));
    if (itr > buffer) {
        itr[-3] = 0; // trim trailing " | "
    }
    return buffer;
}

//------------------------------------------------------------------------------

VxImageAllocation loadTexture(
    const VxContext* pContext,
    const VxBufferAllocation* pUploadBufferAllocation,
    const char* path
) {
    int width, height, channels;
    uint8_t* pixels = stbi_load(path, &width, &height, &channels, 4);

    vxAssert(pixels);
    vxAssert(width > 0);
    vxAssert(height > 0);
    vxAssert(channels > 0);
    vxAssert(channels <= 4);

    channels = 4;

    const uint32_t imageSize = width * height * channels;

    vxAssert(imageSize <= pUploadBufferAllocation->size);

    vxAssertSuccess(
        vxCopyToBufferAllocation(
            pContext,
            pUploadBufferAllocation,
            0, imageSize, pixels
        )
    );

    stbi_image_free(pixels);

    VxImageAllocation imageAllocation = vxImageAllocation(
        pContext,
        (VxImageAllocationInfo){
            .image = {
                .sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType   = VK_IMAGE_TYPE_2D,
                .format      = VK_FORMAT_R8G8B8A8_SRGB,
                .extent      = {
                    .width   = (uint32_t)width,
                    .height  = (uint32_t)height,
                    .depth   = 1u,
                },
                .mipLevels   = 1,
                .arrayLayers = 1,
                .samples     = VK_SAMPLE_COUNT_1_BIT,
                .tiling      = VK_IMAGE_TILING_OPTIMAL,
                .usage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT
                                | VK_IMAGE_USAGE_SAMPLED_BIT,
                .queueFamilyIndexCount = 1,
                .pQueueFamilyIndices = &pContext->graphicsQueueFamilyIndex,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            },
            .imageAspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            .memory = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        }
    );

    vxAssertSuccess(
        vkBeginCommandBuffer(
            pContext->commandBuffer,
            VxInlinePtr(VkCommandBufferBeginInfo){
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            }
        )
    );

    VkImageSubresourceRange subresourceRange = {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel   = 0, .levelCount = 1,
        .baseArrayLayer = 0, .layerCount = 1,
    };

    vkCmdPipelineBarrier(
        pContext->commandBuffer,
        /* srcStageMask             */ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        /* dstStageMask             */ VK_PIPELINE_STAGE_TRANSFER_BIT,
        /* dependencyFlags          */ 0,
        /* memoryBarrierCount       */ 0,
        /* pMemoryBarriers          */ NULL,
        /* bufferMemoryBarrierCount */ 0,
        /* pBufferMemoryBarriers    */ NULL,
        /* imageMemoryBarrierCount  */ 1,
        /* pImageMemoryBarriers     */ VxInlinePtr(VkImageMemoryBarrier){
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask       = 0,
            .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = imageAllocation.image,
            .subresourceRange    = subresourceRange,
        }
    );

    vkCmdCopyBufferToImage(
        pContext->commandBuffer,
        pUploadBufferAllocation->buffer,
        imageAllocation.image,
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
            .imageExtent = imageAllocation.extent,
        }
    );

    vkCmdPipelineBarrier(
        pContext->commandBuffer,
        /* srcStageMask             */ VK_PIPELINE_STAGE_TRANSFER_BIT,
        /* dstStageMask             */ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        /* dependencyFlags          */ 0,
        /* memoryBarrierCount       */ 0,
        /* pMemoryBarriers          */ NULL,
        /* bufferMemoryBarrierCount */ 0,
        /* pBufferMemoryBarriers    */ NULL,
        /* imageMemoryBarrierCount  */ 1,
        /* pImageMemoryBarriers     */ VxInlinePtr(VkImageMemoryBarrier){
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask       = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = imageAllocation.image,
            .subresourceRange    = subresourceRange,
        }
    );

    vkEndCommandBuffer(pContext->commandBuffer);

    vxAssertSuccess(
        vkQueueSubmit(
            pContext->graphicsQueue,
            1, VxInlinePtr(VkSubmitInfo){
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .commandBufferCount = 1,
                .pCommandBuffers = &pContext->commandBuffer,
            },
            VK_NULL_HANDLE
        )
    );

    vxAssertSuccess(
        vkQueueWaitIdle(
            pContext->graphicsQueue
        )
    );

    return imageAllocation;
}

//------------------------------------------------------------------------------

VxBufferAllocation loadBuffer(
    const VxContext* pContext,
    const VxBufferAllocation* pUploadBufferAllocation,
    uint32_t dataSize,
    void* pData
) {
    vxAssertSuccess(
        vxCopyToBufferAllocation(
            pContext,
            pUploadBufferAllocation,
            0, dataSize, pData
        )
    );

    VxBufferAllocation bufferAllocation = vxBufferAllocation(
        pContext,
        (VxBufferAllocationInfo){
            .buffer = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = dataSize,
                .usage =
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                .queueFamilyIndexCount = 1,
                .pQueueFamilyIndices = &pContext->graphicsQueueFamilyIndex,
            },
            .memory = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        }
    );

    vxAssertSuccess(
        vkBeginCommandBuffer(
            pContext->commandBuffer,
            VxInlinePtr(VkCommandBufferBeginInfo){
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            }
        )
    );

    vkCmdPipelineBarrier(
        pContext->commandBuffer,
        /* srcStageMask             */ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        /* dstStageMask             */ VK_PIPELINE_STAGE_TRANSFER_BIT,
        /* dependencyFlags          */ 0,
        /* memoryBarrierCount       */ 0,
        /* pMemoryBarriers          */ NULL,
        /* bufferMemoryBarrierCount */ 1,
        /* pBufferMemoryBarriers    */ VxInlinePtr(VkBufferMemoryBarrier){
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask       = 0,
            .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer              = bufferAllocation.buffer,
            .offset              = 0,
            .size                = VK_WHOLE_SIZE,
        },
        /* imageMemoryBarrierCount  */ 0,
        /* pImageMemoryBarriers     */ NULL
    );

    vkCmdCopyBuffer(
        pContext->commandBuffer,
        pUploadBufferAllocation->buffer,
        bufferAllocation.buffer,
        1, VxInlinePtr(VkBufferCopy){
            .srcOffset = 0,
            .dstOffset = 0,
            .size = dataSize,
        }
    );

    vkCmdPipelineBarrier(
        pContext->commandBuffer,
        /* srcStageMask             */ VK_PIPELINE_STAGE_TRANSFER_BIT,
        /* dstStageMask             */ VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
        /* dependencyFlags          */ 0,
        /* memoryBarrierCount       */ 0,
        /* pMemoryBarriers          */ NULL,
        /* bufferMemoryBarrierCount */ 1,
        /* pBufferMemoryBarriers    */ VxInlinePtr(VkBufferMemoryBarrier){
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask       = VK_ACCESS_SHADER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer              = bufferAllocation.buffer,
            .offset              = 0,
            .size                = VK_WHOLE_SIZE,
        },
        /* imageMemoryBarrierCount  */ 0,
        /* pImageMemoryBarriers     */ NULL
    );

    vkEndCommandBuffer(pContext->commandBuffer);

    vxAssertSuccess(
        vkQueueSubmit(
            pContext->graphicsQueue,
            1, VxInlinePtr(VkSubmitInfo){
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .commandBufferCount = 1,
                .pCommandBuffers = &pContext->commandBuffer,
            },
            VK_NULL_HANDLE
        )
    );

    vxAssertSuccess(
        vkQueueWaitIdle(
            pContext->graphicsQueue
        )
    );

    return bufferAllocation;
}

//------------------------------------------------------------------------------

int main(const int argc, char* const argv[]) {
    setWorkingDirectory(argv[0]);

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
            "context.surfaceCapabilities.minImageCount: %u",
            context.surfaceCapabilities.minImageCount
        );
        vxInfof(
            "context.surfaceCapabilities.maxImageCount: %u",
            context.surfaceCapabilities.maxImageCount
        );
        vxInfof(
            "context.surfaceCapabilities.maxImageArrayLayers: %u",
            context.surfaceCapabilities.maxImageArrayLayers
        );

        vxInfof("context.surfaceFormatCount: %u", context.surfaceFormatCount);
        for (uint32_t i = 0; i < context.surfaceFormatCount; ++i) {
            vxInfof(
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
        vxInfof(
            "using surfaceFormat { %s, %s }",
            vxFormatName(surfaceFormat.format),
            vxColorSpaceName(surfaceFormat.colorSpace)
        );
    }

    puts("");
    for (uint32_t i = 0; i < context.physicalDeviceMemoryProperties.memoryTypeCount; ++i) {
        const VkMemoryType t = context.physicalDeviceMemoryProperties.memoryTypes[i];
        const VkMemoryHeap h = context.physicalDeviceMemoryProperties.memoryHeaps[t.heapIndex];
        uint64_t size = h.size;
        const char* sizeUnit = "B";
        if (size > 1024) {
            size /= 1024;
            sizeUnit = "KB";
        }
        if (size > 1024) {
            size /= 1024;
            sizeUnit = "MB";
        }
        if (size > 1024) {
            size /= 1024;
            sizeUnit = "GB";
        }
        vxInfof(
            "memoryTypes[%u]\n"
            "    propertyFlags: %s\n"
            "    heapIndex:     %u\n"
            "    heap.size:     %llu%s\n"
            "    heap.flags:    %s",
            i,
            vxMemoryPropertyFlagNames(t.propertyFlags),
            t.heapIndex,
            size, sizeUnit,
            vxMemoryHeapFlagNames(h.flags)
        );
    }

    VxBufferAllocation upload = vxBufferAllocation(
        &context,
        (VxBufferAllocationInfo){
            .buffer = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = 64 * 1024 * 1024,
                .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            },
            .memory
                = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        }
    );
    vxInfof("upload.size: %llu", upload.size);

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

    // relative to executable path
    const char texturePath[] = "../../src/texcoordsRGB.png";

    VxImageAllocation texture = loadTexture(&context, &upload, texturePath);
    VkSampler LINEAR_REPEAT;
    VkSampler NEAREST_CLAMP;
    {
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
                &LINEAR_REPEAT
            )
        );

        vxAssertSuccess(
            vkCreateSampler(
                context.device,
                VxInlinePtr(VkSamplerCreateInfo){
                    .sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                    .magFilter        = VK_FILTER_NEAREST,
                    .minFilter        = VK_FILTER_NEAREST,
                    .mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                    .addressModeU     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                    .addressModeV     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                    .addressModeW     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
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
                &NEAREST_CLAMP
            )
        );
    }

    const uint32_t vertexCount = 4;
    const uint32_t instanceCountX = 16;
    const uint32_t instanceCountY = 16;
    const uint32_t instanceCount = instanceCountX * instanceCountY;
    VxBufferAllocation instances;
    {
        typedef struct f32x2 {
            float x, y;
        } f32x2;

        typedef struct Instance {
            f32x2 origin;
            f32x2 extent;
        } Instance;

        const size_t instanceDataSize = sizeof(Instance) * instanceCount;
        Instance* instanceData = (Instance*)malloc(instanceDataSize);

        const float stepX = 2.f / (float)instanceCountX; // [-1..+1]
        const float stepY = 2.f / (float)instanceCountY; // [-1..+1]
        const float sizeX = 1.f / (float)instanceCountX;
        const float sizeY = 1.f / (float)instanceCountY;
        const float extentX = sizeX * 0.75f;
        const float extentY = sizeY * 0.75f;
        for (uint32_t y = 0; y < instanceCountY; ++y)
        for (uint32_t x = 0; x < instanceCountX; ++x) {
            const uint32_t i = x + instanceCountX * y;
            float offsetX = -1.f + sizeX + stepX * (float)x;
            float offsetY = -1.f + sizeY + stepY * (float)y;
            instanceData[i] = (Instance){
                .origin = { offsetX,  offsetY },
                .extent = { extentX,  extentY },
            };
        }

        instances = loadBuffer(
            &context, &upload,
            instanceDataSize,
            instanceData
        );

        free(instanceData);
    }

    VxBindGroup bindGroup0 = vxBindGroup(
        &context,
        (VxBindGroupCreateInfo){
            .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
            .bindingCount = 4,
            .bindings = {
                {
                    .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    .imageView = texture.imageView,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                },
                {
                    .type = VK_DESCRIPTOR_TYPE_SAMPLER,
                    .sampler = LINEAR_REPEAT,
                },
                {
                    .type = VK_DESCRIPTOR_TYPE_SAMPLER,
                    .sampler = NEAREST_CLAMP,
                },
                {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .buffer = instances.buffer,
                    .bufferOffset = 0,
                    .bufferRange = VK_WHOLE_SIZE,
                },
            },
        }
    );

    VkPipelineLayout graphicsPipelineLayout;
    VkPipeline graphicsPipeline;
    {
        const uint32_t vert[] =
            #include "04-instancing.vert.h"
            ;

        const uint32_t frag[] =
            #include "04-instancing.frag.h"
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
                    .pSetLayouts = &bindGroup0.descriptorSetLayout,
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
                        .scissorCount = 1,
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

    VxCanvas canvas = vxCanvas(
        &context,
        (VxCanvasCreateInfo){
            .windowHandleType    = VX_WINDOW_HANDLE_TYPE_GLFW,
            .windowHandle        = window,
            .surfaceFormat       = surfaceFormat,
            .swapchainImageCount = swapchainImageCount,
            .preferredPresentModeCount = 1,
            .preferredPresentModes     = {
                VK_PRESENT_MODE_FIFO_KHR,
            },
            .renderPass = renderPass,
        }
    );

    vxInfof("canvas.frameCount: %u", canvas.frameCount);

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

        VxCanvasFrame* pFrame = vxBeginFrame(&context, &canvas);
        vxAssert(pFrame);

        VkCommandBuffer commandBuffer = pFrame->commandBuffer;

        vkCmdBeginRenderPass(
            commandBuffer,
            VxInlinePtr(VkRenderPassBeginInfo){
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass  = canvas.renderPass,
                .framebuffer = pFrame->framebuffer,
                .renderArea  = {
                    .extent  = canvas.extent,
                },
                .clearValueCount = 1,
                .pClearValues    = VxInlinePtr(VkClearValue){
                    .color       = clearColorValue
                },
            },
            VK_SUBPASS_CONTENTS_INLINE
        );
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
                0, 1, &bindGroup0.descriptorSet,
                0, NULL
            );

            vkCmdDraw(commandBuffer, vertexCount, instanceCount, 0, 0);
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
    vxDestroy(&context, &graphicsPipeline);
    vxDestroy(&context, &graphicsPipelineLayout);
    vxDestroy(&context, &renderPass);
    vxDestroy(&context, &NEAREST_CLAMP);
    vxDestroy(&context, &LINEAR_REPEAT);
    vxDestroy(&context, &bindGroup0);
    vxDestroy(&context, &instances);
    vxDestroy(&context, &texture);
    vxDestroy(&context, &upload);
    vxDestroy(&context, &canvas);
    vxDestroyContext(&context);
    puts("DONE");
    return 0;
}
