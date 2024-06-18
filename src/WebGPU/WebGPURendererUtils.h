#pragma once

#include "WebGPURenderer2D.h"

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
        vAttrib.shaderLocation = renderSysBufferLayout.attributes[attribCounter].shaderLocation;
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
    else
    {
        assert(false);
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

wgpu::BindGroupLayoutEntry* GetWebGPUBindGroupLayoutEntriesPtr(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries)
{
    static std::vector<wgpu::BindGroupLayoutEntry> bGroupLayoutArr(bindGroupLayoutEntries.size());

    for (size_t i = 0; i < bGroupLayoutArr.size(); i++)    
    {
        bGroupLayoutArr[i].binding = bindGroupLayoutEntries[i].binding;
        bGroupLayoutArr[i].visibility = GetWebGPUShaderStageVisibility(bindGroupLayoutEntries[i].visibility);
        bGroupLayoutArr[i].buffer.type = GetWebGPUBufferBindingType(bindGroupLayoutEntries[i].buffer.type);
        bGroupLayoutArr[i].buffer.minBindingSize = bindGroupLayoutEntries[i].buffer.minBindingSize;
        bGroupLayoutArr[i].buffer.hasDynamicOffset = bindGroupLayoutEntries[i].buffer.hasDynamicOffset;
    }
    
    return &bGroupLayoutArr[0];
}