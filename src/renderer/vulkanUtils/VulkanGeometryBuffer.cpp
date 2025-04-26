#include "VulkanGeometryBuffer.h"
#include <stdexcept>

namespace graphics {

VulkanGeometryBuffer::VulkanGeometryBuffer(VkPhysicalDevice physicalDevice, VkDevice device,
                                         VkQueue graphicsQueue, VkCommandPool commandPool)
    : physicalDevice_(physicalDevice)
    , device_(device)
    , graphicsQueue_(graphicsQueue)
    , commandPool_(commandPool) {
}

VulkanGeometryBuffer::~VulkanGeometryBuffer() {
    destroyBuffers();
}

void VulkanGeometryBuffer::setGeometry(std::shared_ptr<Geometry> geometry) {
    geometry_ = geometry;
    needsUpdate_ = true;
}

void VulkanGeometryBuffer::createBuffers() {
    if (!geometry_) return;

    // Create vertex buffer
    VkDeviceSize vertexBufferSize = sizeof(Vertex) * geometry_->getVertexCount();
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    VulkanUtils::createBuffer(physicalDevice_, device_, vertexBufferSize,
                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            stagingBuffer, stagingBufferMemory);

    // Copy vertex data to staging buffer
    void* data;
    vkMapMemory(device_, stagingBufferMemory, 0, vertexBufferSize, 0, &data);
    memcpy(data, geometry_->getVertices().data(), vertexBufferSize);
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
    VulkanUtils::createBuffer(physicalDevice_, device_, indexBufferSize,
                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            stagingBuffer, stagingBufferMemory);

    // Copy index data to staging buffer
    vkMapMemory(device_, stagingBufferMemory, 0, indexBufferSize, 0, &data);
    memcpy(data, geometry_->getIndices().data(), indexBufferSize);
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
    destroyBuffers();
    createBuffers();
    needsUpdate_ = false;
}

void VulkanGeometryBuffer::destroyBuffers() {
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
}

void VulkanGeometryBuffer::updateIndexBuffer(const std::vector<uint32_t>& indices) {
    if (!geometry_) {
        geometry_ = std::make_shared<Geometry>();
    }
    geometry_->setIndices(indices);
    needsUpdate_ = true;
}

void VulkanGeometryBuffer::bindBuffers() {
    if (needsUpdate_) {
        updateBuffers();
    }
    
    // Note: In Vulkan, buffer binding is done during command buffer recording
    // This method is kept for interface compatibility but actual binding
    // happens in the command buffer recording phase
}

void VulkanGeometryBuffer::draw() {
    if (needsUpdate_) {
        updateBuffers();
    }
    
    // Note: In Vulkan, drawing is done during command buffer recording
    // This method is kept for interface compatibility but actual drawing
    // happens in the command buffer recording phase
}

} // namespace graphics 