#pragma once

#include <filesystem>
#include <glm/ext.hpp>

namespace Geometry
{
namespace fs = std::filesystem;

bool load2DGeometry(const fs::path& path, std::vector<float>& vertexData, std::vector<uint16_t>& indexData);
bool load3DGeometry(const fs::path& path, std::vector<float>& pointData, std::vector<uint16_t>& indexData, int dimensions);

class TinyObjLoader
{
public:
	bool load(const std::string& path);
	std::vector<std::vector<std::tuple<int,int,int>>> m_meshIndices;
	std::vector<float> m_vertices;
	std::vector<float> m_normals;
	std::vector<float> m_colors;
	std::vector<float> m_texcoords;
};

template<typename T>
bool loadGeometryFromObj(const fs::path& path, std::vector<T>& vertexData)
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

		for (size_t i = 0; i < shape.size(); ++i) {
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

template<typename T>
glm::mat3x3 computeTBN(const T corners[3], const glm::vec3& expectedN);

template<typename T>
void populateTextureFrameAttributes(std::vector<T>& vertexData);

template<typename T>
bool loadGeometryFromObjWithUV(const fs::path& path, std::vector<T>& vertexData);

}