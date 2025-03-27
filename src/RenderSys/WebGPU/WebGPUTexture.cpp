#include "WebGPUTexture.h"

namespace RenderSys
{

WebGPUTexture::WebGPUTexture(uint32_t width, uint32_t height, uint32_t mipMapLevelCount)
{
}

WebGPUTexture::~WebGPUTexture()
{
}

void WebGPUTexture::SetData(unsigned char *textureData)
{
}

void WebGPUTexture::SetSampler(RenderSys::TextureSampler sampler)
{
}

wgpu::TextureView WebGPUTexture::GetImageView() const
{
    return wgpu::TextureView();
}
}