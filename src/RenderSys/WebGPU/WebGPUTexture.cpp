#include "WebGPUTexture.h"

namespace RenderSys
{

WebGPUTexture::WebGPUTexture(uint32_t width, uint32_t height, uint32_t mipMapLevelCount)
{
	m_textureDesc.dimension = wgpu::TextureDimension::_2D;
	m_textureDesc.size = {width, height, 1};
	m_textureDesc.format = wgpu::TextureFormat::RGBA8Unorm;;
	m_textureDesc.mipLevelCount = mipMapLevelCount;
	m_textureDesc.sampleCount = 1;
	m_textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
	m_textureDesc.viewFormatCount = 0;
	m_textureDesc.viewFormats = nullptr;
	m_image = GraphicsAPI::WebGPU::GetDevice().createTexture(m_textureDesc);
	std::cout << "texture created: " << m_image << std::endl;

    wgpu::TextureViewDescriptor textureViewDesc;
	textureViewDesc.aspect = wgpu::TextureAspect::All;
	textureViewDesc.baseArrayLayer = 0;
	textureViewDesc.arrayLayerCount = 1;
	textureViewDesc.baseMipLevel = 0;
	textureViewDesc.mipLevelCount = m_textureDesc.mipLevelCount;
	textureViewDesc.dimension = wgpu::TextureViewDimension::_2D;
	textureViewDesc.format = m_textureDesc.format;
	m_imageView = m_image.createView(textureViewDesc);
	std::cout << "texture view created: " << m_imageView << std::endl;
}

WebGPUTexture::~WebGPUTexture()
{
    if (m_image)
    {
        std::cout << "destroying texture : " << m_image << std::endl;
        m_image.destroy();
        m_image = nullptr;
        m_imageView = nullptr;
    }
}

void WebGPUTexture::SetData(unsigned char *textureData)
{
    wgpu::ImageCopyTexture destination;
    destination.texture = m_image;
    destination.origin = { 0, 0, 0 }; // equivalent of the offset argument of Queue::writeBuffer
    destination.aspect = wgpu::TextureAspect::All; // only relevant for depth/Stencil textures

    // Arguments telling how the C++ side pixel memory is laid out
    wgpu::TextureDataLayout source;
    source.offset = 0;

    wgpu::Extent3D mipLevelSize = m_textureDesc.size;
	std::vector<uint8_t> previousLevelPixels;
    wgpu::Extent3D previousMipLevelSize;
	for (uint32_t level = 0; level < m_textureDesc.mipLevelCount; ++level) 
    {
		// Create image data
        std::vector<uint8_t> pixels(4 * mipLevelSize.width * mipLevelSize.height);
        if (level == 0) 
        {
            memcpy(pixels.data(), textureData, 4 * mipLevelSize.width * mipLevelSize.height);
        } 
		else
        {
            // Create mip level data
            for (uint32_t i = 0; i < mipLevelSize.width; ++i)
            {
                for (uint32_t j = 0; j < mipLevelSize.height; ++j)
                {
                    uint8_t *p = &pixels[4 * (j * mipLevelSize.width + i)];
                    // Get the corresponding 4 pixels from the previous level
                    uint8_t *p00 = &previousLevelPixels[4 * ((2 * j + 0) * previousMipLevelSize.width + (2 * i + 0))];
                    uint8_t *p01 = &previousLevelPixels[4 * ((2 * j + 0) * previousMipLevelSize.width + (2 * i + 1))];
                    uint8_t *p10 = &previousLevelPixels[4 * ((2 * j + 1) * previousMipLevelSize.width + (2 * i + 0))];
                    uint8_t *p11 = &previousLevelPixels[4 * ((2 * j + 1) * previousMipLevelSize.width + (2 * i + 1))];
                    // Average
                    p[0] = (p00[0] + p01[0] + p10[0] + p11[0]) / 4;
                    p[1] = (p00[1] + p01[1] + p10[1] + p11[1]) / 4;
                    p[2] = (p00[2] + p01[2] + p10[2] + p11[2]) / 4;
                    p[3] = (p00[3] + p01[3] + p10[3] + p11[3]) / 4;
                }
            }
        }

        // Change this to the current level
        destination.mipLevel = level;
        source.bytesPerRow = 4 * mipLevelSize.width;
        source.rowsPerImage = mipLevelSize.height;
        GraphicsAPI::WebGPU::GetQueue().writeTexture(destination, pixels.data(), pixels.size(), source, mipLevelSize);

        previousLevelPixels = std::move(pixels);
        previousMipLevelSize = mipLevelSize;
        mipLevelSize.width /= 2;
        mipLevelSize.height /= 2;
    }
}

void WebGPUTexture::SetSampler(RenderSys::TextureSampler sampler)
{
}

wgpu::TextureView WebGPUTexture::GetImageView() const
{
    return m_imageView;
}
}