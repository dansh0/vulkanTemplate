#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "Vertex.h"

namespace graphics {

/**
 * Geometry class handles raw vertex and index data
 * Similar to Three.js's Geometry class but adapted for Vulkan
 */
class Geometry {
public:
    Geometry();
    ~Geometry() = default;

    // Vertex data management
    void setVertices(const std::vector<Vertex>& vertices);
    void setIndices(const std::vector<uint32_t>& indices);
    void clear();

    // Getters
    const std::vector<Vertex>& getVertices() const { return vertices_; }
    const std::vector<uint32_t>& getIndices() const { return indices_; }
    size_t getVertexCount() const { return vertices_.size(); }
    size_t getIndexCount() const { return indices_.size(); }
    bool hasGeometry() const { return !vertices_.empty(); }

    // Geometry operations
    void computeVertexNormals();
    void computeBoundingBox();
    void computeBoundingSphere();

    // Bounding volume getters
    const glm::vec3& getBoundingBoxMin() const { return boundingBoxMin_; }
    const glm::vec3& getBoundingBoxMax() const { return boundingBoxMax_; }
    const glm::vec3& getBoundingSphereCenter() const { return boundingSphereCenter_; }
    float getBoundingSphereRadius() const { return boundingSphereRadius_; }

private:
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;

    // Bounding volumes
    glm::vec3 boundingBoxMin_;
    glm::vec3 boundingBoxMax_;
    glm::vec3 boundingSphereCenter_;
    float boundingSphereRadius_;
};

} // namespace graphics 