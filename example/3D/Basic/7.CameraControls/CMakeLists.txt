add_executable(CameraControls
            main.cpp
)

target_link_libraries(CameraControls PRIVATE RenderSys3D walnut::walnut tinyobjloader)

target_compile_definitions(CameraControls PRIVATE
    RESOURCE_DIR="${CMAKE_SOURCE_DIR}/example/Resources"
)


