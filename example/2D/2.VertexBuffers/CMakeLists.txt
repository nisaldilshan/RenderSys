add_executable(VertexBuffers 
            main.cpp
)

target_link_libraries(VertexBuffers PRIVATE RenderSys2D walnut::walnut)

target_compile_definitions(VertexBuffers PRIVATE
    RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
