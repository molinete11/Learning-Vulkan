#ifndef RENDERER_H
#define RENDERER_H

#include "../x11.h"

#include <vulkan/vulkan.h>


typedef struct {
    VkInstance instance;
    VkPhysicalDevice physical_device;

}Renderer;


Renderer init_renderer(X11Window *window);

#endif