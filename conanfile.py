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
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.verbose = True
        cmake.configure()
        cmake.build()