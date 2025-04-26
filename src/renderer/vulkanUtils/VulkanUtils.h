#pragma once

#define GLFW_INCLUDE_VULKAN // Ensure Vulkan headers are included via GLFW
#include <GLFW/glfw3.h>     // For VkSurfaceKHR and other window system integration types

#include <vector>
#include <string>
#include <optional> // For QueueFamilyIndices

// Forward declare Vulkan handles used in function signatures
// This avoids including vulkan.h directly in this header, reducing compile times slightly
// for files that only need these declarations. Actual implementations in VulkanUtils.cpp
// will include the full vulkan.h.
struct VkPhysicalDevice_T; typedef VkPhysicalDevice_T* VkPhysicalDevice;
struct VkDevice_T; typedef VkDevice_T* VkDevice;
struct VkInstance_T; typedef VkInstance_T* VkInstance;
struct VkBuffer_T; typedef VkBuffer_T* VkBuffer;
struct VkDeviceMemory_T; typedef VkDeviceMemory_T* VkDeviceMemory;
struct VkImage_T; typedef VkImage_T* VkImage;
struct VkImageView_T; typedef VkImageView_T* VkImageView;
struct VkCommandPool_T; typedef VkCommandPool_T* VkCommandPool;
struct VkQueue_T; typedef VkQueue_T* VkQueue;
struct VkShaderModule_T; typedef VkShaderModule_T* VkShaderModule;
struct VkDebugUtilsMessengerEXT_T; typedef VkDebugUtilsMessengerEXT_T* VkDebugUtilsMessengerEXT;
struct VkAllocationCallbacks;
struct VkDebugUtilsMessengerCreateInfoEXT;


/**
 * @brief Namespace containing utility functions and structures for Vulkan operations.
 *
 * This helps organize helper functions that don't belong to the main VulkanEngine class
 * but are essential for its operation.
 */
namespace VulkanUtils {

    /**
     * @brief Holds indices of queue families required by the application.
     *
     * Vulkan commands are submitted to queues, and queues belong to families.
     * We need to find queue families that support graphics operations and presentation.
     *
     * `std::optional` is used because a physical device might not support the required families.
     *
     * Keywords: Queue Family, Graphics Queue, Present Queue, VkQueueFamilyProperties
     */
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily; // Index of a queue family supporting graphics commands
        std::optional<uint32_t> presentFamily;  // Index of a queue family supporting presentation to a surface

        /**
         * @brief Checks if all required queue families have been found.
         * @return true if both graphics and present families have values, false otherwise.
         */
        bool isComplete() const { // Added const qualifier
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    /**
     * @brief Contains details about the swap chain capabilities of a physical device.
     *
     * Before creating a swap chain, we need to query the device's capabilities,
     * supported surface formats, and presentation modes.
     *
     * Keywords: Swap Chain Support, VkSurfaceCapabilitiesKHR, VkSurfaceFormatKHR, VkPresentModeKHR
     */
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;        // Basic surface properties (min/max image count, extent, transforms)
        std::vector<VkSurfaceFormatKHR> formats;      // Supported image formats and color spaces
        std::vector<VkPresentModeKHR> presentModes;   // Supported presentation modes (e.g., FIFO, Mailbox)
    };

    // --- Function Declarations ---

    /**
     * @brief Reads the entire content of a binary file.
     * @param filename Path to the file.
     * @return std::vector<char> containing the file content. Throws std::runtime_error on failure.
     *
     * Commonly used to load compiled SPIR-V shader code.
     *
     * Keywords: File I/O, SPIR-V Loading
     */
    std::vector<char> readFile(const std::string& filename);

    /**
     * @brief Creates a Vulkan Shader Module from SPIR-V code.
     * @param device The logical device.
     * @param code A vector of chars containing the SPIR-V bytecode.
     * @return The created VkShaderModule handle. Throws std::runtime_error on failure.
     *
     * A shader module wraps the shader bytecode, making it ready for use in a pipeline stage.
     * The module is destroyed later using vkDestroyShaderModule.
     *
     * Keywords: VkShaderModule, SPIR-V, Shader Bytecode, Pipeline Stages
     */
    VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);


    /**
     * @brief Finds a suitable memory type index on the physical device.
     * @param physicalDevice The physical device to query.
     * @param typeFilter A bitmask specifying allowed memory types (from VkMemoryRequirements::memoryTypeBits).
     * @param properties Required memory property flags (e.g., HOST_VISIBLE, DEVICE_LOCAL).
     * @return The index of a suitable memory type. Throws std::runtime_error if no suitable type is found.
     *
     * Vulkan exposes different memory heaps with varying properties (CPU visible, GPU local, etc.).
     * Buffers and images have memory type requirements. This function finds a memory type that
     * satisfies both the resource's requirements (typeFilter) and the application's needs (properties).
     *
     * Keywords: VkMemoryType, VkMemoryPropertyFlags, VkPhysicalDeviceMemoryProperties, Memory Allocation
     */
    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    /**
     * @brief Creates a Vulkan buffer and allocates memory for it.
     * @param physicalDevice The physical device (needed for memory properties).
     * @param device The logical device.
     * @param size The desired size of the buffer in bytes.
     * @param usage Flags specifying how the buffer will be used (e.g., Vertex Buffer, Transfer Source).
     * @param properties Required memory properties for the buffer's backing memory.
     * @param buffer Reference to store the created VkBuffer handle.
     * @param bufferMemory Reference to store the allocated VkDeviceMemory handle.
     *
     * This is a fundamental helper for creating any buffer (vertex, index, uniform, staging).
     * It handles buffer creation, querying memory requirements, finding a suitable memory type,
     * allocating the memory, and binding the memory to the buffer.
     *
     * Keywords: VkBuffer, VkDeviceMemory, Buffer Creation, Memory Allocation, VkBufferUsageFlags, vkCreateBuffer, vkAllocateMemory, vkBindBufferMemory
     */
    void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    /**
     * @brief Copies data from one Vulkan buffer to another using a command buffer.
     * @param device The logical device.
     * @param commandPool The command pool to allocate the temporary command buffer from.
     * @param graphicsQueue The queue to submit the copy command to.
     * @param srcBuffer The source buffer.
     * @param dstBuffer The destination buffer.
     * @param size The number of bytes to copy.
     *
     * This function encapsulates the process of:
     * 1. Allocating a temporary command buffer.
     * 2. Recording a vkCmdCopyBuffer command.
     * 3. Submitting the command buffer to a queue.
     * 4. Waiting for the queue to become idle (ensuring the copy completes).
     * 5. Freeing the temporary command buffer.
     * Essential for transferring data from CPU-visible staging buffers to GPU-local device buffers.
     *
     * Keywords: vkCmdCopyBuffer, Buffer Transfer, Staging Buffer, Command Buffer Submission, vkQueueSubmit, vkQueueWaitIdle
     */
    void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    /**
     * @brief Creates a Vulkan image object and allocates memory for it.
     * @param physicalDevice The physical device (needed for memory properties).
     * @param device The logical device.
     * @param width Image width.
     * @param height Image height.
     * @param format The pixel format of the image (e.g., VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_D32_SFLOAT).
     * @param tiling Tiling mode (OPTIMAL for device access, LINEAR for host access).
     * @param usage Flags specifying how the image will be used (e.g., Sampled, Color Attachment, Depth Attachment).
     * @param properties Required memory properties for the image's backing memory.
     * @param image Reference to store the created VkImage handle.
     * @param imageMemory Reference to store the allocated VkDeviceMemory handle.
     *
     * Similar to createBuffer, but for 2D image resources like textures or attachments.
     *
     * Keywords: VkImage, VkDeviceMemory, Image Creation, Texture, Framebuffer Attachment, VkImageUsageFlags, VkFormat, VkImageTiling
     */
    void createImage(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

    /**
     * @brief Creates a Vulkan image view.
     * @param device The logical device.
     * @param image The VkImage to create a view for.
     * @param format The format of the image (must be compatible with the image's format).
     * @param aspectFlags Specifies which aspects of the image are included in the view (e.g., COLOR, DEPTH).
     * @return The created VkImageView handle. Throws std::runtime_error on failure.
     *
     * An image view describes how to access an image and which part of it to access.
     * It's needed to use an image as a texture sampler, render target attachment, etc.
     * Views interpret the raw image data according to the specified format and aspect mask.
     *
     * Keywords: VkImageView, Image View, Texture View, Framebuffer Attachment View, VkImageAspectFlags
     */
    VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

    /**
     * @brief Finds a suitable image format from a list of candidates that supports requested features.
     * @param physicalDevice The physical device to query format properties from.
     * @param candidates A vector of potential VkFormat values to check.
     * @param tiling The desired image tiling (VK_IMAGE_TILING_OPTIMAL or VK_IMAGE_TILING_LINEAR).
     * @param features Required format features (e.g., VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT).
     * @return The first supported VkFormat from the candidates list. Throws std::runtime_error if none are supported.
     *
     * Devices may support different features for different image formats and tilings. This function helps select
     * a format that meets the application's requirements (e.g., usable as a depth buffer with optimal tiling).
     *
     * Keywords: VkFormat, VkFormatProperties, VkFormatFeatureFlags, Format Selection, vkGetPhysicalDeviceFormatProperties
     */
    VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    /**
     * @brief Finds the best supported depth buffer format.
     * @param physicalDevice The physical device to query.
     * @return A supported VkFormat suitable for depth buffering (e.g., D32_SFLOAT, D24_UNORM_S8_UINT).
     * Throws std::runtime_error if no suitable format is found.
     *
     * Convenience function using findSupportedFormat to specifically look for common depth formats.
     * Prefers formats with higher precision if available.
     *
     * Keywords: Depth Buffer, Depth Format, Z-Buffer
     */
    VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);

    // --- Debug Messenger Functions ---
    // Need full definition as they use PFN types directly

    /**
     * @brief Dynamically loads and calls vkCreateDebugUtilsMessengerEXT.
     * @param instance The Vulkan instance.
     * @param pCreateInfo Pointer to the messenger create info structure.
     * @param pAllocator Optional custom allocator callbacks.
     * @param pDebugMessenger Pointer to store the created messenger handle.
     * @return VkResult indicating success or failure.
     *
     * This function is needed because vkCreateDebugUtilsMessengerEXT is an extension function
     * and might not be directly available as a statically linked function. We load its pointer
     * using vkGetInstanceProcAddr.
     *
     * Keywords: Debug Messenger, Validation Layers, vkCreateDebugUtilsMessengerEXT, vkGetInstanceProcAddr, Extension Function
     */
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

    /**
     * @brief Dynamically loads and calls vkDestroyDebugUtilsMessengerEXT.
     * @param instance The Vulkan instance.
     * @param debugMessenger The messenger handle to destroy.
     * @param pAllocator Optional custom allocator callbacks.
     *
     * Similar to CreateDebugUtilsMessengerEXT, this loads the destruction function pointer dynamically.
     *
     * Keywords: Debug Messenger, Validation Layers, vkDestroyDebugUtilsMessengerEXT, vkGetInstanceProcAddr
     */
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

    /**
     * @brief Populates a VkDebugUtilsMessengerCreateInfoEXT structure with default settings.
     * @param createInfo Reference to the struct to populate.
     *
     * Sets up severity levels (Warning, Error), message types (General, Validation, Performance),
     * and assigns the debug callback function. The callback function itself must be defined elsewhere
     * (typically as a static member function, like VulkanEngine::debugCallback).
     *
     * Keywords: Debug Messenger Setup, VkDebugUtilsMessengerCreateInfoEXT, Validation Layers
     */
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo); // <-- ADD THIS DECLARATION

    VkResult CreateDebugUtilsMessengerEXT(/*...*/); // Keep existing declarations
    void DestroyDebugUtilsMessengerEXT(/*...*/);    // Keep existing declarations

} // namespace VulkanUtils