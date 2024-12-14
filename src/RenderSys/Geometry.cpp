#include "Geometry.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

#define TINYOBJLOADER_IMPLEMENTATION // add this to exactly 1 of your C++ files
#include <tiny_obj_loader.h>

namespace Geometry
{
namespace fs = std::filesystem;

bool loadGeometryFromObj(const fs::path& path, RenderSys::VertexBuffer& vertexData)
{
	TinyObjLoader loader;
	if (!loader.load(path.string()))
	{
		return false;
	}

	// Filling in vertexData:
	vertexData.clear();
	for (const auto& shape : loader.m_meshIndices) {
		size_t offset = vertexData.size();
		vertexData.resize(offset + shape.size());

		for (size_t i = 0; i < shape.size(); ++i) 
		{
			const auto& tuple = shape[i];
			auto vertex_index = std::get<0>(tuple);
            auto normal_index = std::get<1>(tuple);

			vertexData[offset + i].position = {
				loader.m_vertices[3 * vertex_index + 0],
				-loader.m_vertices[3 * vertex_index + 2], // Add a minus to avoid mirroring
				loader.m_vertices[3 * vertex_index + 1]
			};

			// Also apply the transform to normals!!
			vertexData[offset + i].normal = {
				loader.m_normals[3 * normal_index + 0],
				-loader.m_normals[3 * normal_index + 2],
				loader.m_normals[3 * normal_index + 1]
			};

			vertexData[offset + i].color = {
				loader.m_colors[3 * vertex_index + 0],
				loader.m_colors[3 * vertex_index + 1],
				loader.m_colors[3 * vertex_index + 2]
			};
		}
	}

	return true;
}

bool load2DGeometry(const fs::path& path, std::vector<float>& vertexData, std::vector<uint16_t>& indexData) 
{
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    vertexData.clear();
    indexData.clear();

    enum class Section {
        None,
        Points,
        Indices,
    };
    Section currentSection = Section::None;

    float value;
    uint16_t index;
    std::string line;
    while (!file.eof()) {
        getline(file, line);
        
        // overcome the `CRLF` problem
            if (!line.empty() && line.back() == '\r') {
              line.pop_back();
            }
        
        if (line == "[points]") {
            currentSection = Section::Points;
        }
        else if (line == "[indices]") {
            currentSection = Section::Indices;
        }
        else if (line[0] == '#' || line.empty()) {
            // Do nothing, this is a comment
        }
        else if (currentSection == Section::Points) {
            std::istringstream iss(line);
            // Get x, y, r, g, b
            for (int i = 0; i < 5; ++i) {
                iss >> value;
                vertexData.push_back(value);
            }
        }
        else if (currentSection == Section::Indices) {
            std::istringstream iss(line);
            // Get corners #0 #1 and #2
            for (int i = 0; i < 3; ++i) {
                iss >> index;
                indexData.push_back(index);
            }
        }
    }
    return true;
}

bool load3DGeometry(const fs::path& path, std::vector<float>& pointData, std::vector<uint16_t>& indexData, int dimensions) {
	std::ifstream file(path);
	if (!file.is_open()) {
		return false;
	}

	pointData.clear();
	indexData.clear();

	enum class Section {
		None,
		Points,
		Indices,
	};
	Section currentSection = Section::None;

	float value;
	uint16_t index;
	std::string line;
	while (!file.eof()) {
		getline(file, line);
		if (line == "[points]") {
			currentSection = Section::Points;
		}
		else if (line == "[indices]") {
			currentSection = Section::Indices;
		}
		else if (line[0] == '#' || line.empty()) {
			// Do nothing, this is a comment
		}
		else if (currentSection == Section::Points) {
			std::istringstream iss(line);
			// Get x, y, r, g, b
			for (int i = 0; i < dimensions + 3; ++i) {
				iss >> value;
				pointData.push_back(value);
			}
		}
		else if (currentSection == Section::Indices) {
			std::istringstream iss(line);
			// Get corners #0 #1 and #2
			for (int i = 0; i < 3; ++i) {
				iss >> index;
				indexData.push_back(index);
			}
		}
	}
	return true;
}

bool TinyObjLoader::load(const std::string &path)
{
    tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	// Call the core loading procedure of TinyOBJLoader
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str());

	// Check errors
	if (!warn.empty()) {
		std::cout << warn << std::endl;
	}

	if (!err.empty()) {
		std::cerr << err << std::endl;
	}

	if (!ret) {
		return false;
	}

    for (const auto &shape : shapes)
    {
        auto& mesh = m_meshIndices.emplace_back();
        for (const auto &index : shape.mesh.indices)
        {
            auto& tuple = mesh.emplace_back();
            std::get<0>(tuple) = index.vertex_index;
            std::get<1>(tuple) = index.normal_index;
            std::get<2>(tuple) = index.texcoord_index;
        }
    }
    
	m_vertices = attrib.vertices;
	m_normals = attrib.normals;
	m_colors = attrib.colors;
	m_texcoords = attrib.texcoords;

    return true;
}

void populateTextureFrameAttributes(RenderSys::VertexBuffer& vertexData)
{
	size_t triangleCount = vertexData.size() / 3;
	// We compute the local texture frame per triangle
	for (int t = 0; t < triangleCount; ++t) {
		RenderSys::Vertex* v = &vertexData[3 * t];

		for (int k = 0; k < 3; ++k) {
			glm::mat3x3 TBN = computeTBN<RenderSys::Vertex>(v, v[k].normal);
			v[k].tangent = TBN[0];
			//v[k].bitangent = TBN[1];
		}
	}
}

}