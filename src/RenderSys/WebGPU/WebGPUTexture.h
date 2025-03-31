#pragma once

#include <stdint.h>
#include <Walnut/GraphicsAPI/WebGPUGraphics.h>
#include <RenderSys/TextureSampler.h>

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
    wgpu::Texture m_image;
    wgpu::TextureView m_imageView;
    wgpu::TextureDescriptor m_textureDesc;
    // VkDescriptorImageInfo m_descriptorImageInfo;
    // VkImageCreateInfo m_imageCreateInfo;
};


} // namespace RenderSys