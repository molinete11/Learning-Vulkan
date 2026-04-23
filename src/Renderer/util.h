#ifndef RENDERER_UTIL_H
#define RENDERER_UTIL_H

#include <stdio.h>
#include <stdarg.h>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>



#define INFO_LOG(m, ...) (printf("[INFO]:\t"  m  "\n", ##__VA_ARGS__))
#define WARNING_LOG(m, ...) (printf("[WARNING]:\t"  m  "\n", ##__VA_ARGS__))
#define ERROR_LOG(m, ...) (printf("[ERROR]:\t"  m  "\n", ##__VA_ARGS__))


typedef enum {
    FALSE,
    TRUE,
}Bool;

typedef enum {
    INFO,
    WARNING,
    ERROR
}SeverityLog;

static inline void vk_call(VkResult res, const char *success_message, const char *fail_message){
    if(res != VK_SUCCESS){
        ERROR_LOG("%s, reason: %s", fail_message, string_VkResult(res));
    }else{
        INFO_LOG("%s", success_message);
    }
}

#endif