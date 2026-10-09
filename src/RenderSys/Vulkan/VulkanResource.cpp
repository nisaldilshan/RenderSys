#include "VulkanResource.h"
#include <stdexcept>

namespace RenderSys
{

std::vector<VkDescriptorSetLayoutBinding> GetResourceBindGroupBindings()
{
    static std::vector<VkDescriptorSetLayoutBinding> resourceBindGroupBindings
    {
        VkDescriptorSetLayoutBinding{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr} // baseColor texture
        // VkDescriptorSetLayoutBinding{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}, // normal texture
        // VkDescriptorSetLayoutBinding{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}  // metallic-roughness texture
    };

    return resourceBindGroupBindings;
}

// Resource descriptor sets come from a chain of pools. When the newest pool is full, another one is added,
// so the number of sets is not capped by a guess at the scene size.
std::vector<VkDescriptorPool> g_resourceBindGroupPools;

namespace
{
VkDescriptorPool CreateResourceBindGroupPoolBlock()
{
    constexpr uint32_t maxNumOfModels = 10;
    constexpr uint32_t maxMaterialsPerModel = 50;

    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.emplace_back(
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 * maxMaterialsPerModel * maxNumOfModels}
    );
    poolSizes.emplace_back(
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 * maxMaterialsPerModel * maxNumOfModels}
    );
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxMaterialsPerModel * maxNumOfModels;

    VkDescriptorPool pool = VK_NULL_HANDLE;
    if (vkCreateDescriptorPool(GraphicsAPI::Vulkan::GetDevice(), &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
    return pool;
}

VkResult AllocateResourceBindGroup(VkDescriptorPool pool, VkDescriptorSet& bindGroup)
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    auto resourceBindGroupLayout = GetResourceBindGroupLayout();
    allocInfo.pSetLayouts = &resourceBindGroupLayout;
    return vkAllocateDescriptorSets(GraphicsAPI::Vulkan::GetDevice(), &allocInfo, &bindGroup);
}
} // namespace

void CreateResourceBindGroupPool()
{
    assert(g_resourceBindGroupPools.empty());
    g_resourceBindGroupPools.push_back(CreateResourceBindGroupPoolBlock());
}

void DestroyResourceBindGroupPool()
{
    assert(!g_resourceBindGroupPools.empty());
    vkDeviceWaitIdle(GraphicsAPI::Vulkan::GetDevice());
    // when you destroy a descriptor pool, all descriptor sets allocated from that pool are automatically destroyed
    for (auto pool : g_resourceBindGroupPools)
    {
        vkDestroyDescriptorPool(GraphicsAPI::Vulkan::GetDevice(), pool, nullptr);
    }
    g_resourceBindGroupPools.clear();
}

VkDescriptorSetLayout g_resourceBindGroupLayout = VK_NULL_HANDLE;
void CreateResourceBindGroupLayout()
{
    assert(g_resourceBindGroupLayout == VK_NULL_HANDLE);
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    auto bindings = GetResourceBindGroupBindings();
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(GraphicsAPI::Vulkan::GetDevice(), &layoutInfo, nullptr, &g_resourceBindGroupLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

VkDescriptorSetLayout GetResourceBindGroupLayout()
{
    assert(g_resourceBindGroupLayout != VK_NULL_HANDLE);
    return g_resourceBindGroupLayout;
}

void DestroyResourceBindGroupLayout()
{
    assert(g_resourceBindGroupLayout != VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(GraphicsAPI::Vulkan::GetDevice(), g_resourceBindGroupLayout, nullptr);
    g_resourceBindGroupLayout = VK_NULL_HANDLE;
}

VulkanResourceDescriptor::VulkanResourceDescriptor()
{
    assert(!g_resourceBindGroupPools.empty());
    VkResult result = AllocateResourceBindGroup(g_resourceBindGroupPools.back(), m_bindGroup);
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
    {
        g_resourceBindGroupPools.push_back(CreateResourceBindGroupPoolBlock());
        result = AllocateResourceBindGroup(g_resourceBindGroupPools.back(), m_bindGroup);
    }

    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
}

VulkanResourceDescriptor::~VulkanResourceDescriptor()
{
    // All the descriptor sets are freed when the pool is destroyed
    m_bindGroup = VK_NULL_HANDLE;
}

void VulkanResourceDescriptor::AttachBuffer(uint32_t binding, const std::shared_ptr<RenderSys::VulkanBuffer>& buffer)
{
    auto bindings = GetResourceBindGroupBindings(); // Ensure the bindings are created
    assert(binding < bindings.size());

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_bindGroup;
    write.dstBinding = binding;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.descriptorType = bindings[binding].descriptorType;
    write.pBufferInfo = &buffer->GetBufferInfo();

    m_Writes.push_back(write);
}

void VulkanResourceDescriptor::Init()
{
    vkUpdateDescriptorSets(GraphicsAPI::Vulkan::GetDevice(), m_Writes.size(), m_Writes.data(), 0, nullptr);
}

} // namespace RenderSys