#ifndef RENDERER_UTIL_H
#define RENDERER_UTIL_H

#include <stdio.h>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

static inline void vk_call(VkResult res, const char *success_message, const char *fail_message){
    if(res != VK_SUCCESS){
        printf("[ERROR]:\t%s, reason: %s\n", fail_message, string_VkResult(res));
    }else{
        printf("[INFO]:\t%s\n", success_message);
    }
}

#endif