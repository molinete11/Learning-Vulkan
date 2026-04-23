#ifndef RENDERER_UTIL_H
#define RENDERER_UTIL_H

#include <stdio.h>
#include <stdlib.h>

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

static inline void vkCall(VkResult res, const char *success_message, const char *fail_message){
    if(res != VK_SUCCESS){
        if(fail_message != NULL){
            ERROR_LOG("%s, reason: %s", fail_message, string_VkResult(res));
            exit(EXIT_FAILURE);
        }
    }else{
        if(success_message != NULL){
            INFO_LOG("%s", success_message);
        }   
    }
}

#endif