
#include "util.h"
#include "renderer.h"
#include "../Math/types.h"

#include <stdlib.h>
#include <string.h>

const char* validation_layers[] = {
    "VK_LAYER_KHRONOS_validation"
};

const char *requiredGPUextensions[] = {
    "VK_KHR_dynamic_rendering",
    "VK_KHR_swapchain",
    "VK_EXT_extended_dynamic_state3",
};

void init_vulkan(Renderer *renderer, X11Window *window);
void create_instance(Renderer *renderer);
void getPhysicalDevice(Renderer *renderer);
void createLogicalDevice(Renderer *renderer);
void createSurface(Renderer *renderer, X11Window *window);
void createSwapchainKHR(Renderer *renderer);
void getSwapchainImages(Renderer *renderer);
void createImageViews(Renderer *renderer);
void createGraphicsPipelineLayout(Renderer *renderer);
void createGraphicsPipeline(Renderer *renderer);
void createCommandPool(Renderer *renderer);
void createFrames(Renderer *renderer);

Frame allocFrame(Renderer *renderer);

void setupDebugMessenger(Renderer *renderer);

static uint32_t getQueueFamilyIndex(VkPhysicalDevice GPU, VkSurfaceKHR surface);
static Bool isGPUsuitable(VkPhysicalDevice GPU, VkSurfaceKHR surface);
static const char** get_required_extensions();
static Bool checkValidationLayerSupport();
static void populateDebugUtilsMessengerCreateInfoEXT(VkDebugUtilsMessengerCreateInfoEXT *info);
static VkSurfaceFormatKHR getSurfaceFormats(VkPhysicalDevice GPU, VkSurfaceKHR surface);
static VkPresentModeKHR getPresentationMode(VkPhysicalDevice GPU, VkSurfaceKHR surface);
static VkShaderModule createShaderModule(VkDevice logicalDevice, const char *shader);

static VkCommandBuffer allocCommandBuffer(VkCommandPool commandPool, VkDevice ldevice);

static void transitionImageLayout(
    VkCommandBuffer cmd,
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkAccessFlags2 srcAccesFlags,
    VkAccessFlags2 dstAccesFlags,
    VkPipelineStageFlags2 srcStageFlags,
    VkPipelineStageFlags2 dstStageFlags
);

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

VkResult createDebugUtilsMessenger(
    VkInstance instance, 
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
    const VkAllocationCallbacks* pAllocator, 
    VkDebugUtilsMessengerEXT* pDebugMessenger);

Renderer init_renderer(X11Window *window)
{
    Renderer renderer = { 0 };

    init_vulkan(&renderer, window);

    return renderer;
}

Frame *beginRenderning(Renderer *renderer)
{
    Frame *frame = &renderer->frames;

    vkCall(vkWaitForFences(renderer->logicalDevice, 1, &frame->fence, VK_TRUE, UINT64_MAX),
            NULL,
            "failed to wait for fence");

    VkCommandBufferBeginInfo bufferInfo = { 0 };
    bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bufferInfo.pNext = NULL;
    bufferInfo.flags = 0;
    bufferInfo.pInheritanceInfo = NULL;

    vkBeginCommandBuffer(frame->cmd, &bufferInfo);

    transitionImageLayout(frame->cmd, );

    return frame;
}

void endRendering(Renderer *renderer, Frame *frame)
{
}

void renderTriangle(Frame *frame)
{
}

void init_vulkan(Renderer *renderer, X11Window *window)
{   
    create_instance(renderer);
    setupDebugMessenger(renderer);
    createSurface(renderer, window);
    getPhysicalDevice(renderer);
    createLogicalDevice(renderer);
    createSwapchainKHR(renderer);
    getSwapchainImages(renderer);
    createImageViews(renderer);
    createGraphicsPipelineLayout(renderer);
    createGraphicsPipeline(renderer);
    createCommandPool(renderer);
}

void create_instance(Renderer *renderer)
{
    VkInstanceCreateInfo info = { 0 };

    VkApplicationInfo app_info = { 0 };

    const char **requiredExtensions = get_required_extensions();

    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pApplicationName = NULL;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = NULL;
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_4;

    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pNext = NULL;
    info.enabledLayerCount = 0;
    info.ppEnabledLayerNames = NULL;
    info.enabledExtensionCount = 0;
    info.ppEnabledExtensionNames = NULL;
    info.pApplicationInfo = &app_info;
    info.flags = 0;

    if(requiredExtensions != NULL){
        info.enabledExtensionCount = 3;
        info.ppEnabledExtensionNames = requiredExtensions;
    }

    VkDebugUtilsMessengerCreateInfoEXT messenger = { 0 };
    
    if(ENABLE_VALIDATION_LAYERS && checkValidationLayerSupport()){
        info.enabledLayerCount = 1;
        info.ppEnabledLayerNames = validation_layers;

        populateDebugUtilsMessengerCreateInfoEXT(&messenger);
        info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&messenger;
        INFO_LOG("Validation layers supported");
    }

    vkCall(vkCreateInstance(&info, NULL, &renderer->instance), 
            "vk instance created", 
            "failed to create vk instance");
}

void getPhysicalDevice(Renderer *renderer)
{
    int nPhysicalDevices = 0;

    vkCall(vkEnumeratePhysicalDevices(renderer->instance, &nPhysicalDevices, NULL), 
            "Getting physical devices count",
            "failed to get physical devices count");


    VkPhysicalDevice physicalDevices[nPhysicalDevices];

    vkCall(vkEnumeratePhysicalDevices(renderer->instance, &nPhysicalDevices, physicalDevices),
            "Getting physical devices",
            "failed to get physical devices");

    VkPhysicalDeviceProperties physicalDevicesProperties[nPhysicalDevices];
    for(int i = 0; i < nPhysicalDevices; i++){
        vkGetPhysicalDeviceProperties(physicalDevices[i], &physicalDevicesProperties[i]);
        INFO_LOG("GPU detected: %s", physicalDevicesProperties[i].deviceName);
    }

    int GPUindex = INT32_MIN;

    for(int i = 0; i < nPhysicalDevices; i++){
        if(isGPUsuitable(physicalDevices[i], renderer->surface)){
            GPUindex = i;
        }
    }

    if(GPUindex == INT32_MIN){
        ERROR_LOG("No suitable gpu found");
    }

    renderer->GPUselected = physicalDevices[GPUindex];

    INFO_LOG("GPU selected: %s", physicalDevicesProperties[GPUindex].deviceName);
}

void createLogicalDevice(Renderer *renderer)
{
    VkDeviceQueueCreateInfo queueInfo = { 0 };

    float priorities = 1.0f;

    uint32_t queueFamilyIndex = getQueueFamilyIndex(renderer->GPUselected, renderer->surface);

    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.pNext = NULL;
    queueInfo.flags = 0;
    queueInfo.queueFamilyIndex = queueFamilyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priorities;
    
    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRendering = { 0 };
    dynamicRendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamicRendering.dynamicRendering = VK_TRUE;

    VkDeviceCreateInfo logicalDeviceInfo = { 0 };

    logicalDeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    logicalDeviceInfo.pNext = (VkPhysicalDeviceDynamicRenderingFeaturesKHR*)&dynamicRendering;
    logicalDeviceInfo.flags = 0;
    logicalDeviceInfo.queueCreateInfoCount = 1;
    logicalDeviceInfo.pQueueCreateInfos = &queueInfo;
    logicalDeviceInfo.enabledExtensionCount = 3;
    logicalDeviceInfo.ppEnabledExtensionNames = requiredGPUextensions;
    logicalDeviceInfo.pEnabledFeatures = NULL;


    vkCall(vkCreateDevice(renderer->GPUselected, &logicalDeviceInfo, NULL, &renderer->logicalDevice), 
            "Logical device created", 
            "Failed to create logical device");


    vkGetDeviceQueue(renderer->logicalDevice, queueFamilyIndex, 0, &renderer->graphicsPresentationQueue);
}

void createSurface(Renderer *renderer, X11Window *window)
{
    vkCall(vkw_create_xcb_surface(renderer->instance, NULL, &renderer->surface, window),
            "Surface create",
            "Failed to create surface");

}

void createSwapchainKHR(Renderer *renderer)
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities = { 0 };
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderer->GPUselected, renderer->surface, &surfaceCapabilities);

    VkSurfaceFormatKHR surfaceFormat = getSurfaceFormats(renderer->GPUselected, renderer->surface);

    getPresentationMode(renderer->GPUselected, renderer->surface);

    VkSwapchainCreateInfoKHR swapchainInfo = { 0 };

    INFO_LOG("min image count: %u", surfaceCapabilities.minImageCount);

    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.pNext = NULL;
    swapchainInfo.flags = 0;
    swapchainInfo.surface = renderer->surface;
    swapchainInfo.minImageCount = surfaceCapabilities.minImageCount;
    swapchainInfo.imageFormat = surfaceFormat.format;
    swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainInfo.imageExtent = surfaceCapabilities.currentExtent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = getPresentationMode(renderer->GPUselected, renderer->surface);
    swapchainInfo.clipped = VK_FALSE;
    swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

    vkCall(vkCreateSwapchainKHR(renderer->logicalDevice, &swapchainInfo, NULL, &renderer->swapchain),
            "Swapchain created",
           "failed to create swapchain");
}

void getSwapchainImages(Renderer *renderer)
{
    uint32_t nImages = 0;
    vkCall(vkGetSwapchainImagesKHR(renderer->logicalDevice, renderer->swapchain, &nImages, NULL), NULL, NULL);

    renderer->images = calloc(nImages, sizeof(VkImage));
    renderer->imagesViews = calloc(nImages, sizeof(VkImageView));
    renderer->imageCount = nImages;

    if(renderer->images == NULL || renderer->imagesViews == NULL){
        ERROR_LOG("Failed to allocate images");
    }else{
        INFO_LOG("Images allocated");
    }

    vkCall(vkGetSwapchainImagesKHR(renderer->logicalDevice, renderer->swapchain, &nImages, renderer->images), 
            "Swapchain images obtained",
             "Failed to get swapchain images");

    INFO_LOG("Image count: %u", nImages);
}

void createImageViews(Renderer *renderer)
{
    VkFormat format = getSurfaceFormats(renderer->GPUselected, renderer->surface).format;

    VkComponentMapping componentMapping = { 0 };
    componentMapping.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    componentMapping.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    componentMapping.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    componentMapping.b = VK_COMPONENT_SWIZZLE_IDENTITY;

    VkImageSubresourceRange  imageSubresourceRange = { 0 };

    imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageSubresourceRange.layerCount = 1;
    imageSubresourceRange.levelCount = 1;
    imageSubresourceRange.baseArrayLayer = 0;
    imageSubresourceRange.baseMipLevel = 0;

    for(int i = 0; i < renderer->imageCount; i++){

        VkImageViewCreateInfo imageViewInfo = { 0 };
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.pNext = NULL;
        imageViewInfo.flags = 0;
        imageViewInfo.image = renderer->images[i];
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.format = format;
        imageViewInfo.components = componentMapping;
        imageViewInfo.subresourceRange = imageSubresourceRange;

        vkCall(vkCreateImageView(renderer->logicalDevice, &imageViewInfo, NULL, &renderer->imagesViews[i]),
                NULL, NULL);

        INFO_LOG("ImageView %d Created", i);
    }
}

void createGraphicsPipelineLayout(Renderer *renderer)
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = { 0 };
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;


    vkCall(vkCreatePipelineLayout(renderer->logicalDevice, &pipelineLayoutInfo, NULL, &renderer->graphicsPipelineLayout), 
            "pipeline layout created",
            "failed to create pipeline layout");
}

void createGraphicsPipeline(Renderer *renderer)
{
    VkShaderModule vertexModule = createShaderModule(renderer->logicalDevice, "vert.spv");
    VkShaderModule fragmentModule = createShaderModule(renderer->logicalDevice, "frag.spv");

    VkPipelineShaderStageCreateInfo vertexShaderStage = { 0 };
    vertexShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStage.pNext = NULL;
    vertexShaderStage.flags = 0;
    vertexShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStage.module = vertexModule;
    vertexShaderStage.pName = "main";
    vertexShaderStage.pSpecializationInfo = NULL;

    VkPipelineShaderStageCreateInfo fragmentShaderStage = { 0 };
    fragmentShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStage.pNext = NULL;
    fragmentShaderStage.flags = 0;
    fragmentShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderStage.module = fragmentModule;
    fragmentShaderStage.pName = "main";
    fragmentShaderStage.pSpecializationInfo = NULL;

    VkPipelineShaderStageCreateInfo stages[] = {vertexShaderStage, fragmentShaderStage}; 

    VkVertexInputBindingDescription bindingDescription = { 0 };
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(vec3);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    VkVertexInputAttributeDescription attributeDescription = { 0 };
    attributeDescription.location = 0;
    attributeDescription.binding = 0;
    attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescription.offset = 0;

    VkPipelineVertexInputStateCreateInfo vertexInputState = { 0 };

    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.pNext = NULL;
    vertexInputState.flags = 0;
    vertexInputState.vertexBindingDescriptionCount = 1;
    vertexInputState.pVertexBindingDescriptions = &bindingDescription;
    vertexInputState.vertexAttributeDescriptionCount = 1;
    vertexInputState.pVertexAttributeDescriptions = &attributeDescription;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = { 0 };
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.pNext = NULL;
    inputAssemblyState.flags = 0;
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyState.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewPort = { 0 };
    viewPort.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewPort.scissorCount = 1;
    viewPort.viewportCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization = { 0 };
    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.pNext = NULL;
    rasterization.flags = 0;
    rasterization.rasterizerDiscardEnable = VK_FALSE;
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization.depthBiasClamp = VK_FALSE;
    rasterization.lineWidth = 1;

    VkPipelineMultisampleStateCreateInfo multisampleInfo = { 0 };
    multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.sampleShadingEnable = VK_FALSE;
    multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = { 0 };
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlendingState = { 0 };
    colorBlendingState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendingState.pNext = NULL;
    colorBlendingState.flags = 0;
    colorBlendingState.logicOpEnable = VK_FALSE;
    colorBlendingState.attachmentCount = 1;
    colorBlendingState.pAttachments = &colorBlendAttachment;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicStatesInfo = { 0 };
    dynamicStatesInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStatesInfo.pNext = 0;
    dynamicStatesInfo.flags = 0;
    dynamicStatesInfo.dynamicStateCount = 2;
    dynamicStatesInfo.pDynamicStates = dynamicStates;

    VkSurfaceFormatKHR surfaceFormat = getSurfaceFormats(renderer->GPUselected, renderer->surface);

    VkPipelineRenderingCreateInfo renderingInfo = { 0 };
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &surfaceFormat.format;

    VkGraphicsPipelineCreateInfo graphicsPipelineInfo = { 0 };

    graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineInfo.pNext = &renderingInfo;
    graphicsPipelineInfo.flags = VK_PIPELINE_CREATE_2_DISABLE_OPTIMIZATION_BIT;
    graphicsPipelineInfo.stageCount = 2;
    graphicsPipelineInfo.pStages = stages;
    graphicsPipelineInfo.pVertexInputState = &vertexInputState;
    graphicsPipelineInfo.pInputAssemblyState = &inputAssemblyState;
    graphicsPipelineInfo.pTessellationState = NULL;
    graphicsPipelineInfo.pViewportState = &viewPort;
    graphicsPipelineInfo.pRasterizationState = &rasterization;
    graphicsPipelineInfo.pMultisampleState = &multisampleInfo;
    graphicsPipelineInfo.pColorBlendState = &colorBlendingState;
    graphicsPipelineInfo.pDynamicState = &dynamicStatesInfo;
    graphicsPipelineInfo.layout = renderer->graphicsPipelineLayout;
    graphicsPipelineInfo.renderPass = NULL;

    vkCall(vkCreateGraphicsPipelines(renderer->logicalDevice, NULL, 1, &graphicsPipelineInfo, NULL, &renderer->graphicsPipeline),
        "Graphics pipeline created",
        "failed to create graphics pipeline");
}

void createCommandPool(Renderer *renderer)
{
    uint32_t queueFamilyIndex =  getQueueFamilyIndex(renderer->GPUselected, renderer->surface);

    VkCommandPoolCreateInfo commandPoolInfo = { 0 };
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.pNext = NULL;
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolInfo.queueFamilyIndex = queueFamilyIndex;
    

    vkCall(vkCreateCommandPool(renderer->logicalDevice, &commandPoolInfo, NULL, &renderer->commandPool),
            "command pool created",
            "command pool not created");
}

void createFrames(Renderer *renderer)
{
    renderer->frames = allocFrame(renderer);
}

Frame allocFrame(Renderer *renderer)
{
    Frame f = { 0 };
    f.cmd = allocCommandBuffer(renderer->commandPool, renderer->logicalDevice);
    return f;
}

void setupDebugMessenger(Renderer *renderer)
{
    VkDebugUtilsMessengerEXT debugMessenger = { 0 };

    VkDebugUtilsMessengerCreateInfoEXT createInfo = { 0 };
    populateDebugUtilsMessengerCreateInfoEXT(&createInfo);

    vkCall(createDebugUtilsMessenger(renderer->instance, &createInfo, NULL, &renderer->debugMessenger),
            "Debug messenger callback created",
            "Failed to create debug messenger callback");
    
}

uint32_t getQueueFamilyIndex(VkPhysicalDevice GPU, VkSurfaceKHR surface)
{
    uint32_t nQueueFamilies = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(GPU, &nQueueFamilies, NULL);

    VkQueueFamilyProperties queueFamilyProperties[nQueueFamilies];

    vkGetPhysicalDeviceQueueFamilyProperties(GPU, &nQueueFamilies, queueFamilyProperties);

    for(int i = 0; i < nQueueFamilies; i++){
        VkBool32 presentationSupport = 0;
        vkCall(vkGetPhysicalDeviceSurfaceSupportKHR(GPU, i, surface, &presentationSupport), 
                NULL, 
                "failed to get physical device support");

        if(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentationSupport){
            return i;
        }
    }
}

Bool isGPUsuitable(VkPhysicalDevice GPU, VkSurfaceKHR surface)
{

    Bool hasPresentationQueue = FALSE, hasGraphicsQueue = FALSE, hasExtensionSupport = TRUE;
    uint32_t nQueueFamilies = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(GPU, &nQueueFamilies, NULL);

    VkQueueFamilyProperties queueFamilyProperties[nQueueFamilies];

    vkGetPhysicalDeviceQueueFamilyProperties(GPU, &nQueueFamilies, queueFamilyProperties);

    for(int i = 0; i < nQueueFamilies; i++){
        VkBool32 presentationSupport = 0;
        vkCall(vkGetPhysicalDeviceSurfaceSupportKHR(GPU, i, surface, &presentationSupport), NULL, "failed to get physical device support");

        if(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentationSupport){
            hasGraphicsQueue = TRUE;
            hasPresentationQueue = TRUE;
        }
    }

    uint32_t nExtensions = 0;
    vkEnumerateDeviceExtensionProperties(GPU, NULL, &nExtensions, NULL);

    VkExtensionProperties GPUextensionProperties[nExtensions];

    vkEnumerateDeviceExtensionProperties(GPU, NULL, &nExtensions, GPUextensionProperties);

    for(int i = 0; i < 1; i++){
        Bool extensionFound = FALSE;
        for(int j = 0; j < nExtensions; j++){
            if(strcmp(requiredGPUextensions[i], GPUextensionProperties[j].extensionName)){
                extensionFound = TRUE;
            }
        }

        if(!extensionFound){
            hasExtensionSupport = FALSE;
        }
    }


    return hasGraphicsQueue && hasPresentationQueue && hasExtensionSupport;
}

const char **get_required_extensions()
{
    static const char *required_extensions[] = {
        "VK_KHR_surface",
        "VK_KHR_xcb_surface",
        "VK_EXT_debug_utils",
    };

    int num_extensions = 0;
    vkCall(vkEnumerateInstanceExtensionProperties(NULL, &num_extensions, NULL), 
            "Num extensions obtained", 
            "failed to obtain num extensions");

    INFO_LOG("num extensions %i", num_extensions);

    VkExtensionProperties extensionProperties[num_extensions];

    vkCall(vkEnumerateInstanceExtensionProperties(NULL, &num_extensions, extensionProperties),
            "Name extensions obtained",
            "failed to obtain name extensions");

    Bool allExtensionsSupported = TRUE;

    for(int i = 0; i < 3; i++){
        Bool extensionsSupported = FALSE;
        for(int j = 0; j < num_extensions; j++){
            if(strcmp(required_extensions[i], extensionProperties[j].extensionName) == 0){
                INFO_LOG("Extension %s found", required_extensions[i]);
                extensionsSupported = TRUE;
                break;
            }
        }

        if(!extensionsSupported){
            ERROR_LOG("Extension %s not supported", required_extensions[i]);
            allExtensionsSupported = FALSE;
        }
    }

    if(!allExtensionsSupported){
        ERROR_LOG("There are some missing extension support");
        return NULL;
    }else{
        INFO_LOG("All extensions supported");
        return required_extensions;
    }
}

Bool checkValidationLayerSupport()
{
    int nLayers = 0;
    vkCall(vkEnumerateInstanceLayerProperties(&nLayers, NULL),
            "layers obtained",
            "failed to obtain layers");

    INFO_LOG("num layers %i", nLayers);

    VkLayerProperties layerProperties[nLayers];

    vkCall(vkEnumerateInstanceLayerProperties(&nLayers, layerProperties),
            "layer's name obtained",
            "failed to obtain layer's name");
    
    Bool validationLayerSupport = FALSE;
    for(int i = 0; i < nLayers; i++){
        if(strcmp(validation_layers[0], layerProperties[i].layerName)){
            validationLayerSupport = TRUE;
            break;
        }
    }

    if(!validationLayerSupport){
        INFO_LOG("Validation layers not supported");
    }

    return validationLayerSupport;
}

void populateDebugUtilsMessengerCreateInfoEXT(VkDebugUtilsMessengerCreateInfoEXT *info)
{
    info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info->pNext = NULL;
    info->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info->pfnUserCallback = debugCallback;
    info->pUserData = NULL; // Optional
}

VkSurfaceFormatKHR getSurfaceFormats(VkPhysicalDevice GPU, VkSurfaceKHR surface)
{

    uint32_t nSurfaceFormats = 0;
    vkCall(vkGetPhysicalDeviceSurfaceFormatsKHR(GPU, surface, &nSurfaceFormats, NULL), NULL, NULL);

    VkSurfaceFormatKHR surfaceFormats[nSurfaceFormats];
    vkCall(vkGetPhysicalDeviceSurfaceFormatsKHR(GPU, surface, &nSurfaceFormats, surfaceFormats), NULL, NULL);

    for(int i = 0; i < nSurfaceFormats; i++){
        if(surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB){
            return surfaceFormats[i];
        }
    }       

    return surfaceFormats[0];
}

VkPresentModeKHR getPresentationMode(VkPhysicalDevice GPU, VkSurfaceKHR surface)
{
    uint32_t nPresentationModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(GPU, surface, &nPresentationModes, NULL);

    VkPresentModeKHR presentationModes[nPresentationModes];

    vkGetPhysicalDeviceSurfacePresentModesKHR(GPU, surface, &nPresentationModes, presentationModes);

    for(int i = 0; i < nPresentationModes; i++){
        if(presentationModes[i] == VK_PRESENT_MODE_FIFO_KHR){
            return presentationModes[i];
        }
    }
}

VkShaderModule createShaderModule(VkDevice logicalDevice, const char *shader)
{
    VkShaderModule shaderModule;

    FileInfo fileInfo = readFile(shader);

    VkShaderModuleCreateInfo shaderInfo = { 0 };

    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.pNext = NULL;
    shaderInfo.codeSize = fileInfo.size;
    shaderInfo.pCode = (uint32_t*)fileInfo.byteCode;
    shaderInfo.flags = 0;

    vkCall(vkCreateShaderModule(logicalDevice, &shaderInfo, NULL, &shaderModule), "module created", "failed to create module");

    free(fileInfo.byteCode);

    return shaderModule;
}

VkCommandBuffer allocCommandBuffer(VkCommandPool commandPool, VkDevice ldevice)
{
    VkCommandBuffer cmd;
    VkCommandBufferAllocateInfo allocInfo = { 0 };
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.pNext = NULL;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    vkCall(vkAllocateCommandBuffers(ldevice, &allocInfo, &cmd),
            NULL,
            "failed to create command buffer");

    return cmd;
}

void transitionImageLayout(
    VkCommandBuffer cmd,
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkAccessFlags2 srcAccesFlags,
    VkAccessFlags2 dstAccesFlags,
    VkPipelineStageFlags2 srcStageFlags,
    VkPipelineStageFlags2 dstStageFlags)
{
    VkImageMemoryBarrier2 barrier = { 0 };
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcAccessMask = srcAccesFlags;
    barrier.dstAccessMask = dstAccesFlags;
    barrier.srcStageMask = srcStageFlags;
    barrier.dstStageMask = dstStageFlags;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.baseMipLevel = 0;

    VkDependencyInfo dependencyInfo = { 0 };
    dependencyInfo.dependencyFlags = 0;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(cmd, &dependencyInfo);
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
    VkDebugUtilsMessageTypeFlagsEXT messageType, 
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, 
    void *pUserData)
{
    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        INFO_LOG("%s", pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        WARNING_LOG("%s", pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        ERROR_LOG("%s", pCallbackData->pMessage);
    default:
        break;
    }

    return VK_FALSE;
}

VkResult createDebugUtilsMessenger(
    VkInstance instance, 
    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, 
    const VkAllocationCallbacks *pAllocator, 
    VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if(func != NULL){
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
