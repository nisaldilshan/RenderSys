add_executable(Lighting
            main.cpp
)

target_link_libraries(Lighting PRIVATE RenderSys3D RenderSysCommon walnut::walnut)

target_compile_definitions(Lighting PRIVATE
    RESOURCE_DIR="${CMAKE_SOURCE_DIR}/example/Resources"
)


