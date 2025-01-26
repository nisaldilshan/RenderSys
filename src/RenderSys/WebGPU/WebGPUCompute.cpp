#include "WebGPUCompute.h"
#include "WebGPURendererUtils.h"

namespace GraphicsAPI
{

wgpu::Buffer m_mapBuffer = nullptr;
std::vector<uint8_t> m_mapBufferMappedData;

WebGPUCompute::~WebGPUCompute()
{
    for (auto& [name, buffer] : m_buffersAccessibleToShader)
    {
        buffer.destroy();
        buffer.release();
    }
    m_buffersAccessibleToShader.clear();
	
    if (m_mapBuffer)
    {
        m_mapBuffer.destroy();
	    m_mapBuffer.release();
        m_mapBuffer = nullptr;
    }
}

void WebGPUCompute::CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries)
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
        auto it = m_buffersAccessibleToShader.find(bindGroupLayoutEntries[i].buffer.bufferName);
        assert(it != m_buffersAccessibleToShader.end());

        bindings[i].binding = bindGroupLayoutEntries[i].binding;
        bindings[i].buffer = it->second;
        bindings[i].offset = 0;
        bindings[i].size = it->second.getSize();
    }

    wgpu::BindGroupDescriptor bindGroupDesc;
	bindGroupDesc.layout = m_bindGroupLayout;
	bindGroupDesc.entryCount = (uint32_t)bindings.size();
	bindGroupDesc.entries = (WGPUBindGroupEntry*)bindings.data();
    m_bindGroup = WebGPU::GetDevice().createBindGroup(bindGroupDesc);

    std::cout << "Compute bind group: " << m_bindGroup << std::endl;
}

void WebGPUCompute::CreateShaders(const char *shaderSource)
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
    shaderCodeDesc.code = shaderSource;
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

void WebGPUCompute::CreateBuffer(uint32_t bufferLength, RenderSys::ComputeBuf::BufferType type, const std::string& name)
{
    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.mappedAtCreation = false;
    bufferDesc.size = bufferLength;
    bufferDesc.label = name.c_str();

    switch (type)
    {
    case RenderSys::ComputeBuf::BufferType::Input:
        std::cout << "Creating input buffer..." << std::endl;
        bufferDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
        m_buffersAccessibleToShader.emplace(name, WebGPU::GetDevice().createBuffer(bufferDesc));
        std::cout << "input buffer: " << m_buffersAccessibleToShader.find(name)->second << std::endl;
        break;
    case RenderSys::ComputeBuf::BufferType::Output:
        std::cout << "Creating output buffer..." << std::endl;
        bufferDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc;
        m_buffersAccessibleToShader.emplace(name, WebGPU::GetDevice().createBuffer(bufferDesc));
        std::cout << "output buffer: " << m_buffersAccessibleToShader.find(name)->second << std::endl;
        break;
    case RenderSys::ComputeBuf::BufferType::Map:
        std::cout << "Creating map buffer..." << std::endl;
        bufferDesc.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
        m_mapBuffer = WebGPU::GetDevice().createBuffer(bufferDesc);
        m_mapBufferMappedData.resize(bufferLength);
        std::cout << "map buffer: " << m_mapBuffer << std::endl;
        break;
    case RenderSys::ComputeBuf::BufferType::Uniform:
        std::cout << "Creating map buffer..." << std::endl;
        bufferDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
        bufferDesc.mappedAtCreation = false;
        m_buffersAccessibleToShader.emplace(name, WebGPU::GetDevice().createBuffer(bufferDesc));
        std::cout << "map buffer: " << m_mapBuffer << std::endl;
        break;
    }
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

void WebGPUCompute::SetBufferData(const void *bufferData, uint32_t bufferLength, const std::string &name)
{
    auto it = m_buffersAccessibleToShader.find(name);
    assert(it != m_buffersAccessibleToShader.end());
    WebGPU::GetQueue().writeBuffer(it->second, 0, bufferData, bufferLength);
}

bool g_resultReady = false;

void WebGPUCompute::Compute(const uint32_t workgroupCountX, const uint32_t workgroupCountY)
{
    g_resultReady = false;
    m_computePass.setPipeline(m_pipeline);
    m_computePass.setBindGroup(0, m_bindGroup, 0, nullptr);
	m_computePass.dispatchWorkgroups(workgroupCountX, workgroupCountY, 1);
}

void dummyFunc(WGPUMapAsyncStatus status, char const * message, void* userdata1, void* userdata2) // userdata1 = m_mapBuffer and userdata2 = m_mapBufferMappedData
{
    if (status == wgpu::MapAsyncStatus::Success)
    {
        const uint8_t *output = static_cast<const uint8_t *>(m_mapBuffer.getConstMappedRange(0, m_mapBufferMappedData.size()));
        if (output)
        {
            memcpy(m_mapBufferMappedData.data(), output, m_mapBufferMappedData.size());
        }
        else
        {
            std::cout << "null output" << std::endl;
            // assert(false);
        }
        m_mapBuffer.unmap();
    }
    else
    {
        std::cout << "Failed to map buffer" << std::endl;
        assert(false);
    }
    g_resultReady = true;
}

void WebGPUCompute::EndComputePass()
{
    m_computePass.end();

    // Have to copy buffers before encoder.finish
    const auto sizeOfMapBuffer = m_mapBuffer.getSize();
    // Copy the memory from the output buffer that lies in the storage part of the
    // memory to the map buffer, which is in the "mappable" part of the memory.
    auto it = m_buffersAccessibleToShader.find("OUTPUT_BUFFER");
    assert(it != m_buffersAccessibleToShader.end());
	m_commandEncoder.copyBufferToBuffer(it->second, 0, m_mapBuffer, 0, sizeOfMapBuffer);

    wgpu::CommandBufferDescriptor cmdBufferDescriptor;
    cmdBufferDescriptor.label = "Compute Command Buffer";
    wgpu::CommandBuffer commands = m_commandEncoder.finish(cmdBufferDescriptor);
    WebGPU::GetQueue().submit(commands);

    // Copy output
    wgpu::BufferMapCallbackInfo2 callbackInfo;
    callbackInfo.callback = &dummyFunc;
    callbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    callbackInfo.userdata1 = static_cast<void*>(&m_mapBuffer);
    callbackInfo.userdata2 = static_cast<void*>(&m_mapBufferMappedData);

    wgpu::Future handle = m_mapBuffer.mapAsync2(wgpu::MapMode::Read, 0, sizeOfMapBuffer, callbackInfo);

	while (!g_resultReady) {
		// Checks for ongoing asynchronous operations and call their callbacks if needed
#ifdef WEBGPU_BACKEND_WGPU
        queue.submit(0, nullptr);
#else
        WebGPU::GetInstance().processEvents();
        WebGPU::GetDevice().tick();
#endif
	}

    m_commandEncoder.release();
}

std::vector<uint8_t>& WebGPUCompute::GetMappedResult()
{
    if (!g_resultReady)
        assert(false);
        
    return m_mapBufferMappedData;
}

}

