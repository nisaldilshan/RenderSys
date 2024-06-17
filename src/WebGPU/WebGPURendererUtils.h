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

wgpu::ShaderStage GetWebGPUShaderStageVisibility(RenderSys::ShaderStage shaderStage)
{

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