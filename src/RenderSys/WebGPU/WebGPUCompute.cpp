#include "WebGPUCompute.h"
#include "WebGPURendererUtils.h"

namespace GraphicsAPI
{

WebGPUCompute::~WebGPUCompute()
{
    Destroy();
}

void WebGPUCompute::Init()
{
}

void WebGPUCompute::CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry> &bindGroupLayoutEntries)
{
    // Create a bind group layout using a vector of layout entries
    auto entries = GetWebGPUBindGroupLayoutEntriesPtr(bindGroupLayoutEntries, false);
    auto bindGroupLayoutEntryCount = (uint32_t)entries.size();
    assert(bindGroupLayoutEntryCount > 0);
	wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc;
	bindGroupLayoutDesc.entryCount = bindGroupLayoutEntryCount;
	bindGroupLayoutDesc.entries = entries.data();
    bindGroupLayoutDesc.label = "MainBindGroupLayout";
    m_bindGroupLayout = WebGPU::GetDevice().createBindGroupLayout(bindGroupLayoutDesc);

    std::vector<wgpu::BindGroupEntry> bindings;
    bindings.resize(bindGroupLayoutEntryCount);

    // Buffers
    for (size_t i = 0; i < bindings.size(); i++)
    {
        assert(bindGroupLayoutEntries[i].buffer.type != RenderSys::BufferBindingType::Undefined);
        auto it = m_buffersAccessibleToShader.find(bindGroupLayoutEntries[i].binding);
        wgpu::Buffer buffer = nullptr;
        if (it != m_buffersAccessibleToShader.end())
        {
            buffer = it->second;
        }
        else
        {
            auto outBufIter = m_shaderOutputBuffers.find(bindGroupLayoutEntries[i].binding);
            assert(outBufIter != m_shaderOutputBuffers.end());
            buffer = outBufIter->second->buffer;
        }

        bindings[i].binding = bindGroupLayoutEntries[i].binding;
        bindings[i].buffer = buffer;
        bindings[i].offset = 0;
        bindings[i].size = buffer.getSize();
    }

    wgpu::BindGroupDescriptor bindGroupDesc;
	bindGroupDesc.layout = m_bindGroupLayout;
	bindGroupDesc.entryCount = (uint32_t)bindings.size();
	bindGroupDesc.entries = (WGPUBindGroupEntry*)bindings.data();
    m_bindGroup = WebGPU::GetDevice().createBindGroup(bindGroupDesc);

    std::cout << "Compute bind group: " << m_bindGroup << std::endl;
}

void WebGPUCompute::CreateShaders(RenderSys::Shader& shader)
{
    std::cout << "Creating shader module..." << std::endl;

    wgpu::ShaderModuleDescriptor shaderDesc;
#ifdef WEBGPU_BACKEND_WGPU
    shaderDesc.hintCount = 0;
    shaderDesc.hints = nullptr;
#endif

    wgpu::ShaderModuleWGSLDescriptor shaderCodeDesc;
    // Set the chained struct's header
    shaderCodeDesc.chain.next = nullptr;
    shaderCodeDesc.chain.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
    shaderCodeDesc.code = shader.shaderSrc.c_str();
    // Connect the chain
    shaderDesc.nextInChain = &shaderCodeDesc.chain;
    m_shaderModule = WebGPU::GetDevice().createShaderModule(shaderDesc);

    std::cout << "Shader module: " << m_shaderModule << std::endl;
}

void WebGPUCompute::CreatePipeline()
{
    std::cout << "Creating compute pipeline..." << std::endl;
    assert(m_bindGroupLayout);
    assert(m_shaderModule);

    // Create compute pipeline layout
    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindGroupLayoutCount = 1;
    pipelineLayoutDesc.bindGroupLayouts = (WGPUBindGroupLayout*)&m_bindGroupLayout;
    auto pipelineLayout = WebGPU::GetDevice().createPipelineLayout(pipelineLayoutDesc);

    // Create compute pipeline
    wgpu::ComputePipelineDescriptor computePipelineDesc;
    computePipelineDesc.compute.constantCount = 0;
    computePipelineDesc.compute.constants = nullptr;
    computePipelineDesc.compute.entryPoint = "computeStuff";
    computePipelineDesc.compute.module = m_shaderModule;
    computePipelineDesc.layout = pipelineLayout;
    m_pipeline = WebGPU::GetDevice().createComputePipeline(computePipelineDesc);

    std::cout << "Compute pipeline: " << m_pipeline << std::endl;
}

void WebGPUCompute::CreateBuffer(uint32_t binding, uint32_t bufferLength, RenderSys::ComputeBuf::BufferType type)
{
    switch (type)
    {
        case RenderSys::ComputeBuf::BufferType::Input:
        {
            wgpu::BufferDescriptor inputBufferDesc;
            inputBufferDesc.mappedAtCreation = false;
            inputBufferDesc.size = bufferLength;
            inputBufferDesc.label = ("Buffer bound to binding " + std::to_string(binding)).c_str();
            std::cout << "Creating input buffer..." << std::endl;
            inputBufferDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
            m_buffersAccessibleToShader[binding] = WebGPU::GetDevice().createBuffer(inputBufferDesc);
            std::cout << "input buffer: " << m_buffersAccessibleToShader.find(binding)->second << std::endl;
            break;
        }
        case RenderSys::ComputeBuf::BufferType::Output:
        {
            wgpu::BufferDescriptor outputBufferDesc;
            outputBufferDesc.mappedAtCreation = false;
            outputBufferDesc.size = bufferLength;
            outputBufferDesc.label = ("Buffer bound to binding " + std::to_string(binding)).c_str();
            std::cout << "Creating output buffer..." << std::endl;
            outputBufferDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc;
            auto outBuf = WebGPU::GetDevice().createBuffer(outputBufferDesc);
            std::cout << "output buffer: " << outBuf << std::endl;
            
            wgpu::BufferDescriptor mapbufferDesc;
            mapbufferDesc.mappedAtCreation = false;
            mapbufferDesc.size = bufferLength;
            mapbufferDesc.label = ("Map Buffer bound to binding " + std::to_string(binding)).c_str();
            std::cout << "Creating map buffer..." << std::endl;
            mapbufferDesc.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
            auto mapBuf = WebGPU::GetDevice().createBuffer(mapbufferDesc);
            std::cout << "map buffer: " << mapBuf << std::endl;

            auto& [Iter, inserted] = m_shaderOutputBuffers.insert({binding, std::make_shared<MappedBuffer>()});
            assert(inserted == true);
            Iter->second->buffer = outBuf;
            Iter->second->mapBuffer = mapBuf;
            Iter->second->resultReady = false;
            Iter->second->mappedData.resize(bufferLength);
            break;
        }
        case RenderSys::ComputeBuf::BufferType::Uniform:
        {
            wgpu::BufferDescriptor uniformbufferDesc;
            uniformbufferDesc.mappedAtCreation = false;
            uniformbufferDesc.size = bufferLength;
            uniformbufferDesc.label = ("Uniform Buffer bound to binding " + std::to_string(binding)).c_str();
            std::cout << "Creating uniform buffer..." << std::endl;
            uniformbufferDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
            m_buffersAccessibleToShader[binding] = WebGPU::GetDevice().createBuffer(uniformbufferDesc);
            std::cout << "uniform buffer: " << m_buffersAccessibleToShader.find(binding)->second << std::endl;
            break;
        }
    }
}

void WebGPUCompute::SetBufferData(uint32_t binding, const void *bufferData, uint32_t bufferLength)
{
    auto it = m_buffersAccessibleToShader.find(binding);
    assert(it != m_buffersAccessibleToShader.end());
    WebGPU::GetQueue().writeBuffer(it->second, 0, bufferData, bufferLength);
}

void WebGPUCompute::BeginComputePass()
{
    wgpu::CommandEncoderDescriptor commandEncoderDesc;
    commandEncoderDesc.label = "Compute Command Encoder";
    m_commandEncoder = WebGPU::GetDevice().createCommandEncoder(commandEncoderDesc);
    

    wgpu::ComputePassDescriptor computePassDesc;
    //computePassDesc.timestampWriteCount = 0;
    computePassDesc.timestampWrites = nullptr;

    m_computePass = m_commandEncoder.beginComputePass(computePassDesc);
}

void WebGPUCompute::Compute(const uint32_t workgroupCountX, const uint32_t workgroupCountY)
{
    m_computePass.setPipeline(m_pipeline);
    m_computePass.setBindGroup(0, m_bindGroup, 0, nullptr);
	m_computePass.dispatchWorkgroups(workgroupCountX, workgroupCountY, 1);
}

void WebGPUCompute::BufferMapCallback(WGPUMapAsyncStatus status, char const * message, uint32_t binding)
{
    if (status == wgpu::MapAsyncStatus::Success)
    {
        auto it = m_shaderOutputBuffers.find(binding);
        assert(it != m_shaderOutputBuffers.end());
        auto& mappedBuffer = it->second;

        const uint8_t *output = static_cast<const uint8_t *>(mappedBuffer->mapBuffer.getConstMappedRange(0, mappedBuffer->mappedData.size()));
        if (output)
        {
            memcpy(mappedBuffer->mappedData.data(), output, mappedBuffer->mappedData.size());
        }
        else
        {
            std::cout << "null output" << std::endl;
            // assert(false);
        }
        mappedBuffer->mapBuffer.unmap();
        mappedBuffer->resultReady.store(true);
    }
    else
    {
        std::cout << "Failed to map buffer" << std::endl;
        assert(false);
    }
}

void WebGPUCompute::EndComputePass()
{
    m_computePass.end();

    // Have to copy buffers before encoder.finish
    // Copy the memory from the output buffer that lies in the storage part of the
    // memory to the map buffer, which is in the "mappable" part of the memory.
    for (auto &outBufPair : m_shaderOutputBuffers)
    {
        auto& outBuf = outBufPair.second;
        m_commandEncoder.copyBufferToBuffer(outBuf->buffer, 0, outBuf->mapBuffer, 0, outBuf->mapBuffer.getSize());
    }

    wgpu::CommandBufferDescriptor cmdBufferDescriptor;
    cmdBufferDescriptor.label = "Compute Command Buffer";
    wgpu::CommandBuffer commands = m_commandEncoder.finish(cmdBufferDescriptor);
    WebGPU::GetQueue().submit(commands);
}

std::vector<uint8_t>& WebGPUCompute::GetMappedResult(uint32_t binding)
{
    // Copy output
    auto& found = m_shaderOutputBuffers.find(binding);
    assert(found != m_shaderOutputBuffers.end());
    auto& mapperBufferStruct = found->second;
    mapperBufferStruct->resultReady.store(false);
    wgpu::BufferMapCallbackInfo2 callbackInfo;
    callbackInfo.callback = [](WGPUMapAsyncStatus status, char const * message, void* userdata1, void* userdata2) 
    {
        WebGPUCompute* compute = static_cast<WebGPUCompute*>(userdata1);
        uint32_t* binding = static_cast<uint32_t*>(userdata2);
        compute->BufferMapCallback(status, message, *binding);
    };
    callbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    callbackInfo.userdata1 = static_cast<void*>(this);
    uint32_t* bindingPtr = const_cast<uint32_t*>(&found->first);
    callbackInfo.userdata2 = static_cast<void*>(bindingPtr);
    wgpu::Future handle = mapperBufferStruct->mapBuffer.mapAsync2(
                                        wgpu::MapMode::Read, 0, mapperBufferStruct->mapBuffer.getSize(), callbackInfo);

	while (!mapperBufferStruct->resultReady.load()) 
    {
		// Checks for ongoing asynchronous operations and call their callbacks if needed
#ifdef WEBGPU_BACKEND_WGPU
        queue.submit(0, nullptr);
#else
        WebGPU::GetInstance().processEvents();
        WebGPU::GetDevice().tick();
#endif
	}

    m_commandEncoder.release();
    assert(mapperBufferStruct->resultReady.load() == true);        
    return mapperBufferStruct->mappedData;
}

void WebGPUCompute::Destroy()
{
    for (auto& [binding, buffer] : m_buffersAccessibleToShader)
    {
        buffer.destroy();
        buffer.release();
    }
    
	
    for (auto& [binding, mappedBuffer] : m_shaderOutputBuffers)
    {
        mappedBuffer->buffer.destroy();
	    mappedBuffer->buffer.release();
        mappedBuffer->mapBuffer.destroy();
	    mappedBuffer->mapBuffer.release();
    }

    m_buffersAccessibleToShader.clear();
    m_shaderOutputBuffers.clear();
}

}

