from conans import ConanFile, tools
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain

class RenderSysConan(ConanFile):
    name = "RenderSys"
    version = "0.0.1"
    license = "MIT license"
    url = "https://developer.avatarin.com/"
    description = "avatarin's development framework (CoreSDK)"
    author = "Nisal Dilshan"
    settings = "os", "compiler", "build_type", "arch"
    generators = 'VirtualBuildEnv'
    
    def requirements(self):
        self.requires('walnut/latest')

    def build_requirements(self):
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
        self.cpp_info.defines = ["RENDERER_BACKEND=3"] # using 3 as backend support only WebGPU

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def layout(self):
        self.folders.source = "."
        self.folders.build = "build"
        self.folders.generators = "build"
        self.folders.imports = self.folders.build