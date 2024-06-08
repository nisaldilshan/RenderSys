cmake_minimum_required (VERSION 3.22.0)
project (RenderSys)

message(STATUS "RENDERER: " ${RENDERER})

find_package(walnut REQUIRED)


add_library (RenderSys2D STATIC
                src/Renderer2D.cpp
                src/WebGPU/WebGPURenderer2D.cpp
            )
target_link_libraries(RenderSys2D PRIVATE walnut::walnut)


add_library (RenderSys3D STATIC
                src/Renderer3D.cpp
                src/WebGPU/WebGPURenderer3D.cpp
                src/Camera.cpp
            )
target_link_libraries(RenderSys3D PRIVATE walnut::walnut)


add_library (ComputeSys STATIC
                src/Compute.cpp
                src/WebGPU/WebGPUCompute.cpp
            )
target_link_libraries(ComputeSys PRIVATE walnut::walnut)

add_subdirectory(example)