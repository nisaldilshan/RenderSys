add_executable(Noise
            main.cpp
)

target_link_libraries(Noise PRIVATE ComputeSys RenderSysCommon walnut::walnut)

target_compile_definitions(Noise PRIVATE
    RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../Resources"
)


