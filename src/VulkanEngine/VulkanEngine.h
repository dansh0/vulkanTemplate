#pragma once

// Use Vulkan included via GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Use Vulkan's depth range [0, 1]
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Geometry/Mesh.h" // Include Vertex definition
#include "VulkanUtils.h"      // Include helper functions and structs

#include <vector>
#include <string>
#include <optional>
#include <stdexcept> // For runtime_error
#include <chrono>    // Potentially for timing within engine later

// Forward declare Scene class to avoid circular includes if Scene needs VulkanEngine
class Scene;

/**
 * @brief Encapsulates the core Vulkan initialization, rendering resources, and frame loop logic.
 *
 * This class manages all the fundamental Vulkan objects (Instance, Device, Swapchain, Pipeline, etc.)
 * and provides an interface to initialize Vulkan, manage resources, and render frames.
 * It aims to abstract away much of the boilerplate setup.
 *
 * Keywords: Vulkan Abstraction, Rendering Engine Core, Vulkan Initialization, Frame Loop
 */
class VulkanEngine {
public:
    /**
     * @brief Constructor. Takes the GLFW window handle.
     * @param window Pointer to the initialized GLFW window.
     */
    VulkanEngine(GLFWwindow* window);

    /**
     * @brief Destructor. Calls the main cleanup function.
     */
    ~VulkanEngine();

    /**
     * @brief Initializes all necessary Vulkan components.
     *
     * This is the main setup function, calling various private helper methods.
     * Should be called once after the VulkanEngine object is created.
     * @param scene Reference to the scene object to get initial mesh data.
     */
    void initVulkan(const Scene& scene);

    /**
     * @brief Cleans up all Vulkan resources created by this engine.
     *
     * Should be called before the application exits.
     */
    void cleanup();

    /**
     * @brief Executes the rendering logic for a single frame.
     *
     * This function handles:
     * - Waiting for the previous frame to finish.
     * - Acquiring the next swapchain image.
     * - Updating the uniform buffer (with data likely provided by the Scene).
     * - Recording drawing commands into a command buffer.
     * - Submitting the command buffer.
     * - Presenting the rendered image.
     * - Handling swapchain recreation if necessary.
     * @param scene Reference to the scene to get current object transforms (for UBO).
     */
    void drawFrame(const Scene& scene);

    /**
     * @brief Notifies the engine that the framebuffer has been resized.
     *
     * This flag is checked during drawFrame to trigger swapchain recreation.
     */
    void notifyFramebufferResized();

     // --- Debug Callback ---
    // Static member function to be used as the callback by Vulkan
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData); // pUserData can be used to pass `this` pointer if needed

private:
    // --- Core Vulkan Objects ---
    GLFWwindow* window; // Pointer to the application window
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE; // Logical device
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    // --- Swapchain Objects ---
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    // --- Pipeline Objects ---
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    // --- Framebuffers ---
    std::vector<VkFramebuffer> swapChainFramebuffers;

    // --- Command Objects ---
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers; // One per frame in flight

    // --- Buffers & Memory ---
    // Geometry buffers (handles owned by engine, data provided by scene at init)
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    uint32_t indexCount = 0; // Store index count after buffer creation

    // Uniform buffers (one per frame in flight)
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped; // Persistently mapped pointers

    // --- Descriptors ---
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets; // One per frame in flight

    // --- Depth Buffering ---
    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;

    // --- Synchronization ---
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0; // Index for current frame in flight (0 to MAX_FRAMES_IN_FLIGHT-1)
    int MAX_FRAMES_IN_FLIGHT = 2; // Number of frames to process concurrently

    // --- State Flags ---
    bool framebufferResized = false; // Flag set by GLFW callback

    // --- Private Initialization Steps ---
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createCommandPool();
    void createDepthResources();
    void createFramebuffers();
    void createVertexBuffer(const std::vector<Vertex>& vertices);
    void createIndexBuffer(const std::vector<uint32_t>& indices);
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffers();
    void createSyncObjects();

    // --- Private Runtime Steps ---
    void updateUniformBuffer(uint32_t currentImageIndex, const Scene& scene);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void cleanupSwapChain();
    void recreateSwapChain(const Scene& scene); // Needs scene data again for buffers

    // --- Private Helper Functions ---
    // (Device suitability checks are closely tied to engine state)
    bool isDeviceSuitable(VkPhysicalDevice queryDevice);
    bool checkDeviceExtensionSupport(VkPhysicalDevice queryDevice);
    VulkanUtils::QueueFamilyIndices findQueueFamilies(VkPhysicalDevice queryDevice);
    VulkanUtils::SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice queryDevice);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    std::vector<const char*> getRequiredExtensions();
    bool checkValidationLayerSupport();

    // Flag to enable/disable validation layers (set based on build type)
    #ifdef NDEBUG
        const bool enableValidationLayers = false;
    #else
        const bool enableValidationLayers = true;
    #endif

    // List of required validation layers
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    // List of required device extensions
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // --- UBO Struct Definition ---
    // Kept here as it's tightly coupled with the shader and UBO updates in the engine
    struct UniformBufferObject {
        alignas(16) glm::mat4 model; // alignas(16) ensures alignment meets Vulkan requirements (vec4 size)
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };
};