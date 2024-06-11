cmake_minimum_required (VERSION 3.22.0)
project (RenderSys)

find_package(walnut REQUIRED)
message(STATUS "RENDERER: " ${RENDERER})

if(RENDERER STREQUAL "Vulkan")
    add_compile_definitions(RENDERER_BACKEND=2)
    add_library (RenderSys2D STATIC
                    src/Renderer2D.cpp
                    src/Vulkan/VulkanRenderer2D.cpp
    )
    target_link_libraries(RenderSys2D PRIVATE 
                            walnut::walnut
    )
    add_library (RenderSys3D STATIC
                    src/Renderer3D.cpp
                    src/Vulkan/VulkanRenderer3D.cpp
                    src/Camera.cpp
                )
    target_link_libraries(RenderSys3D PRIVATE 
                            walnut::walnut)
else()
    add_compile_definitions(RENDERER_BACKEND=3)
    add_library (RenderSys2D STATIC
                    src/Renderer2D.cpp
                    src/WebGPU/WebGPURenderer2D.cpp
    )
    target_link_libraries(RenderSys2D PRIVATE 
                            walnut::walnut
    )
    add_library (RenderSys3D STATIC
                    src/Renderer3D.cpp
                    src/WebGPU/WebGPURenderer3D.cpp
                    src/Camera.cpp
                )
    target_link_libraries(RenderSys3D PRIVATE 
                            walnut::walnut)

    add_library (ComputeSys STATIC
                    src/Compute.cpp
                    src/WebGPU/WebGPUCompute.cpp
                )
    target_link_libraries(ComputeSys PRIVATE walnut::walnut)
endif()






add_subdirectory(example)