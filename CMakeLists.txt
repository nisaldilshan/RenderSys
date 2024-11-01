cmake_minimum_required (VERSION 3.23.0)
project (RenderSys)

find_package(walnut REQUIRED)
message(STATUS "RENDERER: " ${RENDERER})

if(RENDERER STREQUAL "Vulkan")
    include(FetchContent)
    set(VulkanMemoryAllocatorURL https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git)
    message(STATUS "Fetching: " ${VulkanMemoryAllocatorURL})
    FetchContent_Declare(
        vulkanmemoryallocator
        GIT_REPOSITORY ${VulkanMemoryAllocatorURL}
        GIT_TAG        v3.1.0
    )
    FetchContent_MakeAvailable(vulkanmemoryallocator) 
    message(STATUS "vulkanmemoryallocator sourceDir: ${vulkanmemoryallocator_SOURCE_DIR}")
    message(STATUS "vulkanmemoryallocator buildDir: ${vulkanmemoryallocator_BINARY_DIR}") 

    add_compile_definitions(RENDERER_BACKEND=2)
    find_package(shaderc REQUIRED)
    add_library (RenderSys2D STATIC
                    src/RenderSys/Renderer2D.cpp
                    src/RenderSys/Vulkan/VulkanRenderer2D.cpp
                    src/RenderSys/Shader.cpp
    )
    target_link_libraries(RenderSys2D PRIVATE GPUOpen::VulkanMemoryAllocator)
    add_library (RenderSys3D STATIC
                    src/RenderSys/Renderer3D.cpp
                    src/RenderSys/Vulkan/VulkanRenderer3D.cpp
    )
    add_library (ComputeSys STATIC
                    src/RenderSys/Compute.cpp
                    src/RenderSys/Vulkan/VulkanCompute.cpp
    )
else()
    add_compile_definitions(RENDERER_BACKEND=3)
    add_library (RenderSys2D STATIC
                    src/RenderSys/Renderer2D.cpp
                    src/RenderSys/WebGPU/WebGPURenderer2D.cpp
    )
    add_library (RenderSys3D STATIC
                    src/RenderSys/Renderer3D.cpp
                    src/RenderSys/WebGPU/WebGPURenderer3D.cpp
                    src/RenderSys/Camera.cpp
    )
    add_library (ComputeSys STATIC
                    src/RenderSys/Compute.cpp
                    src/RenderSys/WebGPU/WebGPUCompute.cpp
    )
endif()

target_sources(RenderSys2D 
                PUBLIC FILE_SET renderSysFileSet 
                TYPE HEADERS 
                BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/src
                FILES src/RenderSys/Renderer2D.h)
# target_sources(RenderSys3D 
#                 PUBLIC FILE_SET renderSysFileSet 
#                 TYPE HEADERS 
#                 BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/src
#                 FILES src/RenderSys/Renderer3D.h src/RenderSys/RenderUtil.h)
# target_sources(ComputeSys 
#                 PUBLIC FILE_SET renderSysFileSet 
#                 TYPE HEADERS 
#                 BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/src
#                 FILES src/RenderSys/Compute.h src/RenderSys/Buffer.h)

target_link_libraries(RenderSys2D PRIVATE walnut::walnut shaderc::shaderc)
target_link_libraries(RenderSys3D PRIVATE walnut::walnut)
target_link_libraries(ComputeSys PRIVATE walnut::walnut)

install(TARGETS RenderSys2D
    FILE_SET renderSysFileSet
    COMPONENT Renderer2D
    LIBRARY DESTINATION lib
    INCLUDES DESTINATION include
)

# install(TARGETS RenderSys3D
#     FILE_SET renderSysFileSet
#     COMPONENT Renderer3D
#     LIBRARY DESTINATION lib
#     INCLUDES DESTINATION include
# )

# install(TARGETS ComputeSys
#     FILE_SET renderSysFileSet
#     COMPONENT Compute
#     LIBRARY DESTINATION lib
#     PRIVATE_HEADER DESTINATION include
# )

add_subdirectory(example)