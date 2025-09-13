include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(RENDERER STREQUAL "Vulkan")

add_library (RenderSysCommon STATIC
    ../Vulkan/VulkanMemAlloc.cpp
)

target_link_libraries(RenderSysCommon PRIVATE 
                            vulkan-memory-allocator::vulkan-memory-allocator
                            walnut::walnut)

install(TARGETS RenderSysCommon
FILE_SET renderSysFileSet
COMPONENT RenderSysCommon
LIBRARY DESTINATION lib
PRIVATE_HEADER DESTINATION include)

endif()

