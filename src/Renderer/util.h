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

typedef struct{
    size_t size;
    char *byteCode;
}FileInfo;

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

static Bool isNull(void *ptr){return ptr == NULL;}


static inline FileInfo readFile(const char *filePath){
    FILE *file = fopen(filePath, "rb");

    if(isNull(file)){
        ERROR_LOG("File not found");
    }else{
        INFO_LOG("File open, path: %s", filePath);
    }
    

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);

    INFO_LOG("Allocating bytes");
    char *byteCode = malloc(sizeof(char) * size);
    if(isNull(byteCode)){
        ERROR_LOG("Error to allocate the byte code");
    }

    rewind(file);

    INFO_LOG("Reading file");
    size_t readSize = fread(byteCode, sizeof(char), size, file);

    if(readSize != size){
        ERROR_LOG("Failed to read code, read size %u, file size %u", readSize, size);
    }else{
        INFO_LOG("Bytes read %u, file size %u", readSize, size);
    }

    fclose(file);

    FileInfo info = { 0 };
    info.size = size;
    info.byteCode = byteCode;

    return info;
}

#endif