#include "VulkanPbrRenderPipeline.h"

#include <vector>

namespace RenderSys {

namespace Vulkan {

PbrRenderPipeline::PbrRenderPipeline(VkRenderPass renderPass,
    std::vector<VkDescriptorSetLayout> &descriptorSetLayouts) {
}

PbrRenderPipeline::~PbrRenderPipeline() {
}

void PbrRenderPipeline::CreatePipelineLayout(std::vector<VkDescriptorSetLayout> &descriptorSetLayouts) {
}

void PbrRenderPipeline::CreatePipeline(VkRenderPass renderPass) {
}

}

}
