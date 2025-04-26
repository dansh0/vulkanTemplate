#pragma once // Use include guards to prevent multiple inclusions

#define GLM_ENABLE_EXPERIMENTAL // Needed for gtx/hash
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp> // For std::hash<glm::vec3> specialization

#include <vulkan/vulkan.h> // For Vulkan types used in descriptions

#include <vector>
#include <array>
#include <cstddef> // For offsetof
#include <memory>

#include "../../core/object3D/Object3D.h"
#include "../Geometry.h"
#include "../Vertex.h"

namespace graphics {

class Material;

/**
 * @brief Interface for geometry buffer management
 * 
 * This interface abstracts the details of how geometry data is stored and managed
 * in different graphics APIs (Vulkan, OpenGL, etc.)
 */
class IGeometryBuffer {
public:
    virtual ~IGeometryBuffer() = default;
    
    /**
     * @brief Updates the vertex buffer with new data
     * @param vertices Vector of vertices to update with
     */
    virtual void updateVertexBuffer(const std::vector<Vertex>& vertices) = 0;
    
    /**
     * @brief Updates the index buffer with new data
     * @param indices Vector of indices to update with
     */
    virtual void updateIndexBuffer(const std::vector<uint32_t>& indices) = 0;
    
    /**
     * @brief Binds the buffers for rendering
     */
    virtual void bindBuffers() = 0;
    
    /**
     * @brief Draws the geometry using the bound buffers
     */
    virtual void draw() = 0;
};

/**
 * @brief Mesh class representing a 3D object with geometry data
 * 
 * The Mesh class extends Object3D to add geometry-specific functionality.
 * It stores vertex data, indices, and provides methods for geometry manipulation.
 * Also includes Vulkan-specific vertex buffer descriptions.
 */
class Mesh : public Object3D {
public:
    /**
     * @brief Constructs a new Mesh object
     * @param buffer The buffer implementation to use for managing geometry data
     */
    explicit Mesh(std::unique_ptr<IGeometryBuffer> buffer);
    ~Mesh() override = default;

    // Geometry management
    void setGeometry(std::shared_ptr<Geometry> geometry);
    std::shared_ptr<Geometry> getGeometry() const { return geometry_; }

    // Material management
    void setMaterial(std::shared_ptr<Material> material);
    std::shared_ptr<Material> getMaterial() const { return material_; }

    // Update methods
    void onBeforeUpdate() override;
    void onAfterUpdate() override;

    /**
     * @brief Gets the vertex data
     * @return Vector of vertices
     */
    const std::vector<Vertex>& getVertices() const { return vertices_; }
    
    /**
     * @brief Gets the index data
     * @return Vector of indices
     */
    const std::vector<uint32_t>& getIndices() const { return indices_; }
    
    /**
     * @brief Sets new vertex data and updates the buffer
     * @param vertices New vertex data
     */
    void setVertices(const std::vector<Vertex>& vertices);
    
    /**
     * @brief Sets new index data and updates the buffer
     * @param indices New index data
     */
    void setIndices(const std::vector<uint32_t>& indices);
    
    /**
     * @brief Clears all geometry data
     */
    void clearGeometry();
    
    /**
     * @brief Gets the number of vertices
     * @return Number of vertices
     */
    size_t getVertexCount() const { return vertices_.size(); }
    
    /**
     * @brief Gets the number of indices
     * @return Number of indices
     */
    size_t getIndexCount() const { return indices_.size(); }
    
    /**
     * @brief Checks if the mesh has geometry data
     * @return true if the mesh has geometry data, false otherwise
     */
    bool hasGeometry() const { return !vertices_.empty() && !indices_.empty(); }
    
    /**
     * @brief Binds the buffers for rendering
     */
    void bindBuffers() { buffer_->bindBuffers(); }
    
    /**
     * @brief Draws the geometry using the bound buffers
     */
    void draw() { buffer_->draw(); }

private:
    std::shared_ptr<Geometry> geometry_;
    std::shared_ptr<Material> material_;
    std::vector<Vertex> vertices_;           // Vertex data
    std::vector<uint32_t> indices_;          // Index data
    std::unique_ptr<IGeometryBuffer> buffer_; // Buffer implementation
    bool needsBufferUpdate_;                 // Flag to track if buffers need updating
};

} // namespace graphics