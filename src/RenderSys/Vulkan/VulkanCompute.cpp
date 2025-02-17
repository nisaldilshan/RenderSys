#include "VulkanCompute.h"

#include "VulkanRendererUtils.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace GraphicsAPI
{

VulkanCompute::~VulkanCompute()
{
    Destroy();
}

void VulkanCompute::Init()
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(Vulkan::GetPhysicalDevice(), &deviceProperties);
    VkPhysicalDeviceLimits deviceLimits = deviceProperties.limits;
	std::cout << "Max Compute Shared Memory Size: " << deviceLimits.maxComputeSharedMemorySize / 1024 << " KB" << std::endl;
    std::cout << "Compute Queue Family Index: " << Vulkan::GetQueueFamilyIndex() << std::endl;

    if (!m_vma)
    {
        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.physicalDevice = Vulkan::GetPhysicalDevice();
        allocatorInfo.device = Vulkan::GetDevice();
        allocatorInfo.instance = Vulkan::GetInstance();
        if (vmaCreateAllocator(&allocatorInfo, &m_vma) != VK_SUCCESS) {
            std::cout << "error: could not init VMA" << std::endl;
        }
    }
}

void VulkanCompute::CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry> &bindGroupLayoutEntries)
{
    assert(bindGroupLayoutEntries.size() >= 1);
    assert(!m_bindGroupLayout && !m_bindGroupPool && !m_bindGroup);

    std::unordered_map<VkDescriptorType, uint32_t> descriptorTypeCountMap;

    for (const auto &bindGroupLayoutEntry : bindGroupLayoutEntries)
    {
        auto vkBinding = GetVulkanBindGroupLayoutEntry(bindGroupLayoutEntry);
        m_bindGroupBindings.push_back(vkBinding);

        auto mapIter = descriptorTypeCountMap.find(vkBinding.descriptorType);
        if (mapIter != descriptorTypeCountMap.end())
        {
            mapIter->second++;
        }
        else
        {
            descriptorTypeCountMap.insert({vkBinding.descriptorType, 1});
        }
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = m_bindGroupBindings.size();
    layoutInfo.pBindings = m_bindGroupBindings.data();

    if (vkCreateDescriptorSetLayout(Vulkan::GetDevice(), &layoutInfo, nullptr, &m_bindGroupLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(descriptorTypeCountMap.size());
    for (const auto& [type, size] : descriptorTypeCountMap) {
        poolSizes.emplace_back(VkDescriptorPoolSize{type, size}); 
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(Vulkan::GetDevice(), &poolInfo, nullptr, &m_bindGroupPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_bindGroupPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_bindGroupLayout;

    if (vkAllocateDescriptorSets(Vulkan::GetDevice(), &allocInfo, &m_bindGroup) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
}

void VulkanCompute::CreateShaders(RenderSys::Shader &shader)
{
    assert(shader.type == RenderSys::ShaderType::SPIRV);
    std::cout << "Creating shader module..." << std::endl;
    std::vector<uint32_t> compiledShader;
    auto shaderMapIter = m_shaderMap.find(shader.GetName());
    if (shaderMapIter == m_shaderMap.end())
    {
        compiledShader = RenderSys::ShaderUtils::compile_file(shader.GetName(), shader);
        assert(compiledShader.size() > 0);
        m_shaderMap.emplace(shader.GetName(), compiledShader);
    }
    else
    {
        compiledShader = shaderMapIter->second;
    }

    VkShaderModuleCreateInfo shaderCreateInfo{};
    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderCreateInfo.codeSize = sizeof(uint32_t) * compiledShader.size();
    shaderCreateInfo.pCode = compiledShader.data();

    VkShaderModule shaderModule = 0;
    if (vkCreateShaderModule(Vulkan::GetDevice(), &shaderCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::cout << "could not load shader" << std::endl;
        return;
    }
    if (shaderModule == VK_NULL_HANDLE) {
        std::cout << "error: could not load shaders" << std::endl;
        return;
    }

    std::cout << "Created Shader module, Name:" << shader.GetName() << ", Ptr:" << shaderModule << std::endl;

    assert(shader.stage == RenderSys::ShaderStage::Compute);
    VkShaderStageFlagBits shaderStageBits = VK_SHADER_STAGE_COMPUTE_BIT;
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = shaderStageBits;
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName = "main";

    m_shaderStageInfos.push_back(shaderStageInfo);
}

void VulkanCompute::CreatePipeline()
{
    std::cout << "Creating compute pipeline..." << std::endl;

    if (!m_pipelineLayout)
    {
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        assert(m_bindGroupLayout != VK_NULL_HANDLE);
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts = &m_bindGroupLayout;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;

        if (vkCreatePipelineLayout(Vulkan::GetDevice(), &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
            std::cout << "error: could not create pipeline layout" << std::endl;
        }        
    }

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = m_pipelineLayout;
    assert(m_shaderStageInfos.size() == 1);
    pipelineInfo.stage = m_shaderStageInfos[0];

    if (vkCreateComputePipelines(Vulkan::GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline!");
    }

    std::cout << "Compute pipeline: " << m_pipeline << std::endl;
}

void VulkanCompute::CreateBuffer(uint32_t binding, uint32_t bufferLength, RenderSys::ComputeBuf::BufferType type)
{
    switch (type)
    {
        case RenderSys::ComputeBuf::BufferType::Input:
        {
            std::cout << "Creating input buffer..." << std::endl;
            VkBufferCreateInfo inputBufferDesc;
            inputBufferDesc.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            inputBufferDesc.pNext = nullptr;
            inputBufferDesc.flags = 0;
            inputBufferDesc.size = bufferLength;
            inputBufferDesc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            inputBufferDesc.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            auto queueFamilyIndex = Vulkan::GetQueueFamilyIndex();
            inputBufferDesc.queueFamilyIndexCount = 1;
            inputBufferDesc.pQueueFamilyIndices = &queueFamilyIndex;

            VmaAllocationCreateInfo vmaAllocInfo{};
            vmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

            VkBuffer buffer = VK_NULL_HANDLE;
            VmaAllocation bufferMemory = VK_NULL_HANDLE;
            auto res = vmaCreateBuffer(m_vma, &inputBufferDesc, &vmaAllocInfo, &buffer, &bufferMemory, nullptr);
            if (res != VK_SUCCESS) {
                std::cout << "vkCreateBuffer() failed!" << std::endl;
                return;
            }

            m_buffersAccessibleToShader[binding] = std::make_pair(buffer, bufferMemory);
            std::cout << "input buffer: " << m_buffersAccessibleToShader.find(binding)->second.first << std::endl;
            break;
        }
        case RenderSys::ComputeBuf::BufferType::Output:
        {
            VkBuffer outputBuffer = VK_NULL_HANDLE;
            VmaAllocation outputBufferMemory = VK_NULL_HANDLE;
            VkBuffer mapBuffer = VK_NULL_HANDLE;
            VmaAllocation mapBufferMemory = VK_NULL_HANDLE;
            {
                VkBufferCreateInfo outputBufferDesc;
                outputBufferDesc.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                outputBufferDesc.pNext = nullptr;
                outputBufferDesc.flags = 0;
                outputBufferDesc.size = bufferLength;
                std::cout << "Creating output buffer..." << std::endl;
                outputBufferDesc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                outputBufferDesc.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                VmaAllocationCreateInfo vmaAllocInfo{};
                vmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    
                auto res = vmaCreateBuffer(m_vma, &outputBufferDesc, &vmaAllocInfo, &outputBuffer, &outputBufferMemory, nullptr);
                if (res != VK_SUCCESS) {
                    std::cout << "vkCreateBuffer() failed!" << std::endl;
                    return;
                }
            }
            {
                VkBufferCreateInfo mapbufferDesc;
                mapbufferDesc.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                mapbufferDesc.pNext = nullptr;
                mapbufferDesc.flags = 0;
                mapbufferDesc.size = bufferLength;
                std::cout << "Creating map buffer..." << std::endl;
                mapbufferDesc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                mapbufferDesc.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                VmaAllocationCreateInfo vmaAllocInfo{};
                vmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    
                auto res = vmaCreateBuffer(m_vma, &mapbufferDesc, &vmaAllocInfo, &mapBuffer, &mapBufferMemory, nullptr);
                if (res != VK_SUCCESS) {
                    std::cout << "vkCreateBuffer() failed!" << std::endl;
                    return;
                }
            }

            auto& [Iter, inserted] = m_shaderOutputBuffers.insert({binding, std::make_shared<MappedBuffer>()});
            assert(inserted == true);
            Iter->second->buffer = outputBuffer;
            Iter->second->bufferMemory = outputBufferMemory;
            Iter->second->mapBuffer = mapBuffer;
            Iter->second->mapBufferMemory = mapBufferMemory;
            Iter->second->resultReady = false;
            Iter->second->mappedData.resize(bufferLength);
            break;
        }
        case RenderSys::ComputeBuf::BufferType::Uniform:
        {
            VkBufferCreateInfo uniformbufferDesc;
            uniformbufferDesc.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            uniformbufferDesc.size = bufferLength;
            std::cout << "Creating uniform buffer..." << std::endl;
            uniformbufferDesc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            uniformbufferDesc.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            VmaAllocationCreateInfo vmaAllocInfo{};
            vmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

            VkBuffer buffer = VK_NULL_HANDLE;
            VmaAllocation bufferMemory = VK_NULL_HANDLE;
            auto res = vmaCreateBuffer(m_vma, &uniformbufferDesc, &vmaAllocInfo, &buffer, &bufferMemory, nullptr);
            if (res != VK_SUCCESS) {
                std::cout << "vkCreateBuffer() failed!" << std::endl;
                return;
            }
            m_buffersAccessibleToShader[binding] = std::make_pair(buffer, bufferMemory);
            std::cout << "uniform buffer: " << m_buffersAccessibleToShader.find(binding)->second.first << std::endl;
            break;
        }
    }
}

void VulkanCompute::SetBufferData(uint32_t binding, const void *bufferData, uint32_t bufferLength)
{
    //vkUpdateDescriptorSets(Vulkan::GetDevice(), 1, &descriptorWrite, 0, nullptr);
}

void VulkanCompute::BeginComputePass()
{
    // TODO: move commandpool/commandbuffer creation to constructor
    if (!m_commandPool)
    {
        auto queueFamilyIndices = Vulkan::FindQueueFamilies();
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        auto err = vkCreateCommandPool(Vulkan::GetDevice(), &poolInfo, nullptr, &m_commandPool);
        Vulkan::check_vk_result(err);
    }
    
    if (!m_commandBuffer)
    {
        VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
        cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocateInfo.commandPool = m_commandPool;
        cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufAllocateInfo.commandBufferCount = 1;
        auto err = vkAllocateCommandBuffers(Vulkan::GetDevice(), &cmdBufAllocateInfo, &m_commandBuffer);
        Vulkan::check_vk_result(err);
    }
    else
    {
        auto err = vkResetCommandBuffer(m_commandBuffer, 0);
        Vulkan::check_vk_result(err);
    }

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    auto err = vkBeginCommandBuffer(m_commandBuffer, &begin_info);
    Vulkan::check_vk_result(err);

    // VkRenderPassBeginInfo rpInfo{};
    // rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    // // rpInfo.renderPass = m_renderpass;
    // // rpInfo.renderArea.offset.x = 0;
    // // rpInfo.renderArea.offset.y = 0;
    // // rpInfo.renderArea.extent = { m_width, m_height };
    // // rpInfo.framebuffer = m_frameBuffer;
    // // rpInfo.clearValueCount = 2;
    // // rpInfo.pClearValues = clearValues;

    // vkCmdBeginQuery(m_commandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCompute::Compute(const uint32_t workgroupCountX, const uint32_t workgroupCountY)
{
    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
    assert(m_bindGroup != VK_NULL_HANDLE);
    vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_bindGroup, 0, nullptr);
    vkCmdDispatch(m_commandBuffer, 10, 10, 1);
}

void VulkanCompute::EndComputePass()
{
    //vkCmdEndRenderPass(m_commandBuffer);
    
    auto err = vkEndCommandBuffer(m_commandBuffer);
    Vulkan::check_vk_result(err);

    VkSubmitInfo end_info{};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &m_commandBuffer;
    GraphicsAPI::Vulkan::QueueSubmit(end_info);
}

std::vector<uint8_t>& VulkanCompute::GetMappedResult(uint32_t binding)
{
    auto& found = m_shaderOutputBuffers.find(binding);
    assert(found != m_shaderOutputBuffers.end());
    auto& mappedBufferStruct = found->second;
    
    // while (!mappedBufferStruct->resultReady.load())
    // {

    // }

    return mappedBufferStruct->mappedData;
}

void VulkanCompute::Destroy()
{
    for (auto& [binding, bufferPair] : m_buffersAccessibleToShader)
    {
        vmaDestroyBuffer(m_vma, bufferPair.first, bufferPair.second);
    }
	
    for (auto& [binding, mappedBufferStruct] : m_shaderOutputBuffers)
    {
        vmaDestroyBuffer(m_vma, mappedBufferStruct->buffer, mappedBufferStruct->bufferMemory);
        vmaDestroyBuffer(m_vma, mappedBufferStruct->mapBuffer, mappedBufferStruct->mapBufferMemory);
    }

    m_buffersAccessibleToShader.clear();
    m_shaderOutputBuffers.clear();

    // Destroy VMA instance
    vmaDestroyAllocator(m_vma);
}

}

