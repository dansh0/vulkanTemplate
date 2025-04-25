#include "VulkanUtils.h"
#include <vulkan/vulkan.h> // Include full Vulkan header for implementations
#include "../VulkanEngine/VulkanEngine.h" // Needed for VulkanEngine::debugCallback
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <set>
#include <cstring> // For strcmp in checkDeviceExtensionSupport (can be removed if moved)
#include <algorithm> // For std::clamp in chooseSwapExtent (can be removed if moved)

// Define implementations within the VulkanUtils namespace
namespace VulkanUtils {

    /**
     * @brief Reads a binary file. Implementation.
     * See VulkanUtils.h for details.
     */
    std::vector<char> readFile(const std::string& filename) {
        // std::ios::ate: Start reading at the end of the file (to easily get size)
        // std::ios::binary: Read file in binary mode (don't do any CRLF translation)
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }

        // Get file size from current position (which is end due to std::ios::ate)
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        // Seek back to beginning and read the whole file
        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();
        return buffer;
    }

    /**
     * @brief Creates a VkShaderModule. Implementation.
     * See VulkanUtils.h for details.
     */
    VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        // Note: Vulkan expects SPIR-V code as an array of uint32_t, but std::vector<char>
        // usually works if the data is correctly aligned. A static_assert or careful
        // handling might be needed in edge cases.
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader module!");
        }
        return shaderModule;
    }

     /**
     * @brief Finds a suitable memory type index. Implementation.
     * See VulkanUtils.h for details.
     */
    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        // Query the available memory types and heaps on the physical device.
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        // Iterate through the available memory types.
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            // Check if the i-th memory type is allowed by the typeFilter bitmask.
            // (typeFilter has a bit set for each memory type allowed by the resource).
            bool typeAllowed = (typeFilter & (1 << i));

            // Check if the i-th memory type has all the required property flags.
            // (e.g., HOST_VISIBLE means the CPU can map it, DEVICE_LOCAL means it's on discrete GPU).
            bool propertiesMatch = (memProperties.memoryTypes[i].propertyFlags & properties) == properties;

            if (typeAllowed && propertiesMatch) {
                return i; // Found a suitable memory type index.
            }
        }

        throw std::runtime_error("Failed to find suitable memory type!");
    }

    /**
     * @brief Creates a Vulkan buffer and its memory. Implementation.
     * See VulkanUtils.h for details.
     */
    void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        // 1. Define buffer properties
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;           // Size in bytes
        bufferInfo.usage = usage;         // How the buffer will be used
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Only used by one queue family (graphics)

        // 2. Create the buffer handle
        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create buffer!");
        }

        // 3. Query memory requirements for the buffer
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        // 4. Define memory allocation info
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size; // Allocate required size
        // Find a memory type index that fits the buffer's requirements and our desired properties
        allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

        // 5. Allocate the device memory
        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            // Clean up the buffer handle if memory allocation fails
            vkDestroyBuffer(device, buffer, nullptr);
            throw std::runtime_error("Failed to allocate buffer memory!");
        }

        // 6. Bind the allocated memory to the buffer handle
        // The '0' is the memory offset - the buffer starts at the beginning of the allocated memory block.
        VkResult bindResult = vkBindBufferMemory(device, buffer, bufferMemory, 0);
        if(bindResult != VK_SUCCESS) {
             // Clean up if binding fails
            vkDestroyBuffer(device, buffer, nullptr);
            vkFreeMemory(device, bufferMemory, nullptr);
             throw std::runtime_error("Failed to bind buffer memory!");
        }
    }

    /**
     * @brief Copies data between buffers. Implementation.
     * See VulkanUtils.h for details.
     */
    void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        // --- Create a temporary command buffer for the copy operation ---
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // Can be submitted directly
        allocInfo.commandPool = commandPool; // Pool to allocate from
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        VkResult allocateResult = vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
        if (allocateResult != VK_SUCCESS) {
             throw std::runtime_error("Failed to allocate temporary command buffer for copy!");
        }


        // --- Record the copy command ---
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: Hint that we'll submit this buffer once and then discard it.
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        // Define the copy region (copy the entire buffer)
        VkBufferCopy copyRegion{};
        // copyRegion.srcOffset = 0; // Optional: default 0
        // copyRegion.dstOffset = 0; // Optional: default 0
        copyRegion.size = size;
        // Record the copy command
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        vkEndCommandBuffer(commandBuffer);

        // --- Submit the command buffer to the graphics queue ---
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer; // The buffer containing the copy command

        // Submit and wait for completion. Using a fence here would be more efficient for overlapping
        // multiple transfers, but waiting for idle is simpler for a single blocking copy.
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE); // No fence needed for blocking wait
        vkQueueWaitIdle(graphicsQueue); // Block CPU until the copy operation finishes on the GPU

        // --- Clean up the temporary command buffer ---
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }


    /**
     * @brief Creates a Vulkan image and its memory. Implementation.
     * See VulkanUtils.h for details.
     */
    void createImage(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
        // 1. Define image properties
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D; // Standard 2D image
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1; // Depth is 1 for 2D images
        imageInfo.mipLevels = 1;    // Not using mipmaps for now
        imageInfo.arrayLayers = 1;  // Not using image arrays for now
        imageInfo.format = format;
        imageInfo.tiling = tiling;  // Optimal or Linear
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Layout when first created
        imageInfo.usage = usage;    // How the image will be used
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT; // No multisampling for now
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Only used by one queue family

        // 2. Create the image handle
        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image!");
        }

        // 3. Query memory requirements
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        // 4. Define memory allocation info
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

        // 5. Allocate memory
        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
             vkDestroyImage(device, image, nullptr); // Clean up image handle
            throw std::runtime_error("Failed to allocate image memory!");
        }

        // 6. Bind memory to the image
        VkResult bindResult = vkBindImageMemory(device, image, imageMemory, 0); // Bind at offset 0
         if(bindResult != VK_SUCCESS) {
             vkDestroyImage(device, image, nullptr);
             vkFreeMemory(device, imageMemory, nullptr);
             throw std::runtime_error("Failed to bind image memory!");
         }
    }

    /**
     * @brief Creates a Vulkan image view. Implementation.
     * See VulkanUtils.h for details.
     */
    VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;          // The image to view
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // Treat as a 2D image view
        viewInfo.format = format;        // How to interpret the image data
        // Swizzling allows remapping color channels (e.g., R to G). IDENTITY keeps them as is.
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        // Subresource range describes which part of the image the view accesses.
        viewInfo.subresourceRange.aspectMask = aspectFlags; // COLOR, DEPTH, or STENCIL aspect
        viewInfo.subresourceRange.baseMipLevel = 0;         // Start at mip level 0
        viewInfo.subresourceRange.levelCount = 1;           // View only one mip level
        viewInfo.subresourceRange.baseArrayLayer = 0;       // Start at array layer 0
        viewInfo.subresourceRange.layerCount = 1;           // View only one array layer

        VkImageView imageView;
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image view!");
        }
        return imageView;
    }

    /**
     * @brief Finds a supported format. Implementation.
     * See VulkanUtils.h for details.
     */
     VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        // Iterate through the candidate formats provided by the caller.
        for (VkFormat format : candidates) {
            // Query the properties of the current format for the physical device.
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            // Check if the required features are supported for the specified tiling mode.
            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format; // Found a suitable format with linear tiling.
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format; // Found a suitable format with optimal tiling.
            }
        }
        // If no suitable format was found in the candidates list.
        throw std::runtime_error("Failed to find supported format!");
    }

    /**
     * @brief Finds the best supported depth format. Implementation.
     * See VulkanUtils.h for details.
     */
    VkFormat findDepthFormat(VkPhysicalDevice physicalDevice) {
        // Call findSupportedFormat with common depth formats, preferring higher precision.
        // Optimal tiling is generally preferred for depth buffers accessed only by the GPU.
        // The feature required is VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT.
        return findSupportedFormat(
            physicalDevice,
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }


    // --- Debug Messenger Functions ---

    /**
     * @brief Populates VkDebugUtilsMessengerCreateInfoEXT. Implementation.
     * See VulkanUtils.h for details.
     */
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {}; // Initialize struct members to zero/null
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        // Define desired severity and message types
        // Add VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT for maximum debugging output if needed
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        // Point to the static callback function defined in VulkanEngine
        // We need to include VulkanEngine.h in VulkanUtils.cpp for this line to work!
        createInfo.pfnUserCallback = VulkanEngine::debugCallback;
        createInfo.pUserData = nullptr; // Optional: Can pass user data (e.g., 'this' pointer) if callback isn't static
    }

    /**
     * @brief Dynamically loads and calls vkCreateDebugUtilsMessengerEXT. Implementation.
     * See VulkanUtils.h for details.
     */
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        // Get the function pointer for vkCreateDebugUtilsMessengerEXT.
        // This function is part of an extension, so we need to load it dynamically using vkGetInstanceProcAddr.
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            // If the function pointer was loaded successfully, call the function.
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            // If the function pointer is null, the extension is likely not available or enabled.
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    /**
     * @brief Dynamically loads and calls vkDestroyDebugUtilsMessengerEXT. Implementation.
     * See VulkanUtils.h for details.
     */
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
        // Get the function pointer for vkDestroyDebugUtilsMessengerEXT.
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            // If loaded, call the function to destroy the messenger.
            func(instance, debugMessenger, pAllocator);
        }
        // No return value to check here; if the function isn't found, we can't destroy the messenger
        // using this specific function, but cleanup should proceed otherwise.
    }


} // namespace VulkanUtils