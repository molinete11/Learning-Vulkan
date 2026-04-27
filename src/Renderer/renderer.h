#ifndef RENDERER_H
#define RENDERER_H


#include "../x11.h"

#include <vulkan/vulkan.h>

#define ENABLE_VALIDATION_LAYERS 1

typedef struct{
    VkCommandBuffer cmd;
    VkFence fence;
    VkSemaphore semaphore;
}Frame;

typedef struct {
    VkInstance instance;
    VkPhysicalDevice GPUselected;

    VkDevice logicalDevice;
    VkQueue graphicsPresentationQueue;

    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;

    uint32_t imageCount;
    VkImage *images;
    VkImageView *imagesViews;

    VkPipeline graphicsPipeline;
    VkPipelineLayout graphicsPipelineLayout;

    VkCommandPool commandPool;

    Frame frames;

    VkDebugUtilsMessengerEXT debugMessenger;
}Renderer;



Renderer init_renderer(X11Window *window);
Frame *beginRenderning(Renderer *renderer);
void endRendering(Renderer *renderer, Frame *frame);

void renderTriangle(Frame *frame);

#endif