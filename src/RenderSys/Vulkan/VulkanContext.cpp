#include "VulkanContext.h"

#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>

namespace RenderSys
{
namespace Vulkan
{

namespace
{
Context g_context;

// Set while the current context is one CreateContext made
bool g_ownsContext = false;
VkDebugUtilsMessengerEXT g_debugMessenger = VK_NULL_HANDLE;
std::atomic<uint32_t> g_validationErrorCount{0};

VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                      VkDebugUtilsMessageTypeFlagsEXT,
                                                      const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                                      void*)
{
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        g_validationErrorCount++;
    fprintf(stderr, "[vulkan validation] %s\n", callbackData->pMessage);
    return VK_FALSE;
}

bool HasInstanceLayer(const char* layerName)
{
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> layers(count);
    vkEnumerateInstanceLayerProperties(&count, layers.data());
    for (const auto& layer : layers)
        if (strcmp(layer.layerName, layerName) == 0)
            return true;
    return false;
}

bool HasInstanceExtension(const char* extensionName)
{
    uint32_t count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> extensions(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data());
    for (const auto& extension : extensions)
        if (strcmp(extension.extensionName, extensionName) == 0)
            return true;
    return false;
}

bool HasDeviceExtension(VkPhysicalDevice physicalDevice, const char* extensionName)
{
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> extensions(count);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, extensions.data());
    for (const auto& extension : extensions)
        if (strcmp(extension.extensionName, extensionName) == 0)
            return true;
    return false;
}

// First queue family that can draw, and present to the surface when there is one
std::optional<uint32_t> FindGraphicsQueueFamily(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, families.data());
    for (uint32_t i = 0; i < count; i++)
    {
        if (!(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
            continue;
        if (surface != VK_NULL_HANDLE)
        {
            VkBool32 canPresent = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &canPresent);
            if (!canPresent)
                continue;
        }
        return i;
    }
    return std::nullopt;
}

void DestroyInstanceObjects(VkInstance instance, VkDebugUtilsMessengerEXT messenger, VkSurfaceKHR surface)
{
    if (surface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(instance, surface, nullptr);
    if (messenger != VK_NULL_HANDLE)
    {
        auto destroyMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
        destroyMessenger(instance, messenger, nullptr);
    }
    vkDestroyInstance(instance, nullptr);
}
} // namespace

void SetContext(const Context& context)
{
    assert(context.device != VK_NULL_HANDLE && context.queue != VK_NULL_HANDLE);
    g_context = context;
}

const Context& GetContext()
{
    assert(g_context.device != VK_NULL_HANDLE && "RenderSys::Vulkan::SetContext must be called before using RenderSys");
    return g_context;
}

bool CreateContext(const ContextCreateInfo& createInfo)
{
    assert(g_context.device == VK_NULL_HANDLE && "a Vulkan context is already set");

    uint32_t instanceVersion = VK_API_VERSION_1_0;
    vkEnumerateInstanceVersion(&instanceVersion);
    if (instanceVersion < VK_API_VERSION_1_2)
    {
        fprintf(stderr, "RenderSys: Vulkan 1.2 is required, the loader supports %u.%u\n",
                VK_API_VERSION_MAJOR(instanceVersion), VK_API_VERSION_MINOR(instanceVersion));
        return false;
    }

    std::vector<const char*> layers;
    std::vector<const char*> extensions;
    if (createInfo.surface)
        extensions = createInfo.surface->instanceExtensions;

    // MoltenVK on macOS is a portability driver, which the loader hides unless asked for
    VkInstanceCreateFlags instanceFlags = 0;
    if (HasInstanceExtension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
    {
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        instanceFlags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }

    bool useDebugUtils = false;
    if (createInfo.validation)
    {
        if (HasInstanceLayer("VK_LAYER_KHRONOS_validation"))
        {
            layers.push_back("VK_LAYER_KHRONOS_validation");
            useDebugUtils = HasInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            if (useDebugUtils)
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        else
        {
            fprintf(stderr, "RenderSys: validation requested but VK_LAYER_KHRONOS_validation is not installed\n");
        }
    }

    VkDebugUtilsMessengerCreateInfoEXT messengerInfo{};
    messengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    messengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                  | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    messengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                              | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                              | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    messengerInfo.pfnUserCallback = DebugMessengerCallback;

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = createInfo.applicationName;
    appInfo.pEngineName = "RenderSys";
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pNext = useDebugUtils ? &messengerInfo : nullptr; // also reports on instance creation itself
    instanceInfo.flags = instanceFlags;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
    instanceInfo.ppEnabledLayerNames = layers.data();
    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instanceInfo.ppEnabledExtensionNames = extensions.data();

    g_validationErrorCount = 0;
    VkInstance instance = VK_NULL_HANDLE;
    if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS)
    {
        fprintf(stderr, "RenderSys: vkCreateInstance failed\n");
        return false;
    }

    VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
    if (useDebugUtils)
    {
        auto createMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
        createMessenger(instance, &messengerInfo, nullptr, &messenger);
    }

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (createInfo.surface)
    {
        surface = createInfo.surface->createSurface(instance);
        if (surface == VK_NULL_HANDLE)
        {
            fprintf(stderr, "RenderSys: the host could not create a surface\n");
            DestroyInstanceObjects(instance, messenger, surface);
            return false;
        }
    }

    // Prefer a discrete GPU, then an integrated one
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties physicalDeviceProperties{};
    uint32_t queueFamily = 0;
    int bestScore = -1;
    for (VkPhysicalDevice candidate : physicalDevices)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(candidate, &properties);
        if (properties.apiVersion < VK_API_VERSION_1_2)
            continue;
        if (surface != VK_NULL_HANDLE && !HasDeviceExtension(candidate, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
            continue;
        const auto family = FindGraphicsQueueFamily(candidate, surface);
        if (!family)
            continue;

        const int score = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 2
                        : properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? 1 : 0;
        if (score > bestScore)
        {
            bestScore = score;
            physicalDevice = candidate;
            physicalDeviceProperties = properties;
            queueFamily = *family;
        }
    }
    if (physicalDevice == VK_NULL_HANDLE)
    {
        fprintf(stderr, "RenderSys: no GPU supports Vulkan 1.2 with a graphics queue%s\n",
                surface != VK_NULL_HANDLE ? " that can present to the surface" : "");
        DestroyInstanceObjects(instance, messenger, surface);
        return false;
    }

    VkPhysicalDeviceVulkan12Features supported12{};
    supported12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    VkPhysicalDeviceFeatures2 supported{};
    supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    supported.pNext = &supported12;
    vkGetPhysicalDeviceFeatures2(physicalDevice, &supported);

    ContextFeatures features;
    features.descriptorIndexing = supported12.shaderSampledImageArrayNonUniformIndexing
                               && supported12.runtimeDescriptorArray
                               && supported12.descriptorBindingPartiallyBound
                               && supported12.descriptorBindingVariableDescriptorCount
                               && supported12.descriptorBindingSampledImageUpdateAfterBind;

    VkPhysicalDeviceVulkan12Features enabled12{};
    enabled12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    if (features.descriptorIndexing)
    {
        enabled12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        enabled12.runtimeDescriptorArray = VK_TRUE;
        enabled12.descriptorBindingPartiallyBound = VK_TRUE;
        enabled12.descriptorBindingVariableDescriptorCount = VK_TRUE;
        enabled12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    }
    VkPhysicalDeviceFeatures2 enabled{};
    enabled.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    enabled.pNext = &enabled12;

    const float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = queueFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &queuePriority;

    std::vector<const char*> deviceExtensions;
    if (surface != VK_NULL_HANDLE)
        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    // A portability driver must have this enabled; it is declared in vulkan_beta.h, hence the literal
    constexpr const char* portabilitySubset = "VK_KHR_portability_subset";
    if (HasDeviceExtension(physicalDevice, portabilitySubset))
        deviceExtensions.push_back(portabilitySubset);

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext = &enabled;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VkDevice device = VK_NULL_HANDLE;
    if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS)
    {
        fprintf(stderr, "RenderSys: vkCreateDevice failed\n");
        DestroyInstanceObjects(instance, messenger, surface);
        return false;
    }

    Context context;
    context.instance = instance;
    context.physicalDevice = physicalDevice;
    context.device = device;
    context.queueFamily = queueFamily;
    vkGetDeviceQueue(device, queueFamily, 0, &context.queue);
    context.surface = surface;
    context.features = features;
    SetContext(context);
    g_ownsContext = true;
    g_debugMessenger = messenger;

    printf("RenderSys: Vulkan device %s, validation %s, descriptor indexing %s\n",
           physicalDeviceProperties.deviceName,
           layers.empty() ? "off" : "on",
           features.descriptorIndexing ? "on" : "off");
    return true;
}

void DestroyContext()
{
    assert(g_ownsContext && "DestroyContext only destroys a context made by CreateContext");
    vkDeviceWaitIdle(g_context.device);
    vkDestroyDevice(g_context.device, nullptr);
    DestroyInstanceObjects(g_context.instance, g_debugMessenger, g_context.surface);

    g_context = Context{};
    g_ownsContext = false;
    g_debugMessenger = VK_NULL_HANDLE;
}

uint32_t GetValidationErrorCount()
{
    return g_validationErrorCount;
}

bool IsValidationEnabled()
{
    return g_debugMessenger != VK_NULL_HANDLE;
}

VkInstance GetInstance()
{
    return GetContext().instance;
}

VkPhysicalDevice GetPhysicalDevice()
{
    return GetContext().physicalDevice;
}

VkDevice GetDevice()
{
    return GetContext().device;
}

uint32_t GetQueueFamily()
{
    return GetContext().queueFamily;
}

VkQueue GetQueue()
{
    return GetContext().queue;
}

void CheckVkResult(VkResult err)
{
    if (err == VK_SUCCESS)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

void QueueSubmitAndWait(const VkSubmitInfo& submitInfo)
{
    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence;
    auto err = vkCreateFence(GetDevice(), &fenceCreateInfo, nullptr, &fence);
    CheckVkResult(err);

    err = vkQueueSubmit(GetQueue(), 1, &submitInfo, fence);
    CheckVkResult(err);

    constexpr uint64_t fenceTimeoutNs = 100'000'000'000;
    err = vkWaitForFences(GetDevice(), 1, &fence, VK_TRUE, fenceTimeoutNs);
    CheckVkResult(err);

    vkDestroyFence(GetDevice(), fence, nullptr);
}

uint32_t FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(GetPhysicalDevice(), &memoryProperties);
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties && (typeBits & (1 << i)))
            return i;
    }
    return 0xffffffff;
}

} // namespace Vulkan
} // namespace RenderSys
