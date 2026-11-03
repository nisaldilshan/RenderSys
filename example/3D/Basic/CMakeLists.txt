# All basic 3D examples in one executable: each example is a layer, picked from a list in the UI
add_executable(Examples3DBasic
            main.cpp
            1.First3DShape/First3DShape.cpp
            2.TransformationMatrices/TransformationMatrices.cpp
            3.BasicShading/BasicShading.cpp
            4.FirstTexture/FirstTexture.cpp
            5.TextureSampler/TextureSampler.cpp
            6.TextureLoader/TextureLoader.cpp
            7.CameraControls/CameraControls.cpp
            8.Lighting/Lighting.cpp
            9.NormalMapping/NormalMapping.cpp
)

target_include_directories(Examples3DBasic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Examples3DBasic PRIVATE RenderSys3D RenderSysWalnut ExampleBrowser stb::stb)

# These examples load the model.txt in their own folder
set_source_files_properties(1.First3DShape/First3DShape.cpp PROPERTIES
    COMPILE_DEFINITIONS RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/1.First3DShape"
)
set_source_files_properties(2.TransformationMatrices/TransformationMatrices.cpp PROPERTIES
    COMPILE_DEFINITIONS RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/2.TransformationMatrices"
)
set_source_files_properties(3.BasicShading/BasicShading.cpp PROPERTIES
    COMPILE_DEFINITIONS RESOURCE_DIR="${CMAKE_SOURCE_DIR}/example/Resources/Meshes"
)
set_source_files_properties(
    4.FirstTexture/FirstTexture.cpp
    5.TextureSampler/TextureSampler.cpp
    6.TextureLoader/TextureLoader.cpp
    7.CameraControls/CameraControls.cpp
    8.Lighting/Lighting.cpp
    9.NormalMapping/NormalMapping.cpp
    PROPERTIES
    COMPILE_DEFINITIONS RESOURCE_DIR="${CMAKE_SOURCE_DIR}/example/Resources"
)
