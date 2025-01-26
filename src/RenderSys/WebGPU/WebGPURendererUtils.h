#pragma once

namespace GraphicsAPI
{

wgpu::VertexFormat GetWebGPUVertexFormat(RenderSys::VertexFormat renderSysFormat)
{
    if (renderSysFormat == RenderSys::VertexFormat::Float32x2)
    {
        return wgpu::VertexFormat::Float32x2;
    }
    else if (renderSysFormat == RenderSys::VertexFormat::Float32x3)
    {
        return wgpu::VertexFormat::Float32x3;
    }
    else if (renderSysFormat == RenderSys::VertexFormat::Uint16x2)
    {
        return wgpu::VertexFormat::Uint16x2;
    }
    else if (renderSysFormat == RenderSys::VertexFormat::Snorm16x4)
    {
        return wgpu::VertexFormat::Snorm16x4;
    }
    else
    {
        assert(false);
        return wgpu::VertexFormat::Undefined;
    }
}

wgpu::VertexBufferLayout GetWebGPUVertexBufferLayout(RenderSys::VertexBufferLayout renderSysBufferLayout)
{
    wgpu::VertexBufferLayout layout;
    layout.arrayStride = renderSysBufferLayout.arrayStride;

    switch (renderSysBufferLayout.stepMode)
    {
    case RenderSys::VertexStepMode::Vertex:
        layout.stepMode = wgpu::VertexStepMode::Vertex;
        break;
    case RenderSys::VertexStepMode::Instance:
        assert(false);
        break;
    case RenderSys::VertexStepMode::VertexBufferNotUsed:
        assert(false);
        break;
    }
    
    layout.attributeCount = renderSysBufferLayout.attributeCount;
    static std::vector<wgpu::VertexAttribute> vAttribArr(layout.attributeCount);
    
    int attribCounter = 0;
    for (wgpu::VertexAttribute& vAttrib : vAttribArr)
    {
        vAttrib.format = GetWebGPUVertexFormat(renderSysBufferLayout.attributes[attribCounter].format);
        vAttrib.offset = renderSysBufferLayout.attributes[attribCounter].offset;
        vAttrib.shaderLocation = renderSysBufferLayout.attributes[attribCounter].location;
        attribCounter++;
    }
    
    layout.attributes = &vAttribArr[0];
                                            
    return layout;
}

WGPUShaderStageFlags GetWebGPUShaderStageVisibility(RenderSys::ShaderStage shaderStage)
{
    WGPUShaderStageFlags result;
    if (static_cast<uint32_t>(shaderStage) == 1) // RenderSys::ShaderStage::Vertex
    {
        result = wgpu::ShaderStage::Vertex;
    }
    else if (static_cast<uint32_t>(shaderStage) == 2) // RenderSys::ShaderStage::Fragment
    {
        result = wgpu::ShaderStage::Fragment;
    }
    else if (static_cast<uint32_t>(shaderStage) == 3) // RenderSys::ShaderStage::Vertex | RenderSys::ShaderStage::Fragment
    {
        result = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
    }
    else if (static_cast<uint32_t>(shaderStage) == 4) // RenderSys::ShaderStage::Vertex | RenderSys::ShaderStage::Fragment
    {
        result = wgpu::ShaderStage::Compute;
    }
    else
    {
        assert(false);
    }
    
    return result;
}

wgpu::BufferBindingType GetWebGPUBufferBindingType(RenderSys::BufferBindingType type)
{
    if (type == RenderSys::BufferBindingType::Uniform)
    {
        return wgpu::BufferBindingType::Uniform;
    }
    else if (type == RenderSys::BufferBindingType::Storage)
    {
        return wgpu::BufferBindingType::Storage;
    }
    else if (type == RenderSys::BufferBindingType::ReadOnlyStorage)
    {
        return wgpu::BufferBindingType::ReadOnlyStorage;
    }
    else if (type == RenderSys::BufferBindingType::Undefined)
    {
        std::cout << "using undefined buffer type" << std::endl;
        return wgpu::BufferBindingType::Undefined;
    }
    else
    {
        assert(false);
        return wgpu::BufferBindingType::Undefined;
    }
}

wgpu::BindGroupLayoutEntry* GetWebGPUBindGroupLayoutEntryPtr(RenderSys::BindGroupLayoutEntry bindGroupLayoutEntry)
{
    static std::vector<wgpu::BindGroupLayoutEntry> bGroupLayoutArr(1);
    bGroupLayoutArr[0].binding = bindGroupLayoutEntry.binding;
    bGroupLayoutArr[0].visibility = GetWebGPUShaderStageVisibility(bindGroupLayoutEntry.visibility);
    bGroupLayoutArr[0].buffer.type = wgpu::BufferBindingType::Uniform; //bindGroupLayoutEntry.buffer.type;
    bGroupLayoutArr[0].buffer.minBindingSize = bindGroupLayoutEntry.buffer.minBindingSize;
    bGroupLayoutArr[0].buffer.hasDynamicOffset = bindGroupLayoutEntry.buffer.hasDynamicOffset;
    return &bGroupLayoutArr[0];
}

wgpu::TextureSampleType GetWebGPUTextureSamplingType(RenderSys::TextureSampleType type)
{
    if (type == RenderSys::TextureSampleType::Float)
    {
        return wgpu::TextureSampleType::Float;
    }
    else if (type == RenderSys::TextureSampleType::UnfilterableFloat)
    {
        return wgpu::TextureSampleType::UnfilterableFloat;
    }
    else
    {
        assert(false);
        return wgpu::TextureSampleType::Undefined;
    }
}

std::vector<wgpu::BindGroupLayoutEntry> gGroupLayoutArr;
std::vector<wgpu::BindGroupLayoutEntry> GetWebGPUBindGroupLayoutEntriesPtr(const std::vector<RenderSys::BindGroupLayoutEntry> &bindGroupLayoutEntries, bool addSamplerBinding)
{
    if (gGroupLayoutArr.size() == 0)
    {
        gGroupLayoutArr.resize(bindGroupLayoutEntries.size());
        for (size_t i = 0; i < gGroupLayoutArr.size(); i++)    
        {
            gGroupLayoutArr[i].binding = bindGroupLayoutEntries[i].binding;
            gGroupLayoutArr[i].visibility = GetWebGPUShaderStageVisibility(bindGroupLayoutEntries[i].visibility);

            if (bindGroupLayoutEntries[i].buffer.type != RenderSys::BufferBindingType::Undefined)
            {
                gGroupLayoutArr[i].buffer.type = GetWebGPUBufferBindingType(bindGroupLayoutEntries[i].buffer.type);
                gGroupLayoutArr[i].buffer.minBindingSize = bindGroupLayoutEntries[i].buffer.minBindingSize;
                gGroupLayoutArr[i].buffer.hasDynamicOffset = bindGroupLayoutEntries[i].buffer.hasDynamicOffset;
            }

            if (bindGroupLayoutEntries[i].texture.sampleType != RenderSys::TextureSampleType::Undefined)
            {
                std::cout << "binding texture" << std::endl;
                gGroupLayoutArr[i].texture.sampleType = GetWebGPUTextureSamplingType(bindGroupLayoutEntries[i].texture.sampleType);
                gGroupLayoutArr[i].texture.viewDimension = wgpu::TextureViewDimension::_2D;
            }

            if (bindGroupLayoutEntries[i].sampler.type != RenderSys::SamplerBindingType::Undefined)
            {
                std::cout << "binding sampler" << std::endl;
                gGroupLayoutArr[i].sampler.type = wgpu::SamplerBindingType::Filtering;
            }
        }

        if (addSamplerBinding)
        {
            auto& textureSamplerBindGroupLayoutEntry = gGroupLayoutArr.emplace_back();
            textureSamplerBindGroupLayoutEntry.binding = gGroupLayoutArr.size() - 1;
            textureSamplerBindGroupLayoutEntry.visibility = wgpu::ShaderStage::Fragment;
            textureSamplerBindGroupLayoutEntry.sampler.type = wgpu::SamplerBindingType::Filtering;
        }
    }
    
    return gGroupLayoutArr;
}

}