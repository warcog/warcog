#ifndef VK_H
#define VK_H

#define _WINDOWS_ //avoid windows.h inclusion
#define HINSTANCE void*
#define HWND void*

#include <vulkan/vulkan.h>
#include <string.h>

#undef _WINDOWS_
#undef HINSTANCE
#undef HWND

#define vk_queue_info(index, priorityp) \
    (VkDeviceQueueCreateInfo){.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, \
    .queueFamilyIndex = index, .queueCount = 1, .pQueuePriorities = priorityp}

#define vk_get_queue(dev, index, queuep) \
    vkGetDeviceQueue(dev, index, 0, queuep)

#define vk_mem_props(gpu, propsp) \
    vkGetPhysicalDeviceMemoryProperties(gpu, propsp)

#define vk_get_images(dev, sc, countp, imagep) \
    vkGetSwapchainImagesKHR(dev, sc, countp, imagep)

#define vk_create_view(dev, viewp, ...) ({ \
    VkImageViewCreateInfo info = { \
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, \
        .components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, \
                 VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A, }, \
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}, \
        .viewType = VK_IMAGE_VIEW_TYPE_2D, \
         __VA_ARGS__ \
    }; \
    vkCreateImageView(dev, &info, 0, viewp); \
})

#define vk_attachment(...) (VkAttachmentDescription){.samples = VK_SAMPLE_COUNT_1_BIT, \
    .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, \
    .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE, \
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, \
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE, \
    __VA_ARGS__ }

#define vk_subpass(nattach, attach, dattach) (VkSubpassDescription) { \
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS, .colorAttachmentCount = nattach, \
    .pColorAttachments = attach, .pDepthStencilAttachment = dattach}

#define vk_create_renderpass(dev, rpassp, ...) ({ \
    VkRenderPassCreateInfo info = { \
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, \
         __VA_ARGS__ \
    }; \
    vkCreateRenderPass(dev, &info, 0, rpassp); \
})

#define vk_create_image(dev, imagep, ...) ({ \
    VkImageCreateInfo info = { \
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, \
         __VA_ARGS__ \
    }; \
    vkCreateImage(dev, &info, 0, imagep); \
})

#define vk_layout(_binding, type, count, stages) (VkDescriptorSetLayoutBinding){ \
    .binding = _binding, .descriptorType = type, .descriptorCount = count, .stageFlags = stages}

#define vk_layout_uniform(binding, count, stages) \
    vk_layout(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count, stages)

#define vk_layout_sampler(binding, count, stages) \
    vk_layout(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count, stages)

#define vk_create_desc_layout(dev, layoutp, ...) ({ \
    VkDescriptorSetLayoutCreateInfo info = { \
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, \
         __VA_ARGS__ \
    }; \
    vkCreateDescriptorSetLayout(dev, &info, 0, layoutp); \
})

#define vk_create_pipe_layout(dev, layoutp, ...) ({ \
    VkPipelineLayoutCreateInfo info = { \
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, \
         __VA_ARGS__ \
    }; \
    vkCreatePipelineLayout(dev, &info, 0, layoutp); \
})

#define vk_assembly(topo) (VkPipelineInputAssemblyStateCreateInfo){ \
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, \
    .topology = topo}

#define vk_depth_stencil(...) (VkPipelineDepthStencilStateCreateInfo){ \
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, __VA_ARGS__}

#define vk_stage(_stage) (VkPipelineShaderStageCreateInfo){ \
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = _stage, .pName = "main"}


#define vk_vertex_input(nbind, bind, nattr, attr) (VkPipelineVertexInputStateCreateInfo){ \
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, \
    .vertexBindingDescriptionCount = nbind, .pVertexBindingDescriptions = bind, \
    .vertexAttributeDescriptionCount = nattr, .pVertexAttributeDescriptions = attr}

#define vk_pipeline(vi, ia, rs, ds, cb, dyns, ms, vp, _layout, stages) \
    (VkGraphicsPipelineCreateInfo){.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, \
    .pVertexInputState = vi, .pInputAssemblyState = ia, .pRasterizationState = rs, \
    .pDepthStencilState = ds, .pColorBlendState = cb, .pDynamicState = dyns, \
    .pMultisampleState = ms, .pViewportState = vp, .pStages = stages, \
    .layout = _layout}

#define vk_create_pipeline(dev, pipelinep, infop) \
    vkCreateGraphicsPipelines(dev, 0, 1, infop, 0, pipelinep)

#define vk_create_shader(dev, shaderp, ...) ({ \
    VkShaderModuleCreateInfo info = { \
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, \
         __VA_ARGS__ \
    }; \
    vkCreateShaderModule(dev, &info, 0, shaderp); \
})

#define vk_create_pool(dev, poolp, ...) ({ \
    VkDescriptorPoolCreateInfo info = { \
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, \
         __VA_ARGS__ \
    }; \
    vkCreateDescriptorPool(dev, &info, 0, poolp); \
})

#define vk_alloc_sets(dev, setsp, ...) ({ \
    VkDescriptorSetAllocateInfo info = { \
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, \
         __VA_ARGS__ \
    }; \
    vkAllocateDescriptorSets(dev, &info, setsp); \
})

#define vk_sampler(_sampler, view) (VkDescriptorImageInfo){.sampler = _sampler, \
    .imageView = view, .imageLayout = VK_IMAGE_LAYOUT_GENERAL}

#define vk_uniform(buf, size) (VkDescriptorBufferInfo){.buffer = buf, .range = size}

#define vk_write_desc(...) (VkWriteDescriptorSet){ \
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, __VA_ARGS__}

#define vk_write_uniform(set, bind, count, p) vk_write_desc(.dstSet = set, .descriptorCount = count, \
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .pBufferInfo = p, .dstBinding = bind)

#define vk_write_sampler(set, bind, offset, count, p) vk_write_desc(.dstSet = set, \
    .descriptorCount = count, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, \
    .pImageInfo = p, .dstBinding = bind, .dstArrayElement = offset)

#define vk_write(dev, count, writes) \
    vkUpdateDescriptorSets(dev, count, writes, 0, 0)

#define vk_create_fb(dev, fbp, ...) ({ \
    VkFramebufferCreateInfo info = { \
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, \
        .layers = 1, \
         __VA_ARGS__ \
    }; \
    vkCreateFramebuffer(dev, &info, 0, fbp); \
})

#define vk_image_barrier(cmd, ...) ({ \
    VkImageMemoryBarrier info = { \
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, \
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, \
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, \
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}, \
         __VA_ARGS__ \
    }; \
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, \
        0, 0, 0, 0, 0, 1, &info); \
})

#define vk_buffer_barrier(cmd, ...) ({ \
    VkBufferMemoryBarrier  info = { \
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, \
         __VA_ARGS__ \
    }; \
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, \
        0, 0, 0, 1, &info, 0, 0); \
})

#define vk_create_semaphore(cmd, semap, ...) ({ \
    VkSemaphoreCreateInfo info = { \
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, \
         __VA_ARGS__ \
    }; \
    vkCreateSemaphore(cmd, &info, 0, semap); \
})

/*#define FE_1(WHAT, X) WHAT(X)
#define FE_2(WHAT, X, ...) WHAT(X)FE_1(WHAT, __VA_ARGS__)
#define FE_3(WHAT, X, ...) WHAT(X)FE_2(WHAT, __VA_ARGS__)
#define FE_4(WHAT, X, ...) WHAT(X)FE_3(WHAT, __VA_ARGS__)
#define FE_5(WHAT, X, ...) WHAT(X)FE_4(WHAT, __VA_ARGS__)
//... repeat as needed

#define GET_MACRO(_1,_2,_3,_4,_5,NAME,...) NAME
#define for_each(action,...) \
  GET_MACRO(__VA_ARGS__,FE_5,FE_4,FE_3,FE_2,FE_1)(action,__VA_ARGS__)

#define __named_arg(x) .x,
#define __named_arg_expand(...) for_each(__named_arg, __VA_ARGS__)*/

#define vk_create_instance(instp, ...) ({ \
    VkInstanceCreateInfo info = { \
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, \
         __VA_ARGS__ \
    }; \
    vkCreateInstance(&info, 0, instp); \
})

#ifdef WIN32
#define VK_KHR_WIN_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#else
#define VK_KHR_WIN_SURFACE_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME
#endif

#define vk_create_surface_win32(inst, surfacep, ...) ({ \
    VkWin32SurfaceCreateInfoKHR info = { \
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, \
         __VA_ARGS__ \
    }; \
    vkCreateWin32SurfaceKHR(inst, &info, 0, surfacep); \
})

#define vk_create_surface_xcb(inst, surfacep, ...) ({ \
    VkXcbSurfaceCreateInfoKHR info = { \
        .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR, \
         __VA_ARGS__ \
    }; \
    vkCreateXcbSurfaceKHR(inst, &info, 0, surfacep); \
})

/*#ifdef VK_KHR_xlib_surface
#define vk_create_surface(inst, surfacep, ...) ({ \
    VkXlibSurfaceCreateInfoKHR info = { \
        .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR, \
         __VA_ARGS__ \
    }; \
    vkCreateXlibSurfaceKHR(inst, &info, 0, surfacep); \
})
//#define VK_KHR_WIN_SURFACE_EXTENSION_NAME VK_KHR_XLIB_SURFACE_EXTENSION_NAME
//#endif*/

#define vk_destroy(objx, obj) _Generic((obj), \
    VkShaderModule : vkDestroyShaderModule, \
    VkSemaphore : vkDestroySemaphore, \
    VkSurfaceKHR : vkDestroySurfaceKHR) \
    (objx, obj, 0)

#define vk_create_device(phys, devicep, ...) ({ \
    VkDeviceCreateInfo info = { \
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, \
         __VA_ARGS__ \
    }; \
    vkCreateDevice(phys, &info, 0, devicep); \
})

#define vk_enum_gpu(inst, countp, gpup) \
    vkEnumeratePhysicalDevices(inst, countp, gpup)

#define vk_get_caps(gpu, surface, capsp) \
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, capsp)

#define vk_get_presentmodes(gpu, surface, countp, modep) \
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, countp, modep)


#define vk_create_cmd(device, cmdp, ...) ({ \
    VkCommandBufferAllocateInfo info = { \
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, \
         __VA_ARGS__ \
    }; \
    vkAllocateCommandBuffers(device, &info, cmdp); \
})

#define vk_create_cmdpool(device, cmdpoolp, ...) ({ \
    VkCommandPoolCreateInfo info = { \
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, \
         __VA_ARGS__ \
    }; \
    vkCreateCommandPool(device, &info, 0, cmdpoolp); \
})

#define vk_create_swapchain(device, swapchainp, ...) ({ \
    VkSwapchainCreateInfoKHR info = { \
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, \
         __VA_ARGS__ \
    }; \
    vkCreateSwapchainKHR(device, &info, 0, swapchainp); \
})

#define vk_create_sampler(device, samplerp, ...) ({ \
    VkSamplerCreateInfo info = { \
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, \
         __VA_ARGS__ \
    }; \
    vkCreateSampler(device, &info, 0, samplerp); \
})

#define vk_create_buffer(device, bufferp, ...) ({ \
    VkBufferCreateInfo info = { \
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, \
         __VA_ARGS__ \
    }; \
    vkCreateBuffer(device, &info, 0, bufferp); \
})

#define vk_destroy_buffer(dev, buf) \
    vkDestroyBuffer(dev, buf, 0)


#define vk_destroy_swapchain(device, swapchain) \
    vkDestroySwapchainKHR(device, swapchain, 0)

/* command buffers */

#define vk_begin_cmd(cmd, ...) ({ \
    VkCommandBufferInheritanceInfo info = { \
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO, \
        __VA_ARGS__ \
    }; \
    VkCommandBufferBeginInfo cinfo = { \
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, \
        .pInheritanceInfo = &info, \
    }; \
    vkBeginCommandBuffer(cmd, &cinfo); \
})

#define vk_begin_render(cmd, ...) ({ \
    VkRenderPassBeginInfo info = { \
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, \
         __VA_ARGS__ \
    }; \
    vkCmdBeginRenderPass(cmd, &info, VK_SUBPASS_CONTENTS_INLINE); \
})

#define vk_bind_pipeline(cmd, pipeline) \
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline)

#define vk_bind_descriptors(cmd, layout, count, descp) \
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, count, descp, 0, 0)


#define vk_set_viewport(cmd, ...) ({ \
    VkViewport info = { \
        .maxDepth = 1.0, \
         __VA_ARGS__ \
    }; \
    vkCmdSetViewport(cmd, 0, 1, &info); \
})

#define vk_set_scissor(cmd, ...) ({ \
    VkRect2D info = { \
         __VA_ARGS__ \
    }; \
    vkCmdSetScissor(cmd, 0, 1, &info); \
})

#define vk_bind_buf(cmd, bufp, offset) ({ \
    VkDeviceSize offsets[1] = {offset}; \
    vkCmdBindVertexBuffers(cmd, 0, 1, bufp, offsets); \
})

#define vk_bind_index_buf(cmd, bufp, offset) \
    vkCmdBindIndexBuffer(cmd, bufp, offset, VK_INDEX_TYPE_UINT16)


#define vk_draw(cmd, nvert, offset) \
    vkCmdDraw(cmd, nvert, 1, offset, 0)

#define vk_end_render(cmd) \
    vkCmdEndRenderPass(cmd)

#define vk_end_cmd(cmd) \
    vkEndCommandBuffer(cmd)

/* memory */

#define vk_mem_reqs(dev, obj, memreqsp) _Generic((obj), \
    VkBuffer : vkGetBufferMemoryRequirements, \
    VkImage : vkGetImageMemoryRequirements) \
    (dev, obj, memreqsp)

#define vk_alloc(dev, memp, ...) ({ \
    VkMemoryAllocateInfo info = { \
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, \
         __VA_ARGS__ \
    }; \
    vkAllocateMemory(dev, &info, 0, memp); \
})

#define vk_bind(dev, obj, mem) _Generic((obj), \
    VkBuffer : vkBindBufferMemory, \
    VkImage : vkBindImageMemory) \
    (dev, obj, mem, 0)

#define vk_free(dev, mem) \
    vkFreeMemory(dev, mem, 0)

#define vk_map(dev, mem, datapp) \
    vkMapMemory(dev, mem, 0, VK_WHOLE_SIZE, 0, datapp)

#define vk_unmap(dev, mem) \
    vkUnmapMemory(dev, mem)

#define vk_memcpy(dev, mem, offset, data, size) ({ \
    void *pdata; \
    VkResult err; \
    err = vkMapMemory(dev, mem, offset, size, 0, &pdata); \
    if (!err) { \
        memcpy(pdata, data, size); \
        vk_unmap(dev, mem); \
    } \
    err; \
})

typedef struct {
    VkImage image;
    VkDeviceMemory mem;
    VkImageView view;
} texture_t;

typedef struct {
    VkBuffer buf;
    VkDeviceMemory mem;
} buffer_t;

#endif
