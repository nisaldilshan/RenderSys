add_executable(DynamicUniforms 
            main.cpp
            ${CMAKE_SOURCE_DIR}/src/Renderer2D.cpp
            ${CMAKE_SOURCE_DIR}/src/WebGPURenderer2D.cpp)

target_include_directories(DynamicUniforms PRIVATE ../../Walnut/src)

target_link_libraries(DynamicUniforms PRIVATE walnut::walnut)

target_compile_definitions(DynamicUniforms PRIVATE
    RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)


