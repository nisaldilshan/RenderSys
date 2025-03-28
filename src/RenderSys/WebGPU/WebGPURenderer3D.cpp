#include "WebGPURenderer3D.h"
#include "WebGPURendererUtils.h"
#include "WebGPUTexture.h"

namespace GraphicsAPI
{

const wgpu::TextureFormat g_depthTextureFormat = wgpu::TextureFormat::Depth24Plus;

bool WebGPURenderer3D::Init()
{
    return true;
}

void WebGPURenderer3D::CreateImageToRender(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
    CreateDefaultTextureSampler();

    wgpu::TextureDescriptor tex_desc = {};
    tex_desc.label = "Renderer Final Texture";
    tex_desc.dimension = WGPUTextureDimension_2D;
    tex_desc.size.width = m_width;
    tex_desc.size.height = m_height;
    tex_desc.size.depthOrArrayLayers = 1;
    tex_desc.sampleCount = 1;
    tex_desc.format = WGPUTextureFormat_BGRA8Unorm;
    tex_desc.mipLevelCount = 1;
    tex_desc.usage = WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment;
    //##
    tex_desc.viewFormatCount = 1;
    wgpu::TextureFormat tf = WebGPU::GetSwapChainFormat();
	tex_desc.viewFormats = (WGPUTextureFormat *)const_cast<wgpu::TextureFormat *>(&tf);
    //##
    wgpu::Texture texture = WebGPU::GetDevice().createTexture(tex_desc);

    wgpu::TextureViewDescriptor tex_view_desc = {};
    tex_view_desc.format = WGPUTextureFormat_BGRA8Unorm;
    tex_view_desc.dimension = WGPUTextureViewDimension_2D;
    tex_view_desc.baseMipLevel = 0;
    tex_view_desc.mipLevelCount = 1;
    tex_view_desc.baseArrayLayer = 0;
    tex_view_desc.arrayLayerCount = 1;
    tex_view_desc.aspect = WGPUTextureAspect_All;
    m_textureToRenderInto = texture.createView(tex_view_desc);
}

void WebGPURenderer3D::CreateDepthImage()
{
    // Create the depth texture
	wgpu::TextureDescriptor depthTextureDesc;
	depthTextureDesc.dimension = wgpu::TextureDimension::_2D;
	depthTextureDesc.format = g_depthTextureFormat;
	depthTextureDesc.mipLevelCount = 1;
	depthTextureDesc.sampleCount = 1;
	depthTextureDesc.size = {m_width, m_height, 1};
	depthTextureDesc.usage = wgpu::TextureUsage::RenderAttachment;
	depthTextureDesc.viewFormatCount = 1;
	depthTextureDesc.viewFormats = (WGPUTextureFormat*)&g_depthTextureFormat;
	m_depthTexture = WebGPU::GetDevice().createTexture(depthTextureDesc);
	std::cout << "Depth texture: " << m_depthTexture << std::endl;

	// Create the view of the depth texture manipulated by the rasterizer
	wgpu::TextureViewDescriptor depthTextureViewDesc;
	depthTextureViewDesc.aspect = wgpu::TextureAspect::DepthOnly;
	depthTextureViewDesc.baseArrayLayer = 0;
	depthTextureViewDesc.arrayLayerCount = 1;
	depthTextureViewDesc.baseMipLevel = 0;
	depthTextureViewDesc.mipLevelCount = 1;
	depthTextureViewDesc.dimension = wgpu::TextureViewDimension::_2D;
	depthTextureViewDesc.format = g_depthTextureFormat;
	m_depthTextureView = m_depthTexture.createView(depthTextureViewDesc);
	std::cout << "Depth texture view: " << m_depthTextureView << std::endl;
}

void WebGPURenderer3D::CreateShaders(RenderSys::Shader& shader)
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
    shaderCodeDesc.code = shader.GetShaderSrc().c_str();
    // Connect the chain
    shaderDesc.nextInChain = &shaderCodeDesc.chain;
    m_shaderModule = WebGPU::GetDevice().createShaderModule(shaderDesc);

    std::cout << "Shader module: " << m_shaderModule << std::endl;
}

void WebGPURenderer3D::CreatePipeline()
{
    std::cout << "Creating render pipeline..." << std::endl;

    wgpu::RenderPipelineDescriptor pipelineDesc;

    // Vertex fetch
    assert(m_vertexIndexBufferInfoMap.size() == 1);
    if (m_vertexIndexBufferInfoMap[0].m_vertexCount > 0)
    {
        pipelineDesc.vertex.bufferCount = 1;
        pipelineDesc.vertex.buffers = &m_vertexBufferLayout;
    }
    else
    {
        pipelineDesc.vertex.bufferCount = 0;
        pipelineDesc.vertex.buffers = nullptr;
    }
    // Vertex shader
    pipelineDesc.vertex.module = m_shaderModule;
	pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
	pipelineDesc.vertex.constants = nullptr;

    // Primitive assembly and rasterization
	// Each sequence of 3 vertices is considered as a triangle
	pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
	// We'll see later how to specify the order in which vertices should be
	// connected. When not specified, vertices are considered sequentially.
	pipelineDesc.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;
	// The face orientation is defined by assuming that when looking
	// from the front of the face, its corner vertices are enumerated
	// in the counter-clockwise (CCW) order.
	pipelineDesc.primitive.frontFace = wgpu::FrontFace::CCW;
	// But the face orientation does not matter much because we do not
	// cull (i.e. "hide") the faces pointing away from us (which is often
	// used for optimization).
	pipelineDesc.primitive.cullMode = wgpu::CullMode::None;

    // Fragment shader
	wgpu::FragmentState fragmentState;
	pipelineDesc.fragment = &fragmentState;
	fragmentState.module = m_shaderModule;
	fragmentState.entryPoint = "fs_main";
	fragmentState.constantCount = 0;
	fragmentState.constants = nullptr;

    // Configure blend state
	wgpu::BlendState blendState;
	// Usual alpha blending for the color:
	blendState.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
	blendState.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
	blendState.color.operation = wgpu::BlendOperation::Add;
	// We leave the target alpha untouched:
	blendState.alpha.srcFactor = wgpu::BlendFactor::Zero;
	blendState.alpha.dstFactor = wgpu::BlendFactor::One;
	blendState.alpha.operation = wgpu::BlendOperation::Add;

    wgpu::ColorTargetState colorTarget;
	colorTarget.format = WebGPU::GetSwapChainFormat();
	colorTarget.blend = &blendState;
	colorTarget.writeMask = wgpu::ColorWriteMask::All; // We could write to only some of the color channels.

	// We have only one target because our render pass has only one output color
	// attachment.
	fragmentState.targetCount = 1;
	fragmentState.targets = &colorTarget;
	
	// We setup a depth buffer state for the render pipeline
	wgpu::DepthStencilState depthStencilState = wgpu::Default;
	// Keep a fragment only if its depth is lower than the previously blended one
	depthStencilState.depthCompare = wgpu::CompareFunction::Less;
    // Each time a fragment is blended into the target, we update the value of the Z-buffer
	depthStencilState.depthWriteEnabled = true;
	// Store the format in a variable as later parts of the code depend on it
	depthStencilState.format = g_depthTextureFormat;
	// Deactivate the stencil alltogether
	depthStencilState.stencilReadMask = 0;
	depthStencilState.stencilWriteMask = 0;

    pipelineDesc.depthStencil = &depthStencilState;

    // Multi-sampling
	// Samples per pixel
	pipelineDesc.multisample.count = 1;
	// Default value for the mask, meaning "all bits on"
	pipelineDesc.multisample.mask = ~0u;
	// Default value as well (irrelevant for count = 1 anyways)
	pipelineDesc.multisample.alphaToCoverageEnabled = false;

	// Pipeline layout
    if (m_pipelineLayout)
	    pipelineDesc.layout = m_pipelineLayout;
    else
        pipelineDesc.layout = nullptr;

    m_pipeline = WebGPU::GetDevice().createRenderPipeline(pipelineDesc);
    std::cout << "Render pipeline: " << m_pipeline << std::endl;
}

void WebGPURenderer3D::CreateFrameBuffer()
{
}

uint32_t WebGPURenderer3D::CreateVertexBuffer(const RenderSys::VertexBuffer& bufferData, RenderSys::VertexBufferLayout bufferLayout)
{
    std::cout << "Creating vertex buffer..." << std::endl;
    WebGPUVertexIndexBufferInfo vertexIndexBufferInfo;
    const auto vertexBufferSize = bufferData.vertices.size() * sizeof(RenderSys::Vertex);
    vertexIndexBufferInfo.m_vertexCount = vertexBufferSize / bufferLayout.arrayStride;
    m_vertexBufferLayout = GetWebGPUVertexBufferLayout(bufferLayout);
    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.size = vertexBufferSize;
    bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
    bufferDesc.mappedAtCreation = false;
    bufferDesc.label = "Vertex Buffer";
    wgpu::Buffer vertexBuffer = WebGPU::GetDevice().createBuffer(bufferDesc);

    // Upload vertex data to the buffer
    WebGPU::GetQueue().writeBuffer(vertexBuffer, 0, bufferData.vertices.data(), bufferDesc.size);
    std::cout << "Vertex buffer: " << vertexBuffer << std::endl;
    return m_vertexIndexBufferInfoMap.insert({m_vertexIndexBufferInfoMap.size() + 1, vertexIndexBufferInfo}).first->first;
}

void WebGPURenderer3D::CreateIndexBuffer(uint32_t vertexBufferID, const std::vector<uint32_t> &bufferData)
{
    std::cout << "Creating index buffer..." << std::endl;

    m_indexCount = bufferData.size();

    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.size = bufferData.size() * sizeof(uint32_t);
    bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
    bufferDesc.label = "Index Buffer";
    m_indexBuffer = WebGPU::GetDevice().createBuffer(bufferDesc);

    // Upload index data to the buffer
    WebGPU::GetQueue().writeBuffer(m_indexBuffer, 0, bufferData.data(), bufferDesc.size);
    std::cout << "Index buffer: " << m_indexBuffer << std::endl;
}


void WebGPURenderer3D::CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries)
{
    // Create a bind group layout using a vector of layout entries
    m_mainBindGroupBindings = GetWebGPUBindGroupLayoutEntriesPtr(bindGroupLayoutEntries, true);

    auto bindGroupLayoutEntryCount = (uint32_t)m_mainBindGroupBindings.size();
	wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc;
	bindGroupLayoutDesc.entryCount = bindGroupLayoutEntryCount;
	bindGroupLayoutDesc.entries = m_mainBindGroupBindings.data();
    bindGroupLayoutDesc.label = "MainBindGroupLayout";
	m_bindGroupLayout = WebGPU::GetDevice().createBindGroupLayout(bindGroupLayoutDesc);

    if (m_bindGroupLayout)
    {
        // Create the pipeline layout
        wgpu::PipelineLayoutDescriptor pipelineLayoutDesc;
        pipelineLayoutDesc.bindGroupLayoutCount = 1;
        pipelineLayoutDesc.bindGroupLayouts = (WGPUBindGroupLayout*)&m_bindGroupLayout;
        pipelineLayoutDesc.label = "PipelineLayout";
        m_pipelineLayout = WebGPU::GetDevice().createPipelineLayout(pipelineLayoutDesc);

        assert(bindGroupLayoutEntryCount > 0);
        std::vector<wgpu::BindGroupEntry> bindings;
        bindings.resize(bindGroupLayoutEntryCount);

        for (const auto& bindGroupLayoutEntry : bindGroupLayoutEntries)
        {
            auto bindingIndex = bindGroupLayoutEntry.binding;
            bindings[bindingIndex].binding = bindingIndex;

            if (bindGroupLayoutEntry.buffer.type == RenderSys::BufferBindingType::Uniform)
            {
                bool uniformBufferFound = false;
                for (const auto& uniformBuffer : m_uniformBuffers)
                {
                    auto bindingOfUniform = uniformBuffer.first;
                    if (bindingOfUniform == bindingIndex)
                    {
                        uniformBufferFound = true;
                        const auto buffer = std::get<0>(uniformBuffer.second);
                        const auto bufferSize = std::get<1>(uniformBuffer.second);
                        bindings[bindingIndex].buffer = buffer;
                        bindings[bindingIndex].offset = 0;
                        bindings[bindingIndex].size = bufferSize;
                        break;
                    }
                }

                assert(uniformBufferFound);
            }
            else if (bindGroupLayoutEntry.texture.viewDimension == RenderSys::TextureViewDimension::_2D)
            {
                if (bindingIndex == 1) // this is base color texture
                    bindings[bindingIndex].textureView = m_textures[bindingIndex]->GetImageView(); // m_texturesAndViews[0] is the base color texture
                else // this is normal texture
                    bindings[bindingIndex].textureView = m_textures[bindingIndex]->GetImageView(); // m_texturesAndViews[0] is the normal texture
            }
            else
            {
                assert(false);
            }
        }

        if (m_defaultTextureSampler)
        {
            auto& textureSamplerBinding = bindings[bindings.size() - 1];
            textureSamplerBinding.binding = bindings.size() - 1;
            textureSamplerBinding.sampler = m_defaultTextureSampler;
        }

        // A bind group contains one or multiple bindings
        wgpu::BindGroupDescriptor bindGroupDesc;
        bindGroupDesc.layout = m_bindGroupLayout;
        // There must be as many bindings as declared in the layout!
        bindGroupDesc.entryCount = (uint32_t)bindings.size();
        bindGroupDesc.entries = bindings.data();
        m_bindGroup = WebGPU::GetDevice().createBindGroup(bindGroupDesc);
    }
}

void WebGPURenderer3D::SetClearColor(glm::vec4 clearColor)
{
    m_clearColor = wgpu::Color{clearColor.x, clearColor.y, clearColor.z, clearColor.w};
}

uint32_t WebGPURenderer3D::GetUniformStride(const uint32_t& uniformIndex, const uint32_t& sizeOfUniform)
{
    if (uniformIndex == 0)
        return 0;

    // Get device limits
    wgpu::SupportedLimits deviceSupportedLimits;
    WebGPU::GetDevice().getLimits(&deviceSupportedLimits);
    wgpu::Limits deviceLimits = deviceSupportedLimits.limits;
    
    /** Round 'value' up to the next multiplier of 'step' */
    auto ceilToNextMultiple = [](uint32_t value, uint32_t step) -> uint32_t
    {
        uint32_t divide_and_ceil = value / step + (value % step == 0 ? 0 : 1);
        return step * divide_and_ceil;
    };

    // Create uniform buffer
    // Subtility
    assert(sizeOfUniform > 0);
    uint32_t uniformStride = ceilToNextMultiple(
        (uint32_t)sizeOfUniform,
        (uint32_t)deviceLimits.minUniformBufferOffsetAlignment
    );

    return uniformStride * uniformIndex;
}

void WebGPURenderer3D::CreateUniformBuffer(uint32_t binding, uint32_t sizeOfOneUniform)
{
    // Create uniform buffer
    // The buffer will only contain 1 float with the value of uTime
    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.size = sizeOfOneUniform;
    // Make sure to flag the buffer as BufferUsage::Uniform
    bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
    bufferDesc.mappedAtCreation = false;

    // if (type == UniformBuf::UniformType::ModelViewProjection)
    // {
    //     bufferDesc.label = "ModelViewProjection";
    // }
    // else
    // {
    //     bufferDesc.label = "Lighting";
    // }

    auto buffer = WebGPU::GetDevice().createBuffer(bufferDesc);
    m_uniformBuffers.insert({binding, std::make_tuple(buffer, sizeOfOneUniform)});
}

void WebGPURenderer3D::SetUniformData(uint32_t binding, const void* bufferData)
{
    auto uniformBuffer = m_uniformBuffers.find(binding);
    if (uniformBuffer != m_uniformBuffers.end())
    {
        const auto buffer = std::get<0>(uniformBuffer->second);
        const auto bufferSize = std::get<1>(uniformBuffer->second);
        assert(bufferSize > 0);
        WebGPU::GetQueue().writeBuffer(buffer, 0, bufferData, bufferSize);
    }
    else
    {
        assert(false);
    }
}

void WebGPURenderer3D::BindResources()
{
}

void WebGPURenderer3D::Render()
{
    assert(m_vertexIndexBufferInfoMap.size() > 0);
    m_renderPass.setVertexBuffer(0, m_vertexIndexBufferInfoMap[0].m_vertexBuffer
                                , 0, m_vertexIndexBufferInfoMap[0].m_vertexCount * sizeof(RenderSys::Vertex));

    if (m_bindGroup)
    {
        bool hasDynamicOffset = false;
        for (const auto &bindGroupBinding : m_mainBindGroupBindings)
        {
            if (bindGroupBinding.buffer.hasDynamicOffset)
            {
                hasDynamicOffset = true;
            }
        }
        
        if (hasDynamicOffset)
        {
            auto& uniformBufferTuple = m_uniformBuffers[0]; // this is working because we have dynamicOffsetys only in binding 0;
            //const uint32_t dynamicOffset = GetUniformStride(uniformIndex, std::get<1>(uniformBufferTuple));
            uint32_t dynamicOffsetCount = 1; // because we have enabled dynamic offset in only one binding in the bind group
            m_renderPass.setBindGroup(0, m_bindGroup, 0, nullptr);
        }
        else
        {
            m_renderPass.setBindGroup(0, m_bindGroup, 0, nullptr);
        }
    }

    if (m_indexCount > 0)
    {
        m_renderPass.setIndexBuffer(m_indexBuffer, wgpu::IndexFormat::Uint16, 0, m_indexCount * sizeof(uint16_t));
        m_renderPass.drawIndexed(m_indexCount, 1, 0, 0, 0);
    }
    else
        m_renderPass.draw(m_vertexIndexBufferInfoMap[0].m_vertexCount, 1, 0, 0);
}

void WebGPURenderer3D::RenderIndexed()
{
    assert(m_indexBuffer);
    assert(m_vertexIndexBufferInfoMap.size() > 0);
    m_renderPass.setVertexBuffer(0, m_vertexIndexBufferInfoMap[0].m_vertexBuffer, 0, m_vertexIndexBufferInfoMap[0].m_vertexCount * sizeof(RenderSys::Vertex));
    m_renderPass.setIndexBuffer(m_indexBuffer, wgpu::IndexFormat::Uint32, 0, m_indexCount * sizeof(uint32_t));

    auto& uniformBufferTuple = m_uniformBuffers[0]; // this is working because we have dynamicOffsetys only in binding 0;
    //const uint32_t dynamicOffset = GetUniformStride(uniformIndex, std::get<1>(uniformBufferTuple));
    uint32_t dynamicOffsetCount = 0; // because we have enabled dynamic offset in only one binding in the bind group
    m_renderPass.setBindGroup(0, m_bindGroup, dynamicOffsetCount, nullptr);
    m_renderPass.drawIndexed(m_indexCount, 1, 0, 0, 0);
}

void WebGPURenderer3D::RenderMesh(const RenderSys::Mesh &mesh)
{
}

ImTextureID WebGPURenderer3D::GetDescriptorSet()
{
    return m_textureToRenderInto;
}

void WebGPURenderer3D::BeginRenderPass()
{
    if (!m_textureToRenderInto)
        std::cerr << "Cannot acquire texture to render into" << std::endl;

    wgpu::CommandEncoderDescriptor commandEncoderDesc;
    commandEncoderDesc.label = "Renderer Command Encoder";
    m_currentCommandEncoder = WebGPU::GetDevice().createCommandEncoder(commandEncoderDesc);

    wgpu::RenderPassDescriptor renderPassDesc;

    wgpu::RenderPassColorAttachment renderPassColorAttachment;
    renderPassColorAttachment.view = m_textureToRenderInto;
    renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    renderPassColorAttachment.resolveTarget = nullptr;
    renderPassColorAttachment.loadOp = wgpu::LoadOp::Clear;
    renderPassColorAttachment.storeOp = wgpu::StoreOp::Store;
    renderPassColorAttachment.clearValue = m_clearColor;
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &renderPassColorAttachment;

    // We now add a depth/stencil attachment:
    wgpu::RenderPassDepthStencilAttachment depthStencilAttachment;
    // The view of the depth texture
    depthStencilAttachment.view = m_depthTextureView;
    // The initial value of the depth buffer, meaning "far"
    depthStencilAttachment.depthClearValue = 1.0f;
    // Operation settings comparable to the color attachment
    depthStencilAttachment.depthLoadOp = wgpu::LoadOp::Clear;
    depthStencilAttachment.depthStoreOp = wgpu::StoreOp::Store;
    // we could turn off writing to the depth buffer globally here
    depthStencilAttachment.depthReadOnly = false;

    // Stencil setup, mandatory but unused
    depthStencilAttachment.stencilClearValue = 0;
#ifdef WEBGPU_BACKEND_WGPU
    depthStencilAttachment.stencilLoadOp = wgpu::LoadOp::Clear;
    depthStencilAttachment.stencilStoreOp = wgpu::StoreOp::Store;
#else
    depthStencilAttachment.stencilLoadOp = wgpu::LoadOp::Undefined;
    depthStencilAttachment.stencilStoreOp = wgpu::StoreOp::Undefined;
#endif
    depthStencilAttachment.stencilReadOnly = true;

    renderPassDesc.depthStencilAttachment = &depthStencilAttachment;
    //renderPassDesc.timestampWriteCount = 0;
    renderPassDesc.timestampWrites = nullptr;

    m_renderPass = m_currentCommandEncoder.beginRenderPass(renderPassDesc);

    // In its overall outline, drawing a triangle is as simple as this:
    // Select which render pipeline to use
    m_renderPass.setPipeline(m_pipeline);
}

void WebGPURenderer3D::EndRenderPass()
{
    m_renderPass.end();
    SubmitCommandBuffer();
}

void WebGPURenderer3D::Destroy()
{
    //m_vertexBuffer.destroy();
	//m_vertexBuffer.release();
	m_indexBuffer.destroy();
	m_indexBuffer.release();
    // Destroy the depth texture and its view
	m_depthTextureView.release();
	m_depthTexture.destroy();
	m_depthTexture.release();
}

void WebGPURenderer3D::DestroyImages()
{
}

void WebGPURenderer3D::DestroyPipeline()
{
}

void WebGPURenderer3D::DestroyBindGroup()
{
}

void WebGPURenderer3D::SubmitCommandBuffer()
{
    wgpu::CommandBufferDescriptor cmdBufferDescriptor;
    cmdBufferDescriptor.label = "Command buffer";
    wgpu::CommandBuffer commands = m_currentCommandEncoder.finish(cmdBufferDescriptor);
    WebGPU::GetQueue().submit(commands);
}

void WebGPURenderer3D::CreateTextureSamplers(const std::vector<RenderSys::TextureSampler> &samplers)
{
}

void WebGPURenderer3D::CreateTexture(uint32_t binding, const std::shared_ptr<RenderSys::Texture> texture)
{
	// wgpu::TextureDescriptor textureDesc;
	// textureDesc.dimension = wgpu::TextureDimension::_2D;
	// textureDesc.size = {(uint32_t)texDescriptor.width, (uint32_t)texDescriptor.height, 1};
	// textureDesc.format = wgpu::TextureFormat::RGBA8Unorm;;
	// textureDesc.mipLevelCount = texDescriptor.mipMapLevelCount;
	// textureDesc.sampleCount = 1;
	// textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
	// textureDesc.viewFormatCount = 0;
	// textureDesc.viewFormats = nullptr;
	// auto newTexture = WebGPU::GetDevice().createTexture(textureDesc);
	// std::cout << "texture created: " << newTexture << std::endl;

	// wgpu::TextureViewDescriptor textureViewDesc;
	// textureViewDesc.aspect = wgpu::TextureAspect::All;
	// textureViewDesc.baseArrayLayer = 0;
	// textureViewDesc.arrayLayerCount = 1;
	// textureViewDesc.baseMipLevel = 0;
	// textureViewDesc.mipLevelCount = textureDesc.mipLevelCount;
	// textureViewDesc.dimension = wgpu::TextureViewDimension::_2D;
	// textureViewDesc.format = textureDesc.format;
	// auto newTextureView = newTexture.createView(textureViewDesc);
	// std::cout << "texture view created: " << newTextureView << std::endl;

    const auto& [textureIter, inserted] = m_textures.insert(
                                                    {
                                                        binding, 
                                                        texture->GetPlatformTexture()
                                                    }
                                                );
    if (!inserted)
    {
        return;
    }

    //UploadTexture(newTexture, textureDesc, texDescriptor.data);
}

void WebGPURenderer3D::CreateModelMaterials(uint32_t modelID, const std::vector<RenderSys::Material> &materials, 
                                            const std::vector<std::shared_ptr<RenderSys::Texture>> &textures, const int maxNumOfModels)
{
}

void WebGPURenderer3D::UploadTexture(wgpu::Texture texture, wgpu::TextureDescriptor textureDesc, const void* textureData)
{
    wgpu::ImageCopyTexture destination;
    destination.texture = texture;
    destination.origin = { 0, 0, 0 }; // equivalent of the offset argument of Queue::writeBuffer
    destination.aspect = wgpu::TextureAspect::All; // only relevant for depth/Stencil textures

    // Arguments telling how the C++ side pixel memory is laid out
    wgpu::TextureDataLayout source;
    source.offset = 0;

    wgpu::Extent3D mipLevelSize = textureDesc.size;
	std::vector<uint8_t> previousLevelPixels;
    wgpu::Extent3D previousMipLevelSize;
	for (uint32_t level = 0; level < textureDesc.mipLevelCount; ++level) 
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
        WebGPU::GetQueue().writeTexture(destination, pixels.data(), pixels.size(), source, mipLevelSize);

        previousLevelPixels = std::move(pixels);
        previousMipLevelSize = mipLevelSize;
        mipLevelSize.width /= 2;
        mipLevelSize.height /= 2;
    }
}

void WebGPURenderer3D::CreateDefaultTextureSampler()
{
    if (m_defaultTextureSampler)
        return;
    
    wgpu::SamplerDescriptor samplerDesc;
    samplerDesc.addressModeU = wgpu::AddressMode::Repeat;
    samplerDesc.addressModeV = wgpu::AddressMode::Repeat;
    samplerDesc.addressModeW = wgpu::AddressMode::Repeat;
    samplerDesc.magFilter = wgpu::FilterMode::Linear;
    samplerDesc.minFilter = wgpu::FilterMode::Linear;
    samplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Linear;
    samplerDesc.lodMinClamp = 0.0f;
    samplerDesc.lodMaxClamp = 8.0f;
    samplerDesc.compare = wgpu::CompareFunction::Undefined;
    samplerDesc.maxAnisotropy = 1;
    m_defaultTextureSampler = WebGPU::GetDevice().createSampler(samplerDesc);
}

} // namespace GraphicsAPI