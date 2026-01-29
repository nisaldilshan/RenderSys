#pragma once
#include <Walnut/GraphicsAPI/VulkanGraphics.h>
#include <vk_mem_alloc.h>
#include <memory>

namespace RenderSys
{

class VulkanTexture;
namespace Vulkan
{

class ShadowMap
{
public:
    enum class SubPassesShadow
    {
        SUBPASS_SHADOW = 0,
        NUMBER_OF_SUBPASSES
    };
    enum class ShadowRenderTargets
    {
        ATTACHMENT_DEPTH = 0,
        NUMBER_OF_ATTACHMENTS
    };

    ShadowMap(std::shared_ptr<RenderSys::VulkanTexture> shadowMapTexture);
    ~ShadowMap();

    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    VkFramebuffer GetShadowFrameBuffer() { return m_ShadowFramebuffer; }
    VkRenderPass GetShadowRenderPass() { return m_ShadowRenderPass; }
    VkExtent2D GetShadowMapExtent() { return m_ShadowMapExtent; }
    const VkDescriptorImageInfo& GetDescriptorImageInfo() const { return m_DescriptorImageInfo; }
    const VkImage GetShadowDepthImage() const { return m_ShadowDepthImage; }

private:
    void SetShadowDepthResources(std::shared_ptr<RenderSys::VulkanTexture> shadowMapTexture);
    void CreateShadowRenderPass();
    void CreateShadowFramebuffer();

    VkFormat m_DepthFormat{VkFormat::VK_FORMAT_UNDEFINED};
    VkExtent2D m_ShadowMapExtent{};
    VkFramebuffer m_ShadowFramebuffer{nullptr};
    VkRenderPass m_ShadowRenderPass{nullptr};

    VkImage m_ShadowDepthImage{nullptr};
    VkImageLayout m_ImageLayout{};
    VkImageView m_ShadowDepthImageView{nullptr};

    VkDescriptorImageInfo m_DescriptorImageInfo{};
};

} // namespace Vulkan
} // namespace RenderSys
