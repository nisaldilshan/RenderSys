include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Lists examples and runs the selected one as a Walnut layer, see Common/ExampleBrowser.h
add_library(ExampleBrowser STATIC Common/ExampleBrowser.cpp)
target_link_libraries(ExampleBrowser PUBLIC walnut::walnut)

add_subdirectory(2D)
add_subdirectory(3D/Basic)

add_subdirectory(Compute/1.BasicCompute)
add_subdirectory(Compute/2.Noise)

if(RENDERER STREQUAL "Vulkan")
    add_subdirectory(3D/Advanced/2.GLTFModel)
    add_subdirectory(3D/Advanced/3.FirstAnimation)
    add_subdirectory(3D/Advanced/4.ShadowMapping)
endif()

if(RENDERER STREQUAL "WebGPU")
    add_subdirectory(Compute/3.Fluid2D)
    #add_subdirectory(Compute/4.Fluid3D)

    #add_subdirectory(3D/Advanced/1.RayMarching)
endif()

