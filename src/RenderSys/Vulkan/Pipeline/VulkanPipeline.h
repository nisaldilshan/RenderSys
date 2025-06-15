#pragma once
#include <Walnut/GraphicsAPI/VulkanGraphics.h>

namespace RenderSys
{
namespace Vulkan 
{

class Pipeline
{
public:

    static VkPipelineRasterizationStateCreateInfo getRasterizerInfo();
};


} // namespace Vulkan
} // namespace RenderSys