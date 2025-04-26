#pragma once

#include <vulkan/vulkan.h>

namespace graphics {

/**
 * @brief Simple wrapper for Vulkan buffer and its associated memory
 * 
 * This class manages a single Vulkan buffer and its associated device memory.
 * It provides access to the buffer and memory handles and handles cleanup.
 */
class VulkanBuffer {
public:
    /**
     * @brief Constructs a new VulkanBuffer
     * @param buffer The Vulkan buffer handle
     * @param memory The associated device memory handle
     */
    VulkanBuffer(VkBuffer buffer, VkDeviceMemory memory)
        : buffer_(buffer), memory_(memory) {}

    /**
     * @brief Gets the Vulkan buffer handle
     * @return The buffer handle
     */
    VkBuffer getBuffer() const { return buffer_; }

    /**
     * @brief Gets the device memory handle
     * @return The memory handle
     */
    VkDeviceMemory getMemory() const { return memory_; }

private:
    VkBuffer buffer_;
    VkDeviceMemory memory_;
};

} // namespace graphics 