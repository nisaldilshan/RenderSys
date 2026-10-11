if(RENDERER STREQUAL "Vulkan")

add_library (RenderSysCommon STATIC
    ../Vulkan/VulkanContext.cpp
    ../Vulkan/VulkanMemAlloc.cpp
)
target_include_directories(RenderSysCommon PRIVATE ../..)
target_link_libraries(RenderSysCommon PRIVATE GPUOpen::VulkanMemoryAllocator)
target_link_libraries(RenderSysCommon PUBLIC Vulkan::Loader)
else()
add_library (RenderSysCommon STATIC
    ../WebGPU/WebGPUBuffer.cpp
)
target_include_directories(RenderSysCommon PRIVATE ../..)
target_link_libraries(RenderSysCommon PRIVATE 
                            glm::glm
                            walnut::walnut)
endif()

target_compile_definitions(RenderSysCommon PUBLIC RENDERSYS_BACKEND=${RENDERSYS_BACKEND})

install(TARGETS RenderSysCommon
FILE_SET renderSysFileSet
COMPONENT RenderSysCommon
LIBRARY DESTINATION lib
PRIVATE_HEADER DESTINATION include)
