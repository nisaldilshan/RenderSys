#pragma once
#include <RenderSys/Vulkan/VulkanContext.h>

namespace RenderSys
{
namespace Vulkan
{

// Wraps the device Walnut created, so the examples keep running inside Walnut::Application.
Context MakeContextFromWalnut();

} // namespace Vulkan
} // namespace RenderSys
