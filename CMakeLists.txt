cmake_minimum_required (VERSION 3.22.0)

project (RenderSys)

find_package(walnut REQUIRED)

#message(STATUS "Walnut: ${walnut_INCLUDE_DIRS_RELEASE}")
#include_directories(${walnut_INCLUDE_DIRS_RELEASE})


add_library (RenderSys2D STATIC
                src/Renderer2D.cpp
                src/WebGPURenderer2D.cpp
            )

target_link_libraries(RenderSys2D PRIVATE walnut::walnut)


add_library (RenderSys3D STATIC
                src/Renderer3D.cpp
                src/WebGPURenderer3D.cpp
                src/Camera.cpp
            )

target_link_libraries(RenderSys3D PRIVATE walnut::walnut)


add_library (ComputeSys STATIC
                src/Compute.cpp
                src/WebGPUCompute.cpp
            )

target_link_libraries(ComputeSys PRIVATE walnut::walnut)