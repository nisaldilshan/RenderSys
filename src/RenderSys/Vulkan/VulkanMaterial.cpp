#include "VulkanMaterial.h"
#include "VulkanTexture.h"
#include "VulkanRenderer3D.h"

namespace RenderSys
{

std::vector<VkDescriptorSetLayoutBinding> GetMaterialBindGroupBindings()
{
    static std::vector<VkDescriptorSetLayoutBinding> materialBindGroupBindings
    {
        VkDescriptorSetLayoutBinding{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}, // baseColor texture
        VkDescriptorSetLayoutBinding{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}, // normal texture
        VkDescriptorSetLayoutBinding{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}  // metallic-roughness texture
    };

    return materialBindGroupBindings;
}

VkDescriptorPool g_materialBindGroupPool = VK_NULL_HANDLE;
void CreateMaterialBindGroupPool()
{
    assert(g_materialBindGroupPool == VK_NULL_HANDLE);    
    constexpr uint32_t maxNumOfModels = 10;
    constexpr uint32_t maxMaterialsPerModel = 50;

    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.emplace_back(
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 * maxMaterialsPerModel * maxNumOfModels}
    );
    poolSizes.emplace_back(
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 * maxMaterialsPerModel * maxNumOfModels}
    );
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxMaterialsPerModel * maxNumOfModels;

    if (vkCreateDescriptorPool(GraphicsAPI::Vulkan::GetDevice(), &poolInfo, nullptr, &g_materialBindGroupPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

VkDescriptorPool GetMaterialBindGroupPool()
{
    assert(g_materialBindGroupPool != VK_NULL_HANDLE);   
    return g_materialBindGroupPool;
}

void DestroyMaterialBindGroupPool()
{
    assert(g_materialBindGroupPool != VK_NULL_HANDLE); 
    vkDeviceWaitIdle(GraphicsAPI::Vulkan::GetDevice());
    // when you destroy a descriptor pool, all descriptor sets allocated from that pool are automatically destroyed
    vkDestroyDescriptorPool(GraphicsAPI::Vulkan::GetDevice(), g_materialBindGroupPool, nullptr);
    g_materialBindGroupPool = VK_NULL_HANDLE;
}

VkDescriptorSetLayout g_materialBindGroupLayout = VK_NULL_HANDLE;
void CreateMaterialBindGroupLayout()
{
    assert(g_materialBindGroupLayout == VK_NULL_HANDLE);
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    auto bindings = GetMaterialBindGroupBindings();
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(GraphicsAPI::Vulkan::GetDevice(), &layoutInfo, nullptr, &g_materialBindGroupLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

VkDescriptorSetLayout GetMaterialBindGroupLayout()
{
    assert(g_materialBindGroupLayout != VK_NULL_HANDLE);
    return g_materialBindGroupLayout;
}

void DestroyMaterialBindGroupLayout()
{
    assert(g_materialBindGroupLayout != VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(GraphicsAPI::Vulkan::GetDevice(), g_materialBindGroupLayout, nullptr);
    g_materialBindGroupLayout = VK_NULL_HANDLE;
}

VulkanMaterialDescriptor::VulkanMaterialDescriptor(MaterialTextures& textures)
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = GetMaterialBindGroupPool();
    allocInfo.descriptorSetCount = 1;
    auto materialBindGroupLayout = GetMaterialBindGroupLayout();
    allocInfo.pSetLayouts = &materialBindGroupLayout;

    if (vkAllocateDescriptorSets(GraphicsAPI::Vulkan::GetDevice(), &allocInfo, &m_bindGroup) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    std::vector<VkWriteDescriptorSet> descriptorWrites;
    VkDescriptorImageInfo imageInfo{VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
    for (const auto &materialBindGroupBinding : GetMaterialBindGroupBindings())
    {
        VkWriteDescriptorSet textureWrite{};
        textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        textureWrite.dstSet = m_bindGroup;
        textureWrite.dstBinding = materialBindGroupBinding.binding;
        textureWrite.dstArrayElement = 0;
        textureWrite.descriptorType = materialBindGroupBinding.descriptorType;
        textureWrite.descriptorCount = 1;
        textureWrite.pBufferInfo = nullptr;
        textureWrite.pImageInfo = &imageInfo;
        textureWrite.pTexelBufferView = nullptr;

        descriptorWrites.push_back(textureWrite);
    }

    //assert(textures[0] != nullptr);
    auto baseColorTexture = textures[RenderSys::TextureIndices::DIFFUSE_MAP_INDEX];
    if (baseColorTexture)
    {
        auto platformTex = baseColorTexture->GetPlatformTexture();
        auto* baseColorImageInfo = platformTex->GetDescriptorImageInfoAddr();
        if (baseColorImageInfo->sampler == VK_NULL_HANDLE)
        {
            //baseColorImageInfo.sampler = m_defaultTextureSampler;
        }
        descriptorWrites[0].pImageInfo = baseColorImageInfo;
    }
    else
    {
        std::cerr << "Base color texture is null!" << std::endl;
        baseColorTexture = Texture::createDummy(128, 128);
        auto platformTex = baseColorTexture->GetPlatformTexture();
        auto* baseColorImageInfo = platformTex->GetDescriptorImageInfoAddr();
        descriptorWrites[0].pImageInfo = baseColorImageInfo;
    }

    const auto normalTexture = textures[RenderSys::TextureIndices::NORMAL_MAP_INDEX];
    if (normalTexture)
    {
        auto platformTex = normalTexture->GetPlatformTexture();
        auto* normalTextureImageInfo = platformTex->GetDescriptorImageInfoAddr();
        if (normalTextureImageInfo->sampler == VK_NULL_HANDLE)
        {
            //normalTextureImageInfo.sampler = m_defaultTextureSampler;
        }
        descriptorWrites[1].pImageInfo = normalTextureImageInfo;
    }
    else
    {
        auto platformTex = baseColorTexture->GetPlatformTexture();
        descriptorWrites[1].pImageInfo = platformTex->GetDescriptorImageInfoAddr();
    }

    const auto metallicRoughnessTexture = textures[RenderSys::TextureIndices::ROUGHNESS_METALLIC_MAP_INDEX];
    if (metallicRoughnessTexture)
    {
        auto platformTex = metallicRoughnessTexture->GetPlatformTexture();
        auto* metallicRoughnessTextureImageInfo = platformTex->GetDescriptorImageInfoAddr();
        if (metallicRoughnessTextureImageInfo->sampler == VK_NULL_HANDLE)
        {
            //metallicRoughnessTextureImageInfo.sampler = m_defaultTextureSampler;
        }
        descriptorWrites[2].pImageInfo = metallicRoughnessTextureImageInfo;
    }
    else
    {
        auto platformTex = baseColorTexture->GetPlatformTexture();
        descriptorWrites[2].pImageInfo = platformTex->GetDescriptorImageInfoAddr();
    }

    vkUpdateDescriptorSets(GraphicsAPI::Vulkan::GetDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    
}

VulkanMaterialDescriptor::~VulkanMaterialDescriptor()
{
    // All the descriptor sets are freed when the pool is destroyed
    m_bindGroup = VK_NULL_HANDLE;
}

} // namespace RenderSys