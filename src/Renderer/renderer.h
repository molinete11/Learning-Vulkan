#ifndef RENDERER_H
#define RENDERER_H

#include "util.h"
#include "../x11.h"


#include <vulkan/vulkan.h>

#define ENABLE_VALIDATION_LAYERS 1


typedef struct {
    VkInstance instance;
    VkPhysicalDevice GPUselected;

    VkDevice logicalDevice;
    VkQueue graphicsPresentationQueue;

    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;

    VkDebugUtilsMessengerEXT debugMessenger;
}Renderer;


Renderer init_renderer(X11Window *window);


#endif