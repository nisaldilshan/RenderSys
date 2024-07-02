from conans import ConanFile, tools
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain

class RenderSysConan(ConanFile):
    name = "RenderSys"
    version = "0.0.1"
    license = "MIT license"
    author = "Nisal Dilshan"
    settings = "os", "compiler", "build_type", "arch"
    generators = 'VirtualBuildEnv'
    
    def requirements(self):
        self.requires('walnut/latest')

    def build_requirements(self):
        self.tool_requires('pkgconf/1.7.4')
        self.tool_requires('cmake/3.25.3')

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["RENDERER"] = self.dependencies['walnut'].options.rendering_backend
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.verbose = True
        cmake.configure()
        cmake.build()

    def package_id(self):
        pass

    def package_info(self):
        pass
        # if self.options.rendering_backend == "OpenGL":
        #     self.cpp_info.libs = ["walnut", "walnut-graphics-opengl"]
        # elif self.options.rendering_backend == "Vulkan":
        #     self.cpp_info.libs = ["walnut", "walnut-graphics-vulkan"]
        # else:
        #     self.cpp_info.libs = ["walnut", "walnut-graphics-webgpu"]

    def package(self):
        cmake = CMake(self)
        cmake.install()
        # self.copy(pattern="Walnut/src/Walnut/EntryPoint.h", dst="lib", keep_path=False)
        # self.copy(pattern="Walnut/src/Walnut/Application.h", dst="lib", keep_path=False)
        # self.copy(pattern="Walnut/src/Walnut/Image.h", dst="lib", keep_path=False)

    def layout(self):
        self.folders.source = "."
        self.folders.build = "build"
        self.folders.generators = "build"
        self.folders.imports = self.folders.build