#include "VulkanGeometryBuffer.h"
#include <stdexcept>

namespace graphics {

VulkanGeometryBuffer::VulkanGeometryBuffer(VkPhysicalDevice physicalDevice, VkDevice device,
                                         VkQueue graphicsQueue, VkCommandPool commandPool)
    : physicalDevice_(physicalDevice)
    , device_(device)
    , graphicsQueue_(graphicsQueue)
    , commandPool_(commandPool)
    , vertexBuffer_(nullptr)
    , indexBuffer_(nullptr)
    , needsUpdate_(true) {
    // Initialize with empty geometry
    geometry_ = std::make_shared<Geometry>();
    // Create empty buffers immediately
    createBuffers();
}

VulkanGeometryBuffer::~VulkanGeometryBuffer() {
    // Ensure buffers are destroyed before device is destroyed
    destroyBuffers();
}

void VulkanGeometryBuffer::setGeometry(std::shared_ptr<Geometry> geometry) {
    if (!geometry) {
        throw std::runtime_error("Cannot set null geometry");
    }
    geometry_ = geometry;
    needsUpdate_ = true;
    // Create buffers immediately when geometry is set
    createBuffers();
}

void VulkanGeometryBuffer::createBuffers() {
    if (!geometry_) {
        throw std::runtime_error("Cannot create buffers without geometry");
    }

    // Destroy existing buffers if they exist
    destroyBuffers();

    // Create vertex buffer
    VkDeviceSize vertexBufferSize = sizeof(Vertex) * geometry_->getVertexCount();
    if (vertexBufferSize == 0) {
        // Create empty buffer if no vertices
        vertexBufferSize = sizeof(Vertex); // Minimum size
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    VulkanUtils::createBuffer(physicalDevice_, device_, vertexBufferSize,
                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            stagingBuffer, stagingBufferMemory);

    // Copy vertex data to staging buffer
    void* data;
    vkMapMemory(device_, stagingBufferMemory, 0, vertexBufferSize, 0, &data);
    if (geometry_->getVertexCount() > 0) {
        memcpy(data, geometry_->getVertices().data(), vertexBufferSize);
    }
    vkUnmapMemory(device_, stagingBufferMemory);

    // Create vertex buffer
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VulkanUtils::createBuffer(physicalDevice_, device_, vertexBufferSize,
                            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                            vertexBuffer, vertexBufferMemory);

    // Copy from staging to vertex buffer
    VulkanUtils::copyBuffer(device_, commandPool_, graphicsQueue_,
                          stagingBuffer, vertexBuffer, vertexBufferSize);

    // Cleanup staging buffer
    vkDestroyBuffer(device_, stagingBuffer, nullptr);
    vkFreeMemory(device_, stagingBufferMemory, nullptr);

    // Create index buffer
    VkDeviceSize indexBufferSize = sizeof(uint32_t) * geometry_->getIndexCount();
    if (indexBufferSize == 0) {
        // Create empty buffer if no indices
        indexBufferSize = sizeof(uint32_t); // Minimum size
    }

    VulkanUtils::createBuffer(physicalDevice_, device_, indexBufferSize,
                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            stagingBuffer, stagingBufferMemory);

    // Copy index data to staging buffer
    vkMapMemory(device_, stagingBufferMemory, 0, indexBufferSize, 0, &data);
    if (geometry_->getIndexCount() > 0) {
        memcpy(data, geometry_->getIndices().data(), indexBufferSize);
    }
    vkUnmapMemory(device_, stagingBufferMemory);

    // Create index buffer
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    VulkanUtils::createBuffer(physicalDevice_, device_, indexBufferSize,
                            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                            indexBuffer, indexBufferMemory);

    // Copy from staging to index buffer
    VulkanUtils::copyBuffer(device_, commandPool_, graphicsQueue_,
                          stagingBuffer, indexBuffer, indexBufferSize);

    // Cleanup staging buffer
    vkDestroyBuffer(device_, stagingBuffer, nullptr);
    vkFreeMemory(device_, stagingBufferMemory, nullptr);

    // Store buffer handles
    vertexBuffer_ = std::make_unique<VulkanBuffer>(vertexBuffer, vertexBufferMemory);
    indexBuffer_ = std::make_unique<VulkanBuffer>(indexBuffer, indexBufferMemory);
}

void VulkanGeometryBuffer::updateBuffers() {
    if (!needsUpdate_) return;
    if (!geometry_) {
        throw std::runtime_error("Cannot update buffers without geometry");
    }
    createBuffers();
    needsUpdate_ = false;
}

void VulkanGeometryBuffer::destroyBuffers() {
    // Wait for device to finish operations before destroying buffers
    vkDeviceWaitIdle(device_);

    if (vertexBuffer_) {
        vkDestroyBuffer(device_, vertexBuffer_->getBuffer(), nullptr);
        vkFreeMemory(device_, vertexBuffer_->getMemory(), nullptr);
        vertexBuffer_.reset();
    }
    if (indexBuffer_) {
        vkDestroyBuffer(device_, indexBuffer_->getBuffer(), nullptr);
        vkFreeMemory(device_, indexBuffer_->getMemory(), nullptr);
        indexBuffer_.reset();
    }
}

void VulkanGeometryBuffer::updateVertexBuffer(const std::vector<Vertex>& vertices) {
    if (!geometry_) {
        geometry_ = std::make_shared<Geometry>();
    }
    geometry_->setVertices(vertices);
    needsUpdate_ = true;
    createBuffers(); // Create buffers immediately
}

void VulkanGeometryBuffer::updateIndexBuffer(const std::vector<uint32_t>& indices) {
    if (!geometry_) {
        geometry_ = std::make_shared<Geometry>();
    }
    geometry_->setIndices(indices);
    needsUpdate_ = true;
    createBuffers(); // Create buffers immediately
}

void VulkanGeometryBuffer::bindBuffers() {
    if (!vertexBuffer_ || !indexBuffer_) {
        throw std::runtime_error("Cannot bind buffers: buffers not initialized");
    }
}

void VulkanGeometryBuffer::draw() {
    if (!vertexBuffer_ || !indexBuffer_) {
        throw std::runtime_error("Cannot draw: buffers not initialized");
    }
}

} // namespace graphics 