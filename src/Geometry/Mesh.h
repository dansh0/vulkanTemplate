#pragma once // Use include guards to prevent multiple inclusions

#define GLM_ENABLE_EXPERIMENTAL // Needed for gtx/hash
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp> // For std::hash<glm::vec3> specialization

#include <vulkan/vulkan.h> // For Vulkan types used in descriptions

#include <vector>
#include <array>
#include <cstddef> // For offsetof

/**
 * @brief Represents a single vertex in a 3D mesh.
 *
 * Contains position and color information. Also provides static methods
 * to describe its layout to Vulkan pipelines.
 *
 * Keywords: Vertex Data, Vertex Input, Vertex Attribute, Vertex Binding
 */
struct Vertex {
    glm::vec3 pos;   // Position in 3D space
    glm::vec3 color; // Color associated with the vertex

    /**
     * @brief Provides the Vulkan binding description for this vertex type.
     * @return VkVertexInputBindingDescription specifying how vertex data is spaced in memory.
     *
     * A binding description defines a collection of vertex attributes that are loaded
     * together. Here, we have only one binding (binding 0) containing all our vertex data.
     * - `binding`: The index of the binding (matches the index used in vkCmdBindVertexBuffers).
     * - `stride`: The byte distance between consecutive vertex entries in the buffer.
     * - `inputRate`: Specifies whether to advance per-vertex or per-instance.
     *   - `VK_VERTEX_INPUT_RATE_VERTEX`: Load data for each vertex.
     *   - `VK_VERTEX_INPUT_RATE_INSTANCE`: Load data once per instance (for instanced rendering).
     *
     * Keywords: VkVertexInputBindingDescription, Vertex Stride, Input Rate
     */
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0; // We use binding index 0
        bindingDescription.stride = sizeof(Vertex); // Size of one Vertex struct
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Data is per-vertex
        return bindingDescription;
    }

    /**
     * @brief Provides the Vulkan attribute descriptions for this vertex type.
     * @return std::array containing VkVertexInputAttributeDescription for each attribute (pos, color).
     *
     * Attribute descriptions define how to extract individual vertex attributes (like position, color)
     * from the chunk of data specified by a binding description.
     * - `binding`: Which binding description this attribute belongs to (matches getBindingDescription).
     * - `location`: The location index referenced in the vertex shader (`layout(location = ...) in ...`).
     * - `format`: The data type and size of the attribute (e.g., 3 floats). Must match shader input type.
     *   Common formats: `VK_FORMAT_R32G32_SFLOAT` (vec2), `VK_FORMAT_R32G32B32_SFLOAT` (vec3), `VK_FORMAT_R32G32B32A32_SFLOAT` (vec4).
     * - `offset`: The byte offset of this attribute within the Vertex struct (calculated using `offsetof`).
     *
     * Keywords: VkVertexInputAttributeDescription, Vertex Attribute Format, Shader Location, Vertex Offset
     */
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        // Position Attribute (location = 0)
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0; // layout(location = 0) in shader
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 (3 * 32-bit float)
        attributeDescriptions[0].offset = offsetof(Vertex, pos); // Offset of the 'pos' member

        // Color Attribute (location = 1)
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1; // layout(location = 1) in shader
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 (3 * 32-bit float)
        attributeDescriptions[1].offset = offsetof(Vertex, color); // Offset of the 'color' member

        return attributeDescriptions;
    }

    /**
     * @brief Equality operator for Vertex comparison.
     * @param other The other Vertex to compare against.
     * @return true if position and color are equal, false otherwise.
     *
     * Useful for tasks like vertex deduplication if loading models or generating complex geometry.
     */
    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color;
    }
};

// --- Provide a hash function specialization for Vertex ---
// This allows Vertex structs to be used as keys in std::unordered_map (e.g., for vertex deduplication).
// It relies on glm::vec3 having a hash specialization (provided by <glm/gtx/hash.hpp>).
namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            // Combine hashes of position and color using bitwise operations.
            // This is a simple hash combine function; more robust ones exist.
            return ((hash<glm::vec3>()(vertex.pos) ^
                   (hash<glm::vec3>()(vertex.color) << 1)) >> 1);
        }
    };
} // namespace std