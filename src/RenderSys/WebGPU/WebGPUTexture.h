#pragma once

#include <stdint.h>
#include <Walnut/GraphicsAPI/WebGPUGraphics.h>

namespace RenderSys
{

class WebGPUTexture
{
public:
    WebGPUTexture() = delete;
    WebGPUTexture(uint32_t width, uint32_t height, uint32_t mipMapLevelCount);
    ~WebGPUTexture();
    void SetData(unsigned char *textureData);
    void SetSampler(RenderSys::TextureSampler sampler);
    wgpu::TextureView GetImageView() const;
private:
    // VkImage m_image;
    wgpu::TextureView m_textureView;
    // VkDescriptorImageInfo m_descriptorImageInfo;
    // VkImageCreateInfo m_imageCreateInfo;
};


} // namespace RenderSys