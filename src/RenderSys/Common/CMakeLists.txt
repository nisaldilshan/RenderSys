if(RENDERER STREQUAL "Vulkan")

add_library (RenderSysCommon STATIC
    ../Vulkan/VulkanContext.cpp
    ../Vulkan/VulkanMemAlloc.cpp
)
target_include_directories(RenderSysCommon PRIVATE ../..)
target_link_libraries(RenderSysCommon PRIVATE GPUOpen::VulkanMemoryAllocator)
target_link_libraries(RenderSysCommon PUBLIC Vulkan::Loader)
target_sources(RenderSysCommon
                PUBLIC FILE_SET renderSysFileSet
                TYPE HEADERS
                BASE_DIRS ../..
                FILES ../Vulkan/VulkanContext.h)
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
add_library(RenderSys::Common ALIAS RenderSysCommon)

install(TARGETS RenderSysCommon
FILE_SET renderSysFileSet
COMPONENT RenderSysCommon
LIBRARY DESTINATION lib
PRIVATE_HEADER DESTINATION include)
