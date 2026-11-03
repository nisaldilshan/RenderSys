# All 2D examples in one executable: each example is a layer, picked from a list in the UI
add_executable(Examples2D
            main.cpp
            1.FirstTriangle/FirstTriangle.cpp
            2.VertexBuffers/VertexBuffers.cpp
            3.VertexAttributes/VertexAttributes.cpp
            4.IndexBuffers/IndexBuffers.cpp
            5.UniformBuffers/UniformBuffers.cpp
            6.DynamicUniforms/DynamicUniforms.cpp
)

target_include_directories(Examples2D PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Examples2D PRIVATE RenderSys2D RenderSysWalnut ExampleBrowser)

# These examples load the model.txt in their own folder
set_source_files_properties(5.UniformBuffers/UniformBuffers.cpp PROPERTIES
    COMPILE_DEFINITIONS RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/5.UniformBuffers"
)
set_source_files_properties(6.DynamicUniforms/DynamicUniforms.cpp PROPERTIES
    COMPILE_DEFINITIONS RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/6.DynamicUniforms"
)
