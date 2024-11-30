include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(2D/1.FirstTriangle)
add_subdirectory(2D/2.VertexBuffers)
add_subdirectory(2D/3.VertexAttributes)
add_subdirectory(2D/4.IndexBuffers)
add_subdirectory(2D/5.UniformBuffers)
add_subdirectory(2D/6.DynamicUniforms)

add_subdirectory(3D/Basic/1.First3DShape)
add_subdirectory(3D/Basic/2.TransformationMatrices)
add_subdirectory(3D/Basic/3.BasicShading)
add_subdirectory(3D/Basic/4.FirstTexture)
add_subdirectory(3D/Basic/5.TextureSampler)
add_subdirectory(3D/Basic/6.TextureLoader)
add_subdirectory(3D/Basic/7.CameraControls)
add_subdirectory(3D/Basic/8.Lighting)
add_subdirectory(3D/Basic/9.NormalMapping)
add_subdirectory(3D/Advanced/2.GLTFModel)

if(RENDERER STREQUAL "WebGPU")
    add_subdirectory(Compute/1.BasicCompute)
    add_subdirectory(Compute/2.Noise)
    add_subdirectory(Compute/3.Fluid2D)
    #add_subdirectory(Compute/4.Fluid3D)

    add_subdirectory(3D/Advanced/1.RayMarching)
    add_subdirectory(3D/Advanced/2.GLTFModel)
    
endif()

