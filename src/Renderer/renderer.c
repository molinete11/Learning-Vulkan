

#include "renderer.h"

#include <stdlib.h>
#include <string.h>

const char* validation_layers[] = {
    "VK_LAYER_KHRONOS_validation"
};

void init_vulkan(Renderer *renderer);
VkInstance create_instance();
VkDebugUtilsMessengerEXT setupDebugMessenger(VkInstance instance);

static const char** get_required_extensions();
static Bool checkValidationLayerSupport();
static void populateDebugUtilsMessengerCreateInfoEXT(VkDebugUtilsMessengerCreateInfoEXT *info);

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

    init_vulkan(&renderer);

    return renderer;
}

void init_vulkan(Renderer *renderer)
{   
    renderer->instance = create_instance();
    renderer->debugMessenger = setupDebugMessenger(renderer->instance);
}

VkInstance create_instance()
{
    VkInstance instance = VK_NULL_HANDLE;

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

    vk_call(vkCreateInstance(&info, NULL, &instance), 
            "vk instance created", 
            "failed to create vk instance");

    return instance;
}

VkDebugUtilsMessengerEXT setupDebugMessenger(VkInstance instance)
{
    VkDebugUtilsMessengerEXT debugMessenger = { 0 };

    VkDebugUtilsMessengerCreateInfoEXT createInfo = { 0 };
    populateDebugUtilsMessengerCreateInfoEXT(&createInfo);

    vk_call(createDebugUtilsMessenger(instance, &createInfo, NULL, &debugMessenger),
            "Debug messenger callback created",
            "Failed to create debug messenger callback");
    
    return debugMessenger;
}

const char **get_required_extensions()
{
    static const char *required_extensions[] = {
        "VK_KHR_surface",
        "VK_KHR_xcb_surface",
        "VK_EXT_debug_utils",
    };

    int num_extensions = 0;
    vk_call(vkEnumerateInstanceExtensionProperties(NULL, &num_extensions, NULL), 
            "Num extensions obtained", 
            "failed to obtain num extensions");

    INFO_LOG("num extensions %i", num_extensions);

    VkExtensionProperties extensionProperties[num_extensions];

    vk_call(vkEnumerateInstanceExtensionProperties(NULL, &num_extensions, extensionProperties),
            "Name extensions obtained",
            "failed to obtain name extensions");

    Bool allExtensionsSupported = TRUE;

    for(int i = 0; i < 3; i++){
        Bool extensionsSupported = FALSE;
        for(int j = 0; j < num_extensions; j++){
            if(memcmp(required_extensions[i], extensionProperties[j].extensionName, strlen(required_extensions[i])) == 0){
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
    vk_call(vkEnumerateInstanceLayerProperties(&nLayers, NULL),
            "layers obtained",
            "failed to obtain layers");

    INFO_LOG("num layers %i", nLayers);

    VkLayerProperties layerProperties[nLayers];

    vk_call(vkEnumerateInstanceLayerProperties(&nLayers, layerProperties),
            "layer's name obtained",
            "failed to obtain layer's name");
    
    Bool validationLayerSupport = FALSE;
    for(int i = 0; i < nLayers; i++){
        if(memcmp(validation_layers[0], layerProperties[i].layerName, strlen(validation_layers[0]))){
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
