if(RENDERER STREQUAL "Vulkan")

add_library (RenderSysCommon STATIC
    ../Vulkan/VulkanMemAlloc.cpp
)

target_link_libraries(RenderSysCommon PRIVATE 
                            vulkan-memory-allocator::vulkan-memory-allocator
                            walnut::walnut)
else()
add_library (RenderSysCommon STATIC
    ../WebGPU/WebGPUBuffer.cpp
)
target_include_directories(RenderSysCommon PRIVATE ../..)
target_link_libraries(RenderSysCommon PRIVATE 
                            glm::glm
                            walnut::walnut)
endif()

install(TARGETS RenderSysCommon
FILE_SET renderSysFileSet
COMPONENT RenderSysCommon
LIBRARY DESTINATION lib
PRIVATE_HEADER DESTINATION include)
