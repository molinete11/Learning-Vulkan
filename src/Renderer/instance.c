#include "instance.h"

#include <stdio.h>


VkInstance create_instance(){
    VkInstance instance = VK_NULL_HANDLE;

    VkInstanceCreateInfo info = { 0 };

    VkApplicationInfo app_info = { 0 };

    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pApplicationName = NULL;
    app_info.applicationVersion = VK_API_VERSION_1_4;
    app_info.pEngineName = NULL;
    app_info.engineVersion = VK_API_VERSION_1_4;
    app_info.apiVersion = VK_API_VERSION_1_4;

    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pNext = NULL;
    info.enabledLayerCount = 0;
    info.ppEnabledLayerNames = NULL;
    info.enabledExtensionCount = 0;
    info.ppEnabledExtensionNames = NULL;
    info.pApplicationInfo = &app_info;
    info.flags = 0;

    if(vkCreateInstance(&info, NULL, &instance) != VK_SUCCESS){
        printf("[FATAL ERROR]:\tfailed to create vk instance\n");
    }else{
        printf("[INFO]:\tvk instance created\n");
    }

    return instance;
}