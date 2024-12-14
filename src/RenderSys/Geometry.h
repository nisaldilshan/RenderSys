#pragma once

#include <filesystem>
#include <glm/ext.hpp>
#include "Buffer.h"

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

template<typename T>
glm::mat3x3 computeTBN(const T corners[3], const glm::vec3& expectedN)
{
	// What we call e in the figure
	glm::vec3 ePos1 = corners[1].position - corners[0].position;
	glm::vec3 ePos2 = corners[2].position - corners[0].position;

	// What we call \bar e in the figure
	glm::vec2 eUV1 = corners[1].uv - corners[0].uv;
	glm::vec2 eUV2 = corners[2].uv - corners[0].uv;

	glm::vec3 T = normalize(ePos1 * eUV2.y - ePos2 * eUV1.y);
	glm::vec3 B = normalize(ePos2 * eUV1.x - ePos1 * eUV2.x);
	glm::vec3 N = cross(T, B);

	// Fix overall orientation
	if (dot(N, expectedN) < 0.0) {
		T = -T;
		B = -B;
		N = -N;
	}

	// Ortho-normalize the (T, B, expectedN) frame
	// a. "Remove" the part of T that is along expected N
	N = expectedN;
	T = normalize(T - dot(T, N) * N);
	// b. Recompute B from N and T
	B = cross(N, T);

	return glm::mat3x3(T, B, N);
}

template<typename T>
void populateTextureFrameAttributes(std::vector<T>& vertexData)
{
	size_t triangleCount = vertexData.size() / 3;
	// We compute the local texture frame per triangle
	for (int t = 0; t < triangleCount; ++t) {
		T* v = &vertexData[3 * t];

		for (int k = 0; k < 3; ++k) {
			glm::mat3x3 TBN = computeTBN<T>(v, v[k].normal);
			v[k].tangent = TBN[0];
			v[k].bitangent = TBN[1];
		}
	}
}

template<typename T>
bool loadGeometryFromObjWithUV(const fs::path& path, RenderSys::VertexBuffer& vertexData)
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
			auto texcoord_index = std::get<2>(tuple);

			vertexData[offset + i].position = {
				loader.m_vertices[3 * vertex_index + 0],
				-loader.m_vertices[3 * vertex_index + 2], // Add a minus to avoid mirroring
				loader.m_vertices[3 * vertex_index + 1]
			};

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

			vertexData[offset + i].texcoord0 = {
				loader.m_texcoords[2 * texcoord_index + 0],
				1 - loader.m_texcoords[2 * texcoord_index + 1]
			};
		}
	}

	return true;
}

}