#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>
#include <functional>
#include <vector>

namespace RenderSys
{
namespace Vulkan
{

// Optional device features RenderSys enabled, so renderers can pick a fallback path.
struct ContextFeatures
{
    // Vulkan 1.2 descriptor indexing: non-uniform sampled image arrays, partially bound,
    // variable count and update-after-bind bindings. Needed for bindless textures.
    bool descriptorIndexing = false;
};

// Every Vulkan handle RenderSys records and submits with. Either a host fills it and
// calls SetContext (the Walnut adapter wraps Walnut's device), or CreateContext makes
// RenderSys own the instance and device.
struct Context
{
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    uint32_t queueFamily = 0;
    VkQueue queue = VK_NULL_HANDLE;
    // Pool the host's UI draws from, used to show render targets as ImGui images. Optional.
    VkDescriptorPool uiDescriptorPool = VK_NULL_HANDLE;
    const VkAllocationCallbacks* allocator = nullptr;
    // The host window's surface; the graphics queue can present to it. Null when headless.
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    ContextFeatures features;
};

// A window the host owns, handed over as a surface, so RenderSys never links a window system.
struct SurfaceProvider
{
    std::vector<const char*> instanceExtensions; // e.g. from SDL_Vulkan_GetInstanceExtensions
    std::function<VkSurfaceKHR(VkInstance)> createSurface;
};

struct ContextCreateInfo
{
    const char* applicationName = "RenderSys";
    // Enables VK_LAYER_KHRONOS_validation when it is installed and reports its messages on stderr
    bool validation = false;
    const SurfaceProvider* surface = nullptr; // null renders headless
};

void SetContext(const Context& context);
const Context& GetContext();

// Creates an instance, picks a GPU with a graphics queue (that can present, given a surface)
// and creates a device on it, then makes it the current context. Requires Vulkan 1.2.
bool CreateContext(const ContextCreateInfo& createInfo);
// Destroys a context made by CreateContext. Every RenderSys object must be destroyed first.
void DestroyContext();
// Validation errors reported since CreateContext; always 0 without validation.
uint32_t GetValidationErrorCount();
// Whether CreateContext loaded the validation layer and is counting its errors.
bool IsValidationEnabled();

VkInstance GetInstance();
VkPhysicalDevice GetPhysicalDevice();
VkDevice GetDevice();
uint32_t GetQueueFamily();
VkQueue GetQueue();

void CheckVkResult(VkResult err);
// Submits to the context's queue and blocks until the GPU has finished.
void QueueSubmitAndWait(const VkSubmitInfo& submitInfo);
uint32_t FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties);

} // namespace Vulkan
} // namespace RenderSys
