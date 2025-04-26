#include "Geometry.h"
#include <algorithm>

namespace graphics {

Geometry::Geometry() 
    : boundingBoxMin_(glm::vec3(0.0f))
    , boundingBoxMax_(glm::vec3(0.0f))
    , boundingSphereCenter_(glm::vec3(0.0f))
    , boundingSphereRadius_(0.0f)
{
}

void Geometry::setVertices(const std::vector<Vertex>& vertices) {
    vertices_ = vertices;
    computeBoundingBox();
    computeBoundingSphere();
}

void Geometry::setIndices(const std::vector<uint32_t>& indices) {
    indices_ = indices;
}

void Geometry::clear() {
    vertices_.clear();
    indices_.clear();
    boundingBoxMin_ = glm::vec3(0.0f);
    boundingBoxMax_ = glm::vec3(0.0f);
    boundingSphereCenter_ = glm::vec3(0.0f);
    boundingSphereRadius_ = 0.0f;
}

void Geometry::computeVertexNormals() {
    if (vertices_.empty() || indices_.empty()) return;

    // Reset all normals to zero
    for (auto& vertex : vertices_) {
        vertex.normal = glm::vec3(0.0f);
    }

    // Compute face normals and add them to vertex normals
    for (size_t i = 0; i < indices_.size(); i += 3) {
        const auto& v0 = vertices_[indices_[i]];
        const auto& v1 = vertices_[indices_[i + 1]];
        const auto& v2 = vertices_[indices_[i + 2]];

        glm::vec3 edge1 = v1.pos - v0.pos;
        glm::vec3 edge2 = v2.pos - v0.pos;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        vertices_[indices_[i]].normal += normal;
        vertices_[indices_[i + 1]].normal += normal;
        vertices_[indices_[i + 2]].normal += normal;
    }

    // Normalize all vertex normals
    for (auto& vertex : vertices_) {
        vertex.normal = glm::normalize(vertex.normal);
    }
}

void Geometry::computeBoundingBox() {
    if (vertices_.empty()) return;

    boundingBoxMin_ = vertices_[0].pos;
    boundingBoxMax_ = vertices_[0].pos;

    for (const auto& vertex : vertices_) {
        boundingBoxMin_ = glm::min(boundingBoxMin_, vertex.pos);
        boundingBoxMax_ = glm::max(boundingBoxMax_, vertex.pos);
    }
}

void Geometry::computeBoundingSphere() {
    if (vertices_.empty()) return;

    // Use bounding box center as initial sphere center
    boundingSphereCenter_ = (boundingBoxMin_ + boundingBoxMax_) * 0.5f;
    
    // Find maximum distance from center to any vertex
    float maxDistance = 0.0f;
    for (const auto& vertex : vertices_) {
        float distance = glm::distance(vertex.pos, boundingSphereCenter_);
        maxDistance = std::max(maxDistance, distance);
    }
    
    boundingSphereRadius_ = maxDistance;
}

} // namespace graphics 