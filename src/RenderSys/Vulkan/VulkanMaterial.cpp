#include "VulkanMaterial.h"
#include "VulkanTexture.h"
#include "VulkanRenderer3D.h"

namespace RenderSys
{

VulkanMaterialDescriptor::VulkanMaterialDescriptor(MaterialTextures& textures)
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = GraphicsAPI::VulkanRenderer3D::GetMaterialBindGroupPool();
    allocInfo.descriptorSetCount = 1;
    auto materialBindGroupLayout = GraphicsAPI::VulkanRenderer3D::GetMaterialBindGroupLayout();
    allocInfo.pSetLayouts = &materialBindGroupLayout;

    if (vkAllocateDescriptorSets(GraphicsAPI::Vulkan::GetDevice(), &allocInfo, &m_materialbindGroup) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    std::vector<VkWriteDescriptorSet> descriptorWrites;
    VkDescriptorImageInfo imageInfo{VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
    for (const auto &materialBindGroupBinding : GraphicsAPI::VulkanRenderer3D::GetMaterialBindGroupBindings())
    {
        VkWriteDescriptorSet textureWrite{};
        textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        textureWrite.dstSet = m_materialbindGroup;
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
    const auto baseColorTexture = textures[RenderSys::TextureIndices::DIFFUSE_MAP_INDEX];
    if (baseColorTexture)
    {
        auto platformTex = baseColorTexture->GetPlatformTexture();
        auto baseColorImageInfo = platformTex->GetDescriptorImageInfo();
        if (baseColorImageInfo.sampler == VK_NULL_HANDLE)
        {
            //baseColorImageInfo.sampler = m_defaultTextureSampler;
        }
        descriptorWrites[0].pImageInfo = &baseColorImageInfo;
    }
    else
    {
        std::cerr << "Base color texture is null!" << std::endl;
        return;
    }

    const auto normalTexture = textures[RenderSys::TextureIndices::NORMAL_MAP_INDEX];
    if (normalTexture)
    {
        auto platformTex = normalTexture->GetPlatformTexture();
        auto normalTextureImageInfo = platformTex->GetDescriptorImageInfo();
        if (normalTextureImageInfo.sampler == VK_NULL_HANDLE)
        {
            //normalTextureImageInfo.sampler = m_defaultTextureSampler;
        }
        descriptorWrites[1].pImageInfo = &normalTextureImageInfo;
    }
    else
    {
        auto platformTex = baseColorTexture->GetPlatformTexture();
        descriptorWrites[1].pImageInfo = &platformTex->GetDescriptorImageInfo();
    }

    const auto metallicRoughnessTexture = textures[RenderSys::TextureIndices::ROUGHNESS_METALLIC_MAP_INDEX];
    if (metallicRoughnessTexture)
    {
        auto platformTex = metallicRoughnessTexture->GetPlatformTexture();
        auto metallicRoughnessTextureImageInfo = platformTex->GetDescriptorImageInfo();
        if (metallicRoughnessTextureImageInfo.sampler == VK_NULL_HANDLE)
        {
            //metallicRoughnessTextureImageInfo.sampler = m_defaultTextureSampler;
        }
        descriptorWrites[2].pImageInfo = &metallicRoughnessTextureImageInfo;
    }
    else
    {
        auto platformTex = baseColorTexture->GetPlatformTexture();
        descriptorWrites[2].pImageInfo = &platformTex->GetDescriptorImageInfo();
    }

    vkUpdateDescriptorSets(GraphicsAPI::Vulkan::GetDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    
}

VulkanMaterialDescriptor::~VulkanMaterialDescriptor()
{

}

} // namespace RenderSys