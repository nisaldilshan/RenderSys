add_executable(NormalMapping
            main.cpp
)

target_link_libraries(NormalMapping PRIVATE RenderSys3D RenderSysCommon walnut::walnut)

target_compile_definitions(NormalMapping PRIVATE
    RESOURCE_DIR="${CMAKE_SOURCE_DIR}/example/Resources"
)


