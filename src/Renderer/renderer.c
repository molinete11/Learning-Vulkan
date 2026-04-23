

#include "renderer.h"

#include <stdlib.h>
#include <string.h>

const char* validation_layers[] = {
    "VK_LAYER_KHRONOS_validation"
};

const char *requiredGPUextensions[] = {
    "VK_KHR_dynamic_rendering",
    "VK_KHR_swapchain"
};

void init_vulkan(Renderer *renderer, X11Window *window);
void create_instance(Renderer *renderer);
void getPhysicalDevice(Renderer *renderer);
void createLogicalDevice(Renderer *renderer);
void createSurface(Renderer *renderer, X11Window *window);
void createSwapchainKHR(Renderer *renderer);

void setupDebugMessenger(Renderer *renderer);

static uint32_t getQueueFamilyIndex(VkPhysicalDevice GPU, VkSurfaceKHR surface);
static Bool isGPUsuitable(VkPhysicalDevice GPU, VkSurfaceKHR surface);
static const char** get_required_extensions();
static Bool checkValidationLayerSupport();
static void populateDebugUtilsMessengerCreateInfoEXT(VkDebugUtilsMessengerCreateInfoEXT *info);
static VkSurfaceFormatKHR getSurfaceFormats(VkPhysicalDevice GPU, VkSurfaceKHR surface);

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

void init_vulkan(Renderer *renderer, X11Window *window)
{   
    create_instance(renderer);
    setupDebugMessenger(renderer);
    createSurface(renderer, window);
    getPhysicalDevice(renderer);
    createLogicalDevice(renderer);
    createSwapchainKHR(renderer);
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
    app_info.apiVersion = VK_API_VERSION_1_3;

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
    

    VkDeviceCreateInfo logicalDeviceInfo = { 0 };

    logicalDeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    logicalDeviceInfo.pNext = NULL;
    logicalDeviceInfo.flags = 0;
    logicalDeviceInfo.queueCreateInfoCount = 1;
    logicalDeviceInfo.pQueueCreateInfos = &queueInfo;
    logicalDeviceInfo.enabledExtensionCount = 1;
    logicalDeviceInfo.ppEnabledExtensionNames = requiredGPUextensions;
    logicalDeviceInfo.pEnabledFeatures = NULL;


    vkCall(vkCreateDevice(renderer->GPUselected, &logicalDeviceInfo, NULL, &renderer->logicalDevice), 
            "logical device created", 
            "failed to create logical device");


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

    VkSwapchainCreateInfoKHR swapchainInfo = { 0 };

    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.pNext = NULL;
    swapchainInfo.flags = 0;
    swapchainInfo.surface = renderer->surface;
    swapchainInfo.minImageCount = surfaceCapabilities.minImageCount;
    //swapchainInfo.imageFormat = getSurfaceFormats(renderer->GPUselected, renderer->surface).format;

    getSurfaceFormats(renderer->GPUselected, renderer->surface);

    //vkCall(vkCreateSwapchainKHR(renderer->logicalDevice, NULL, NULL, &renderer->swapchain),
    //        "Swapchain created",
    //        "failed to create swapchain");
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
