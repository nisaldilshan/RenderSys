#pragma once
#include <vector>
#include <memory>
#include <string>
#include <glm/ext.hpp>

#include <RenderSys/Buffer.h>

namespace RenderSys
{

namespace Model
{
struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv0;
    glm::vec2 uv1;
    glm::uvec4 joint0;
    glm::vec4 weight0;
    glm::vec4 color;
    glm::vec3 tangent;
};

struct ModelData 
{
    ModelData() = default;
    ~ModelData() = default;
    ModelData(const ModelData&) = delete;
    ModelData &operator=(const ModelData&) = delete;
    ModelData(ModelData&&) = delete;
    ModelData &operator=(ModelData&&) = delete;

    const std::vector<Vertex>& getVertices() const;
    bool hasTangents() const;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

class ModelNode 
{
  public:
    ModelNode(int nodeNum);
    ~ModelNode() = default;

	ModelNode(const ModelNode&) = delete;
	ModelNode &operator=(const ModelNode&) = delete;
	ModelNode(ModelNode&&) = delete;
	ModelNode &operator=(ModelNode&&) = delete;

    int getNodeNum();
    void setNodeName(std::string name);
    void setScale(glm::vec3 scale);
    void setTranslation(glm::vec3 translation);
    void setRotation(glm::quat rotation);
	void setMesh(RenderSys::Mesh mesh);

    void calculateLocalTRSMatrix();
    void calculateNodeMatrix(glm::mat4 parentNodeMatrix);
    void calculateJointMatrices(const std::vector<glm::mat4>& inverseBindMatrices, const std::vector<int>& nodeToJoint, std::vector<glm::mat4>& jointMatrices);
    glm::mat4 getNodeMatrix();
    Mesh getMesh();
    void printHierarchy(int indent);

    std::vector<std::shared_ptr<ModelNode>> m_childNodes{};
	ModelData m_data;
  private:

    int m_nodeNum = 0;
    std::string m_nodeName;
    Mesh m_mesh;
    int m_materialIndex = 0;

    glm::vec3 m_scale = glm::vec3(1.0f);
    glm::vec3 m_translation = glm::vec3(0.0f);
    glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::mat4 mLocalTRSMatrix = glm::mat4(1.0f);
    glm::mat4 m_nodeMatrix = glm::mat4(1.0f);
};

} // namespace Model
} // namespace RenderSys