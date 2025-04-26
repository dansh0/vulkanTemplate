#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include "../../geometry/Geometry.h"
#include "../../geometry/mesh/Mesh.h" // For IGeometryBuffer
#include "VulkanUtils.h"
#include "VulkanBuffer.h"

namespace graphics {

class VulkanBuffer;

/**
 * VulkanGeometryBuffer handles Vulkan-specific buffer management for geometry
 * This separates Vulkan-specific code from the core geometry classes
 */
class VulkanGeometryBuffer : public IGeometryBuffer {
public:
    VulkanGeometryBuffer(VkPhysicalDevice physicalDevice, VkDevice device,
                        VkQueue graphicsQueue, VkCommandPool commandPool);
    ~VulkanGeometryBuffer();

    // IGeometryBuffer interface implementation
    void updateVertexBuffer(const std::vector<Vertex>& vertices) override;
    void updateIndexBuffer(const std::vector<uint32_t>& indices) override;
    void bindBuffers() override;
    void draw() override;

    // Geometry management
    void setGeometry(std::shared_ptr<Geometry> geometry);
    std::shared_ptr<Geometry> getGeometry() const { return geometry_; }

    // Buffer management
    void createBuffers();
    void updateBuffers();
    void destroyBuffers();

    // Buffer access
    VkBuffer getVertexBuffer() const { return vertexBuffer_->getBuffer(); }
    VkBuffer getIndexBuffer() const { return indexBuffer_->getBuffer(); }
    uint32_t getIndexCount() const { return geometry_ ? geometry_->getIndexCount() : 0; }

    // State tracking
    bool needsUpdate() const { return needsUpdate_; }
    void markUpdated() { needsUpdate_ = false; }

private:
    VkPhysicalDevice physicalDevice_;
    VkDevice device_;
    VkQueue graphicsQueue_;
    VkCommandPool commandPool_;
    
    std::shared_ptr<Geometry> geometry_;
    
    // Vulkan buffers
    std::unique_ptr<VulkanBuffer> vertexBuffer_;
    std::unique_ptr<VulkanBuffer> indexBuffer_;
    
    bool needsUpdate_ = false;
};

} // namespace graphics 