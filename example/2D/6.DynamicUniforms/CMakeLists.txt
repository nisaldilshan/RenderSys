add_executable(DynamicUniforms 
            main.cpp
)

target_link_libraries(DynamicUniforms PRIVATE RenderSys2D walnut::walnut)

target_compile_definitions(DynamicUniforms PRIVATE
    RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)


