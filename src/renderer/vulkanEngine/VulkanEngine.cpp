#include "VulkanEngine.h"
#include "../../core/Scene/Scene.h" // Include Scene to get data

#include <set>        // For unique queue families
#include <cstring>    // For strcmp
#include <algorithm>  // For std::clamp
#include <iostream>   // For setup messages / errors
#include <fstream>    // For file operations
#include <chrono>     // For time-based operations

namespace graphics {

// --- Constructor ---
VulkanEngine::VulkanEngine(GLFWwindow* glfwWindow) : window(glfwWindow) {
    if (!window) {
        throw std::runtime_error("GLFW window handle provided to VulkanEngine is null!");
    }
    // Initialize other members to default/null if not done in header
}

// --- Destructor ---
VulkanEngine::~VulkanEngine() {
    // cleanup() should ideally be called explicitly before destruction,
    // but calling it here provides some safety net.
    // However, relying on destructors for Vulkan cleanup can be problematic
    // if exceptions occur during construction or if object lifetime is complex.
    if (device != VK_NULL_HANDLE) { // Basic check if init was partially successful
        // This wait is crucial if cleanup wasn't called explicitly
        // to ensure GPU is idle before destroying resources.
        vkDeviceWaitIdle(device);
        cleanup();
    }
}

// --- Public Methods ---

/**
 * @brief Initializes all Vulkan components. Orchestrator method.
 * @param scene The scene object providing initial geometry data.
 *
 * Calls private helper methods in the correct order to set up the Vulkan instance,
 * device, swapchain, pipeline, buffers, descriptors, and synchronization objects.
 *
 * Keywords: Vulkan Initialization Sequence
 */
void VulkanEngine::initVulkan(const Scene& scene) {
    try {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();      // Color views
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createCommandPool();     // Create pool before buffers that might need it for copies
        createDepthResources();
        createFramebuffers();    // Create framebuffers after render pass and image views

        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();

         std::cout << "Vulkan Engine Initialized Successfully." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Vulkan Initialization Error: " << e.what() << std::endl;
        // Perform partial cleanup if possible before re-throwing
        cleanup(); // Attempt to clean up whatever was created
        throw; // Re-throw the exception to signal failure
    }
}

/**
 * @brief Main cleanup function. Destroys Vulkan objects in reverse order of creation.
 *
 * Ensures all Vulkan resources are properly released before the application exits.
 * It's crucial to call vkDeviceWaitIdle before starting cleanup if rendering was active.
 *
 * Keywords: Vulkan Cleanup, Resource Destruction Order, vkDeviceWaitIdle
 */
void VulkanEngine::cleanup() {
    // Wait for the device to be idle before destroying anything,
    // especially if called implicitly by destructor or after an error.
    // If called explicitly after main loop, vkDeviceWaitIdle in mainLoop is sufficient.
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
    }

    // Cleanup swap chain first (depends on command buffers)
    cleanupSwapChain();

    // Cleanup command pool (depends on command buffers)
    if (commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, commandPool, nullptr);
        commandPool = VK_NULL_HANDLE;
    }

    // Cleanup pipeline
    if (graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        graphicsPipeline = VK_NULL_HANDLE;
    }

    // Cleanup pipeline layout
    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }

    // Cleanup render pass
    if (renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }

    // Cleanup descriptor pool
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
    }

    // Cleanup descriptor set layout
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        descriptorSetLayout = VK_NULL_HANDLE;
    }

    // Cleanup uniform buffers
    for (size_t i = 0; i < uniformBuffers.size(); i++) {
        if (uniformBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            uniformBuffers[i] = VK_NULL_HANDLE;
        }
        if (uniformBuffersMemory[i] != VK_NULL_HANDLE) {
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
            uniformBuffersMemory[i] = VK_NULL_HANDLE;
        }
    }
    uniformBuffers.clear();
    uniformBuffersMemory.clear();
    uniformBuffersMapped.clear();

    // Cleanup semaphores and fences
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Check if vectors were populated before destroying
        if (i < renderFinishedSemaphores.size() && renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            renderFinishedSemaphores[i] = VK_NULL_HANDLE;
        }
        if (i < imageAvailableSemaphores.size() && imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            imageAvailableSemaphores[i] = VK_NULL_HANDLE;
        }
        if (i < inFlightFences.size() && inFlightFences[i] != VK_NULL_HANDLE) {
            vkDestroyFence(device, inFlightFences[i], nullptr);
            inFlightFences[i] = VK_NULL_HANDLE;
        }
    }
    imageAvailableSemaphores.clear();
    renderFinishedSemaphores.clear();
    inFlightFences.clear();

    // Cleanup device
    if (device != VK_NULL_HANDLE) {
        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }

    // Cleanup debug messenger
    if (enableValidationLayers && debugMessenger != VK_NULL_HANDLE) {
        VulkanUtils::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        debugMessenger = VK_NULL_HANDLE;
    }

    // Cleanup surface
    if (surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        surface = VK_NULL_HANDLE;
    }

    // Cleanup instance
    if (instance != VK_NULL_HANDLE) {
        vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
    }

    std::cout << "Vulkan Engine Cleaned Up." << std::endl;
}

/**
 * @brief Handles window resizing by recreating the swapchain and dependent resources.
 * @param scene Scene object needed to recreate vertex/index buffers if necessary (though currently not needed as they are device local).
 *
 * Called when the window size changes. Destroys the old swapchain, framebuffers,
 * depth buffer, and image views, then recreates them with the new dimensions.
 *
 * Keywords: Swapchain Recreation, Window Resize Handling, Framebuffer Resizing
 */
void VulkanEngine::recreateSwapChain(const Scene& scene) {
    // Handle minimization: pause rendering until window is restored
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents(); // Wait for window events (like restore)
    }

    // Wait for the device to finish any current work before destroying resources
    vkDeviceWaitIdle(device);

    // Cleanup old resources
    cleanupSwapChain();

    // Recreate resources with new size/properties
    createSwapChain();
    createImageViews();     // Color views for new swapchain images
    // RenderPass doesn't usually need recreation unless multisampling changes etc.
    createDepthResources(); // Depth buffer needs new size
    createFramebuffers();   // Framebuffers need new size and attachments

    // Buffers (Vertex, Index, Uniform) generally don't need recreation unless their
    // usage/size requirements change fundamentally, which isn't the case on resize.
    // If they were HOST_VISIBLE and mapped, the mapping might need updating, but
    // ours are DEVICE_LOCAL (geometry) or persistently mapped (UBO).

     std::cout << "Swapchain Recreated." << std::endl;

}


/**
 * @brief Main function to render a single frame.
 * @param scene The scene containing objects to render and their state.
 *
 * Orchestrates waiting, image acquisition, command recording, submission, and presentation.
 * Includes logic to handle swapchain recreation automatically if needed.
 *
 * Keywords: Render Loop, Frame Submission, Presentation, Synchronization Primitives
 */
void VulkanEngine::drawFrame(const Scene& scene) {
    // 1. Wait for the fence associated with the 'currentFrame' index.
    // This ensures that the command buffer and resources for this frame index
    // are no longer in use by the GPU from a previous iteration.
    // Timeout is UINT64_MAX, effectively waiting indefinitely.
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // 2. Acquire an available image index from the swapchain.
    // The imageAvailableSemaphore[currentFrame] will be signaled when the presentation
    // engine is finished with this image and it's ready for us to render to.
    uint32_t imageIndex;
    VkResult acquireResult = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    // Handle cases where the swapchain is no longer optimal or usable.
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain(scene); // Recreate swapchain and try again next frame.
        return;
    } else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        // Suboptimal also usually means we should recreate, but we can still present the acquired image.
        // We throw an error for other acquisition failures.
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

     // --- Frame is ready to be rendered ---

    // 3. Update the uniform buffer for the current frame index with scene data.
    updateUniformBuffer(currentFrame, scene);

    // 4. Reset the fence *before* submitting new work that will signal it.
    // We only reset the fence if we are sure we are going to submit work using it.
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    // 5. Reset and Record the command buffer for the current frame index.
    vkResetCommandBuffer(commandBuffers[currentFrame], 0); // Reset the buffer before re-recording
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex, scene); // Record drawing commands


    // 6. Submit the command buffer to the graphics queue.
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // Specify which semaphores to wait for before execution begins.
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    // Specify the pipeline stage(s) where waiting should occur.
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    // Specify the command buffers to execute.
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    // Specify which semaphores to signal once command buffer execution finishes.
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Submit the work. The inFlightFences[currentFrame] will be signaled upon completion.
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    // 7. Present the rendered image to the window.
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    // Wait for rendering to finish (signaled by renderFinishedSemaphore) before presentation.
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    // Specify the swapchain and image index to present.
    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    // presentInfo.pResults = nullptr; // Optional: Get results for multiple swapchains

    VkResult presentResult = vkQueuePresentKHR(presentQueue, &presentInfo);

    // Handle swapchain issues detected during presentation or if a resize happened concurrently.
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false; // Reset the flag if it was set by the callback
        recreateSwapChain(scene);
    } else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    // 8. Advance to the next frame index for the next iteration.
    // Modulo operator ensures wrapping around MAX_FRAMES_IN_FLIGHT.
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

/**
 * @brief Sets the internal flag indicating the framebuffer was resized.
 */
void VulkanEngine::notifyFramebufferResized() {
    framebufferResized = true;
}


// --- Private Initialization Steps ---

/**
 * @brief Creates the Vulkan Instance.
 *
 * The instance connects the application to the Vulkan library and requires enabling
 * necessary extensions (like those for window system integration and debug utils).
 * Validation layers are also enabled here if configured.
 *
 * Keywords: VkInstance, vkCreateInstance, Instance Extensions, Validation Layers, VkApplicationInfo
 */
void VulkanEngine::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("Validation layers requested, but not available!");
    }

    // Optional: Provides info about the application to the driver.
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Obj Viewer App"; // Customize app name
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Custom Vulkan Engine"; // Customize engine name
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0; // Target Vulkan 1.0

    // --- Instance Creation Info ---
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Get required extensions (GLFW + Debug Utils)
    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Enable validation layers and potentially the debug messenger create info
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{}; // Define outside 'if'
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        // Populate the debug messenger create info struct
        // And chain it to the instance create info using pNext.
        // This allows the messenger to log messages during instance creation/destruction.
        VulkanUtils::populateDebugMessengerCreateInfo(debugCreateInfo); // Assuming this exists in VulkanUtils now
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    // Create the instance
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }
     std::cout << "Vulkan Instance Created." << std::endl;

}

/**
 * @brief Creates the Debug Utils Messenger.
 *
 * This requires the VK_EXT_debug_utils extension. The messenger calls our static
 * debugCallback function when validation layers emit messages.
 *
 * Keywords: Debug Messenger, vkCreateDebugUtilsMessengerEXT, Validation Callback
 */
void VulkanEngine::setupDebugMessenger() {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    VulkanUtils::populateDebugMessengerCreateInfo(createInfo); // Populate the struct

    // Use the helper from VulkanUtils to handle dynamic function loading
    if (VulkanUtils::CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("Failed to set up debug messenger!");
    }
     std::cout << "Debug Messenger Set Up." << std::endl;

}

/**
 * @brief Creates the Window Surface (VkSurfaceKHR).
 *
 * Connects Vulkan to the platform's window system using GLFW's helper function.
 * The surface is needed for presentation.
 *
 * Keywords: VkSurfaceKHR, Window System Integration (WSI), glfwCreateWindowSurface
 */
void VulkanEngine::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!");
    }
     std::cout << "Window Surface Created." << std::endl;

}

/**
 * @brief Selects a suitable physical device (GPU).
 *
 * Enumerates available GPUs and checks if they support the required features,
 * extensions, queue families, and swap chain capabilities.
 *
 * Keywords: VkPhysicalDevice, GPU Selection, Device Enumeration, Device Suitability
 */
void VulkanEngine::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // Find the first suitable device
    for (const auto& deviceIter : devices) {
        if (isDeviceSuitable(deviceIter)) {
            physicalDevice = deviceIter;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }

    // Optional: Print the name of the chosen device
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    std::cout << "Selected Physical Device: " << deviceProperties.deviceName << std::endl;

}

/**
 * @brief Creates the Logical Device (VkDevice).
 *
 * Represents the application's connection to the chosen physical device.
 * Specifies which queues to create and enables required device features and extensions.
 *
 * Keywords: VkDevice, Logical Device, vkCreateDevice, Device Queues, Device Features
 */
void VulkanEngine::createLogicalDevice() {
    VulkanUtils::QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    // Use a set to handle cases where graphics and present families are the same
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f; // Priority between 0.0 and 1.0
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1; // Create one queue from this family
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Specify device features to enable (none needed for this basic example)
    VkPhysicalDeviceFeatures deviceFeatures{};
    // Example: if (supportedFeatures.samplerAnisotropy) { deviceFeatures.samplerAnisotropy = VK_TRUE; }

    // --- Logical Device Create Info ---
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    // Enable required device extensions (e.g., swapchain)
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // Enable validation layers (consistent with instance creation)
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    // Create the logical device
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device!");
    }

    // Get handles to the created device queues
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
     std::cout << "Logical Device Created." << std::endl;

}

/**
 * @brief Creates the Swap Chain (VkSwapchainKHR).
 *
 * The swap chain is a queue of images waiting to be presented to the screen.
 * Chooses optimal surface format, presentation mode, and extent based on device capabilities.
 *
 * Keywords: VkSwapchainKHR, vkCreateSwapchainKHR, Swap Chain Images, Presentation
 */
void VulkanEngine::createSwapChain() {
    VulkanUtils::SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    // Determine the number of images in the swap chain (min + 1 for triple buffering, respecting max limit)
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    // --- Swap Chain Create Info ---
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1; // 1 for non-stereoscopic rendering
    // Specify that images will be used as color attachments in the render pass
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Handle image sharing between graphics and present queues if they differ
    VulkanUtils::QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Slower, needs explicit ownership transfer
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // Faster, assumes same queue
    }

    // Use the current surface transform (usually identity)
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    // Blend with the window system (OPAQUE means ignore window alpha)
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE; // Allow clipping of obscured pixels
    // createInfo.oldSwapchain = VK_NULL_HANDLE; // For resizing, provide the old one here

    // Create the swapchain object
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain!");
    }

    // Retrieve the handles to the swap chain images
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr); // Get count first
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data()); // Get image handles

    // Store the chosen format and extent for later use
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

     std::cout << "Swap Chain Created (Images: " << imageCount << ", Format: " << surfaceFormat.format << ", Extent: " << extent.width << "x" << extent.height << ")" << std::endl;
}

/**
 * @brief Creates Image Views (VkImageView) for each swap chain image.
 *
 * Image views are needed to interpret the raw image data (e.g., as color).
 *
 * Keywords: VkImageView, vkCreateImageView, Swap Chain Image View
 */
void VulkanEngine::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        try {
            // Use the helper function to create the view
            swapChainImageViews[i] = VulkanUtils::createImageView(device, swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        } catch (const std::exception& e) {
            // Clean up previously created views if one fails
             for(size_t j = 0; j < i; ++j) {
                 if (swapChainImageViews[j] != VK_NULL_HANDLE) {
                     vkDestroyImageView(device, swapChainImageViews[j], nullptr);
                 }
             }
             swapChainImageViews.clear(); // Clear the vector
             throw; // Re-throw the exception
        }
    }
     std::cout << "Swap Chain Image Views Created." << std::endl;

}

/**
 * @brief Creates the Render Pass (VkRenderPass).
 *
 * Defines the attachments (color, depth) used during rendering, the subpasses,
 * and dependencies between subpasses. Specifies how attachments are handled
 * (e.g., cleared at the start, stored at the end).
 *
 * Keywords: VkRenderPass, vkCreateRenderPass, Framebuffer Attachments, Subpass, Render Pass Dependency
 */
void VulkanEngine::createRenderPass() {
    // --- Define Attachments ---
    // Color Attachment (from swap chain image)
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat; // Format matches swapchain
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // No multisampling
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear before rendering
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store results after rendering
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Not using stencil
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Layout before pass starts
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Layout after pass for presentation

    // Depth Attachment
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VulkanUtils::findDepthFormat(physicalDevice); // Get supported depth format
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear depth before rendering
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Discard depth after rendering
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Optimal layout for depth testing

    // --- Define Attachment References ---
    // References are used by subpasses to specify which attachments to use.
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0; // Index of the color attachment (in the pAttachments array)
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Layout during the subpass

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1; // Index of the depth attachment
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // --- Define Subpass ---
    // Only one subpass in this simple example.
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // For graphics pipelines
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef; // Use the color attachment reference
    subpass.pDepthStencilAttachment = &depthAttachmentRef; // Use the depth attachment reference

    // --- Define Subpass Dependency ---
    // Controls execution order and memory dependencies between subpasses (or external).
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // Implicit subpass before render pass
    dependency.dstSubpass = 0; // Our single subpass
    // Stages to wait for in the source (before pass)
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0; // No access required before pass starts (layout transition handles it)
    // Stages in the destination (our subpass) that depend on the source
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    // Access types that are required in the destination subpass
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // --- Render Pass Create Info ---
    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    // Create the render pass object
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    }
     std::cout << "Render Pass Created." << std::endl;

}

/**
 * @brief Creates the Descriptor Set Layout (VkDescriptorSetLayout).
 *
 * Defines the layout (bindings, types) of descriptors (like UBOs, samplers)
 * that will be used by the pipeline. Shaders reference descriptors via this layout.
 *
 * Keywords: VkDescriptorSetLayout, vkCreateDescriptorSetLayout, Descriptor Binding, UBO Layout
 */
void VulkanEngine::createDescriptorSetLayout() {
    // Define a binding for the Uniform Buffer Object (UBO) at binding index 0
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0; // Corresponds to "layout(binding = 0)" in shader
    uboLayoutBinding.descriptorCount = 1; // Only one UBO in this binding
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Not using immutable samplers
    // Specify which shader stage(s) access this descriptor
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // UBO used in the vertex shader

    // --- Descriptor Set Layout Create Info ---
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1; // Number of bindings in this layout
    layoutInfo.pBindings = &uboLayoutBinding;

    // Create the layout object
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
     std::cout << "Descriptor Set Layout Created." << std::endl;

}

/**
 * @brief Creates the Graphics Pipeline (VkPipeline).
 *
 * Defines the entire rendering pipeline state: shaders, vertex input, assembly,
 * rasterization, viewport, depth/stencil testing, color blending, etc.
 * It links together shader modules, pipeline layout, and render pass.
 *
 * Keywords: VkPipeline, vkCreateGraphicsPipelines, Pipeline State Object (PSO), Shader Stages
 */
void VulkanEngine::createGraphicsPipeline() {
    // --- Load Shader Bytecode ---
    auto vertShaderCode = VulkanUtils::readFile("build/shaders/vert.spv");
    auto fragShaderCode = VulkanUtils::readFile("build/shaders/frag.spv");

    // --- Create Shader Modules ---
    VkShaderModule vertShaderModule = VulkanUtils::createShaderModule(device, vertShaderCode);
    VkShaderModule fragShaderModule = VulkanUtils::createShaderModule(device, fragShaderCode);

    // --- Define Shader Stages ---
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main"; // Entry point function name in the shader

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // --- Vertex Input State ---
    // Describes how vertex data is fed into the vertex shader.
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    auto bindingDescription = Vertex::getBindingDescription(); // Get from Vertex struct
    auto attributeDescriptions = Vertex::getAttributeDescriptions(); // Get from Vertex struct
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // --- Input Assembly State ---
    // Describes how vertices are assembled into primitives (e.g., triangles).
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Draw triangles
    inputAssembly.primitiveRestartEnable = VK_FALSE; // Not using special index values to break strips

    // --- Viewport State ---
    // Defines the transformation from clip space to framebuffer coordinates.
    // We use dynamic viewport/scissor, so actual values are set via command buffer.
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1; // One viewport
    viewportState.scissorCount = 1;  // One scissor rectangle

    // --- Rasterization State ---
    // Configures the rasterizer stage (polygon fill mode, culling, etc.).
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE; // Don't clamp depth values
    rasterizer.rasterizerDiscardEnable = VK_FALSE; // Don't discard primitives before rasterization
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // Fill polygons
    rasterizer.lineWidth = 1.0f; // Line width for line rendering modes
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // Cull back-facing triangles
    // Front face definition must match coordinate system & winding order.
    // GLM defaults often match counter-clockwise for front faces in a right-handed system projected correctly.
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE; // Not using depth bias

    // --- Multisample State --- (Disabled)
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // --- Depth/Stencil State ---
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE; // Enable depth testing
    depthStencil.depthWriteEnable = VK_TRUE; // Allow writing to depth buffer
    // Fragments pass if their depth is less than the stored depth (closer to camera)
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE; // Not constraining depth range
    depthStencil.stencilTestEnable = VK_FALSE; // Not using stencil buffer

    // --- Color Blend State --- (Disabled - standard opaque rendering)
    VkPipelineColorBlendAttachmentState colorBlendAttachment{}; // State per attachment
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE; // No blending

    VkPipelineColorBlendStateCreateInfo colorBlending{}; // Global blend state
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE; // Not using logical operations
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment; // State for the single color attachment

    // --- Dynamic State ---
    // Specifies which pipeline states can be changed dynamically via command buffers.
    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // --- Pipeline Layout ---
    // Defines the layout of descriptor sets and push constants used by the pipeline.
    // Must be compatible with the descriptor sets bound during rendering.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1; // Number of descriptor set layouts
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Use the layout created earlier
    // pipelineLayoutInfo.pushConstantRangeCount = 0; // Not using push constants

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        // Cleanup shader modules if layout creation fails
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    // --- Graphics Pipeline Create Info ---
    // Brings all the state objects together.
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2; // Number of shader stages
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil; // Include depth state
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState; // Specify dynamic states
    pipelineInfo.layout = pipelineLayout; // Pipeline layout created above
    pipelineInfo.renderPass = renderPass; // Must be compatible with this render pass
    pipelineInfo.subpass = 0; // Index of the subpass where this pipeline will be used
    // pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Not deriving from another pipeline
    // pipelineInfo.basePipelineIndex = -1;

    // Create the graphics pipeline object
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        // Cleanup layout and shader modules if pipeline creation fails
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    // --- Cleanup ---
    // Shader modules can be destroyed after pipeline creation as they are baked into the pipeline object.
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
     std::cout << "Graphics Pipeline Created." << std::endl;

}

/**
 * @brief Creates the Command Pool (VkCommandPool).
 *
 * Command buffers are allocated from command pools. Pools are associated with a specific queue family.
 *
 * Keywords: VkCommandPool, vkCreateCommandPool, Command Buffer Allocation
 */
void VulkanEngine::createCommandPool() {
    VulkanUtils::QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allows individual command buffers to be reset.
    // Without this, the entire pool would need to be reset.
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    // Specify the queue family this pool will create buffers for (graphics queue).
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }
     std::cout << "Command Pool Created." << std::endl;

}

/**
 * @brief Creates the depth buffer image and image view.
 *
 * Allocates a VkImage and VkImageView suitable for use as a depth/stencil attachment
 * in the render pass. Typically uses optimal tiling and device-local memory.
 *
 * Keywords: Depth Buffer, Z-Buffer, VkImage, VkImageView, Depth Attachment
 */
void VulkanEngine::createDepthResources() {
    VkFormat depthFormat = VulkanUtils::findDepthFormat(physicalDevice);

    // Create the depth image
    VulkanUtils::createImage(physicalDevice, device,
        swapChainExtent.width, swapChainExtent.height,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL, // Optimal tiling for GPU access
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, // Usage as depth/stencil attachment
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // Allocate in GPU-local memory
        depthImage, depthImageMemory);

    // Create the image view for the depth image
    depthImageView = VulkanUtils::createImageView(device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    // Note: Explicit layout transition from UNDEFINED to DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    // using a command buffer is often recommended here for correctness, but the render pass
    // can implicitly handle the transition from UNDEFINED on first use.
     std::cout << "Depth Resources Created (Format: " << depthFormat << ")" << std::endl;
}

/**
 * @brief Creates Framebuffers (VkFramebuffer).
 *
 * Framebuffers link render pass attachments (like color and depth image views)
 * for a specific render pass instance. One framebuffer is created for each swap chain image view.
 *
 * Keywords: VkFramebuffer, vkCreateFramebuffer, Render Target, Attachment Linking
 */
void VulkanEngine::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        // List of attachments for this framebuffer: color view + depth view
        std::array<VkImageView, 2> attachments = {
            swapChainImageViews[i], // Color attachment (from swap chain)
            depthImageView          // Depth attachment (shared)
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        // Must be compatible with the render pass used for rendering
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data(); // Link to the image views
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1; // Number of layers (for array textures)

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
             // Cleanup partially created framebuffers on error
             for(size_t j = 0; j < i; ++j) {
                if(swapChainFramebuffers[j] != VK_NULL_HANDLE) {
                    vkDestroyFramebuffer(device, swapChainFramebuffers[j], nullptr);
                }
             }
             swapChainFramebuffers.clear();
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
     std::cout << "Framebuffers Created." << std::endl;

}

/**
 * @brief Creates the Uniform Buffers (VkBuffer).
 *
 * Creates one UBO for each frame in flight. These buffers are host-visible and
 * persistently mapped for efficient updates from the CPU each frame.
 *
 * Keywords: VkBuffer, Uniform Buffer Object (UBO), Constant Buffer, Host Visible Memory, Persistent Mapping
 */
void VulkanEngine::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VulkanUtils::createBuffer(
            physicalDevice,
            device,
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            uniformBuffers[i],
            uniformBuffersMemory[i]
        );

        // Map the memory once during creation
        vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}

/**
 * @brief Creates the Descriptor Pool (VkDescriptorPool).
 *
 * Descriptor sets are allocated from descriptor pools. The pool defines the maximum
 * number of sets and the maximum number of descriptors of each type (e.g., UBOs) it can allocate.
 *
 * Keywords: VkDescriptorPool, vkCreateDescriptorPool, Descriptor Set Allocation
 */
void VulkanEngine::createDescriptorPool() {
    // Define the types and counts of descriptors the pool should be able to allocate.
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Type of descriptor
    // Number of descriptors of this type (one UBO per frame in flight)
    poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    // --- Descriptor Pool Create Info ---
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1; // Number of pool size structures
    poolInfo.pPoolSizes = &poolSize;
    // Maximum number of descriptor sets that can be allocated from this pool.
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    // Optional flag: VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT allows individual sets to be freed.

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
     std::cout << "Descriptor Pool Created." << std::endl;
}

/**
 * @brief Allocates Descriptor Sets (VkDescriptorSet) and updates them to point to the UBOs.
 *
 * Allocates one descriptor set for each frame in flight from the descriptor pool,
 * using the previously created descriptor set layout. Updates each set to reference
 * the corresponding uniform buffer.
 *
 * Keywords: VkDescriptorSet, vkAllocateDescriptorSets, vkUpdateDescriptorSets, Descriptor Binding
 */
void VulkanEngine::createDescriptorSets() {
    // Need one layout per set to allocate
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

    // --- Descriptor Set Allocation Info ---
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool; // Pool to allocate from
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data(); // Layout for each set

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    // Allocate the descriptor set handles
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    // --- Update each Descriptor Set ---
    // Configure each set to point to the correct uniform buffer for that frame.
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Information about the buffer to bind
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i]; // The UBO for frame 'i'
        bufferInfo.offset = 0;                // Start at the beginning of the buffer
        bufferInfo.range = sizeof(UniformBufferObject); // Size of the UBO data

        // Structure describing the write operation
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];     // The set to update
        descriptorWrite.dstBinding = 0;               // The binding index within the set (matches layout)
        descriptorWrite.dstArrayElement = 0;          // Index within the binding (for array descriptors)
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;          // Number of descriptors to update
        descriptorWrite.pBufferInfo = &bufferInfo;    // Pointer to buffer info
        // descriptorWrite.pImageInfo = nullptr;       // For image descriptors
        // descriptorWrite.pTexelBufferView = nullptr; // For buffer views

        // Perform the update
        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }
     std::cout << "Descriptor Sets Created and Updated." << std::endl;

}

/**
 * @brief Allocates Command Buffers (VkCommandBuffer).
 *
 * Allocates one primary command buffer for each frame in flight from the command pool.
 * These buffers will be recorded each frame with drawing commands.
 *
 * Keywords: VkCommandBuffer, vkAllocateCommandBuffers, Primary Command Buffer
 */
void VulkanEngine::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    // VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted directly to a queue.
    // VK_COMMAND_BUFFER_LEVEL_SECONDARY: Can be executed from primary command buffers.
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
     std::cout << "Command Buffers Allocated." << std::endl;

}

/**
 * @brief Creates Synchronization Primitives (Semaphores and Fences).
 *
 * Creates semaphores to synchronize operations within or across queues (GPU-GPU sync)
 * and fences to synchronize operations between the host (CPU) and a queue (CPU-GPU sync).
 * One set is created for each frame in flight.
 *
 * Keywords: VkSemaphore, VkFence, vkCreateSemaphore, vkCreateFence, Synchronization, GPU-GPU Sync, CPU-GPU Sync
 */
void VulkanEngine::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // Create fences in the signaled state, so the first call to vkWaitForFences in drawFrame doesn't block indefinitely.
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    bool success = true;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
        {
             success = false;
             // Stop creating on failure
             // Cleanup already created objects for this frame index 'i'
             if (inFlightFences[i] != VK_NULL_HANDLE) vkDestroyFence(device, inFlightFences[i], nullptr);
             if (renderFinishedSemaphores[i] != VK_NULL_HANDLE) vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
             if (imageAvailableSemaphores[i] != VK_NULL_HANDLE) vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
             // Cleanup objects created for previous frame indices (0 to i-1)
             for(size_t j = 0; j < i; ++j) {
                vkDestroyFence(device, inFlightFences[j], nullptr);
                vkDestroySemaphore(device, renderFinishedSemaphores[j], nullptr);
                vkDestroySemaphore(device, imageAvailableSemaphores[j], nullptr);
             }
             // Clear vectors to prevent double deletion in main cleanup
            imageAvailableSemaphores.clear();
            renderFinishedSemaphores.clear();
            inFlightFences.clear();
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }
    }
     std::cout << "Synchronization Objects Created." << std::endl;
}


// --- Private Runtime Steps ---

/**
 * @brief Updates the Uniform Buffer for the current frame.
 * @param currentImageIndex Index of the uniform buffer to update (matches current frame in flight).
 * @param scene The scene object providing the current model transform.
 *
 * Calculates Model, View, Projection matrices and copies them into the
 * persistently mapped uniform buffer for the current frame.
 *
 * Keywords: UBO Update, Model View Projection (MVP), glm::lookAt, glm::perspective
 */
void VulkanEngine::updateUniformBuffer(uint32_t currentImageIndex, const Scene& scene) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    // Get the main mesh's transform information
    glm::mat4 model = scene.getMainMeshTransform();
    glm::vec3 position = scene.getMainMeshPosition();
    glm::vec3 rotation = scene.getMainMeshRotation();

    // Create view matrix
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 5.0f),  // Camera position
        glm::vec3(0.0f, 0.0f, 0.0f),  // Look at point
        glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector
    );

    // Create projection matrix
    glm::mat4 proj = glm::perspective(
        glm::radians(45.0f),  // Field of view
        swapChainExtent.width / (float)swapChainExtent.height,  // Aspect ratio
        0.1f,  // Near plane
        10.0f  // Far plane
    );

    // Vulkan's coordinate system has Y pointing down, so we need to flip Y
    proj[1][1] *= -1;

    // Create MVP matrix
    UniformBufferObject ubo{};
    ubo.model = model;
    ubo.view = view;
    ubo.proj = proj;

    // Copy data directly to the already mapped buffer
    memcpy(uniformBuffersMapped[currentImageIndex], &ubo, sizeof(ubo));
}

/**
 * @brief Records drawing commands into the specified command buffer.
 * @param commandBuffer The command buffer to record into.
 * @param imageIndex The index of the swapchain image (and framebuffer) to render to.
 *
 * Begins the render pass, binds the pipeline and descriptor sets, sets dynamic state (viewport/scissor),
 * binds vertex/index buffers, and issues the draw call.
 *
 * Keywords: vkCmdBeginRenderPass, vkCmdBindPipeline, vkCmdBindDescriptorSets, vkCmdBindVertexBuffers, vkCmdBindIndexBuffer, vkCmdDrawIndexed, Command Buffer Recording
 */
void VulkanEngine::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, const Scene& scene) {
    // --- Begin Command Buffer Recording ---
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // beginInfo.flags = 0; // Optional flags
    // beginInfo.pInheritanceInfo = nullptr; // For secondary command buffers

    // Start recording commands into the buffer.
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    // --- Begin Render Pass ---
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass; // The render pass object
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex]; // Target framebuffer
    renderPassInfo.renderArea.offset = {0, 0}; // Area to render within the framebuffer
    renderPassInfo.renderArea.extent = swapChainExtent;

    // Clear values for the attachments (defined in the render pass)
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.2f, 0.2f, 0.3f, 1.0f}}; // Clear color for attachment 0
    clearValues[1].depthStencil = {1.0f, 0}; // Clear depth (1.0 = far plane) for attachment 1
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    // Start the render pass instance. Commands recorded after this are within the pass.
    // VK_SUBPASS_CONTENTS_INLINE: Commands are recorded directly in this primary buffer.
    // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: Primary buffer executes secondary buffers.
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // --- Bind Pipeline ---
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    // --- Set Dynamic State ---
    // Set Viewport
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f; // Standard depth range [0, 1]
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    // Set Scissor Rectangle
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // --- Bind Buffers ---
    // Get the main mesh from the scene
    auto mainMesh = scene.getMainMesh();
    if (mainMesh) {
        // Get the geometry buffer from the mesh
        auto geometryBuffer = dynamic_cast<VulkanGeometryBuffer*>(mainMesh->getBuffer().get());
        if (geometryBuffer) {
            // Bind Vertex Buffer
            VkBuffer vertexBuffers[] = {geometryBuffer->getVertexBuffer()};
            VkDeviceSize offsets[] = {0}; // Starting offset in the buffer
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets); // Bind to binding point 0

            // Bind Index Buffer
            // VK_INDEX_TYPE_UINT32 because our indices vector uses uint32_t
            vkCmdBindIndexBuffer(commandBuffer, geometryBuffer->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

            // --- Bind Descriptor Sets ---
            // Bind the descriptor set for the current frame (containing the updated UBO)
            // Binds set `descriptorSets[currentFrame]` to set index 0 for the graphics pipeline.
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

            // --- Issue Draw Call ---
            // Draw the indexed geometry.
            // indexCount: Number of indices to draw (retrieved from geometry buffer).
            // instanceCount: 1 (not using instancing).
            // firstIndex: 0 (start at the beginning of the index buffer).
            // vertexOffset: 0 (add to vertex index before indexing into vertex buffer).
            // firstInstance: 0 (offset for instanced rendering).
            vkCmdDrawIndexed(commandBuffer, geometryBuffer->getIndexCount(), 1, 0, 0, 0);
        }
    }

    // --- End Render Pass ---
    vkCmdEndRenderPass(commandBuffer);

    // --- End Command Buffer Recording ---
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }
}

/**
 * @brief Cleans up swap chain specific resources.
 *
 * Destroys framebuffers, depth buffer, color image views, and the swapchain itself.
 * Called during main cleanup and before swapchain recreation.
 */
void VulkanEngine::cleanupSwapChain() {
    // Destroy depth resources
    if (depthImageView != VK_NULL_HANDLE) vkDestroyImageView(device, depthImageView, nullptr);
    if (depthImage != VK_NULL_HANDLE) vkDestroyImage(device, depthImage, nullptr);
    if (depthImageMemory != VK_NULL_HANDLE) vkFreeMemory(device, depthImageMemory, nullptr);
    depthImageView = VK_NULL_HANDLE; depthImage = VK_NULL_HANDLE; depthImageMemory = VK_NULL_HANDLE;

    // Destroy framebuffers
    for (auto framebuffer : swapChainFramebuffers) {
        if (framebuffer != VK_NULL_HANDLE) vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    swapChainFramebuffers.clear();

    // Destroy color image views
    for (auto imageView : swapChainImageViews) {
         if (imageView != VK_NULL_HANDLE) vkDestroyImageView(device, imageView, nullptr);
    }
    swapChainImageViews.clear();

    // Destroy swapchain
    if (swapChain != VK_NULL_HANDLE) vkDestroySwapchainKHR(device, swapChain, nullptr);
    swapChain = VK_NULL_HANDLE;
}


// --- Private Helper Function Implementations ---
// (These are closely tied to the engine's state and logic)

/**
 * @brief Checks if a physical device is suitable for the application's needs.
 * @param queryDevice The VkPhysicalDevice handle to check.
 * @return true if the device is suitable, false otherwise.
 *
 * Verifies required queue families, extensions, and swap chain support.
 * Can be extended to check for specific features (e.g., geometry shaders, anisotropic filtering).
 */
bool VulkanEngine::isDeviceSuitable(VkPhysicalDevice queryDevice) {
    VulkanUtils::QueueFamilyIndices indices = findQueueFamilies(queryDevice);
    bool extensionsSupported = checkDeviceExtensionSupport(queryDevice);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        VulkanUtils::SwapChainSupportDetails swapChainSupport = querySwapChainSupport(queryDevice);
        // Basic check: Ensure at least one format and present mode are supported for the surface.
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    // Optionally check for required features:
    // VkPhysicalDeviceFeatures supportedFeatures;
    // vkGetPhysicalDeviceFeatures(queryDevice, &supportedFeatures);
    // bool featuresSupported = supportedFeatures.samplerAnisotropy; // Example

    return indices.isComplete() && extensionsSupported && swapChainAdequate; // && featuresSupported;
}

/**
 * @brief Checks if a physical device supports all required device extensions.
 * @param queryDevice The VkPhysicalDevice handle to check.
 * @return true if all required extensions are supported, false otherwise.
 */
bool VulkanEngine::checkDeviceExtensionSupport(VkPhysicalDevice queryDevice) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(queryDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(queryDevice, nullptr, &extensionCount, availableExtensions.data());

    // Create a set of required extensions for easy lookup
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    // Remove found extensions from the required set
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    // If the set is empty, all required extensions were found
    return requiredExtensions.empty();
}

/**
 * @brief Finds indices of queue families supporting graphics and presentation.
 * @param queryDevice The VkPhysicalDevice handle to check.
 * @return VulkanUtils::QueueFamilyIndices struct containing optional indices.
 */
VulkanUtils::QueueFamilyIndices VulkanEngine::findQueueFamilies(VkPhysicalDevice queryDevice) {
    VulkanUtils::QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(queryDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(queryDevice, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        // Check for graphics support
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        // Check for presentation support to the created surface
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(queryDevice, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        // Early exit if all found
        if (indices.isComplete()) {
            break;
        }
        i++;
    }
    return indices;
}

/**
 * @brief Queries swap chain support details for a physical device and surface.
 * @param queryDevice The VkPhysicalDevice handle to check.
 * @return VulkanUtils::SwapChainSupportDetails struct containing capabilities, formats, and present modes.
 */
VulkanUtils::SwapChainSupportDetails VulkanEngine::querySwapChainSupport(VkPhysicalDevice queryDevice) {
    VulkanUtils::SwapChainSupportDetails details;
    // Get surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(queryDevice, surface, &details.capabilities);

    // Get supported surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(queryDevice, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(queryDevice, surface, &formatCount, details.formats.data());
    }

    // Get supported presentation modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(queryDevice, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(queryDevice, surface, &presentModeCount, details.presentModes.data());
    }
    return details;
}

/**
 * @brief Chooses the best available surface format for the swap chain.
 * @param availableFormats Vector of supported VkSurfaceFormatKHR.
 * @return The preferred VkSurfaceFormatKHR (typically B8G8R8A8_SRGB).
 *
 * Prefers a common SRGB format for better color representation. Falls back to the first available format.
 */
VkSurfaceFormatKHR VulkanEngine::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    // Return the first format if the preferred one isn't available
    return availableFormats[0];
}

/**
 * @brief Chooses the best available presentation mode for the swap chain.
 * @param availablePresentModes Vector of supported VkPresentModeKHR.
 * @return The preferred VkPresentModeKHR (typically MAILBOX for low latency, FIFO as fallback).
 *
 * - MAILBOX: Triple buffering. Non-blocking presentation, renders newest frame. Good for low latency.
 * - FIFO: Standard vsync. Blocks if queue is full. Prevents tearing. Guaranteed availability.
 * - FIFO_RELAXED: Like FIFO, but presents immediately if late, may cause tearing.
 * - IMMEDIATE: Presents immediately, may cause tearing. Not always available.
 */
VkPresentModeKHR VulkanEngine::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode; // Prefer mailbox if available
        }
    }
    // FIFO is always guaranteed to be available
    return VK_PRESENT_MODE_FIFO_KHR;
}

/**
 * @brief Chooses the swap chain image resolution (extent).
 * @param capabilities Surface capabilities struct.
 * @return VkExtent2D representing the desired width and height.
 *
 * Uses the current window size from GLFW if the surface allows variable extents,
 * otherwise uses the fixed extent provided by the surface capabilities. Clamps
 * the chosen size within the min/max limits reported by the device.
 */
VkExtent2D VulkanEngine::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    // If currentExtent is not max uint32_t, the size is fixed.
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        // Get the window size in pixels from GLFW.
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        // Clamp the size between the min and max extents supported by the surface.
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

/**
 * @brief Gets the list of required Vulkan instance extensions.
 * @return Vector of const char* containing extension names.
 *
 * Includes extensions required by GLFW for window system integration and,
 * if validation layers are enabled, the debug utils extension.
 */
std::vector<const char*> VulkanEngine::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount); // Get GLFW extensions

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    // Add debug utils extension if validation layers are enabled
    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

/**
 * @brief Checks if all requested validation layers are available.
 * @return true if all layers are supported, false otherwise.
 */
bool VulkanEngine::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr); // Get count
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()); // Get properties

    // Check if each requested layer exists in the available layers
    for (const char* layerName : validationLayers) {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            std::cerr << "Validation layer not found: " << layerName << std::endl;
            return false;
        }
    }
    return true;
}


// --- Static Debug Callback Implementation ---

/**
 * @brief Static callback function passed to the Vulkan debug messenger.
 * @param messageSeverity Severity level of the message (Verbose, Info, Warning, Error).
 * @param messageType Type of message (General, Validation, Performance).
 * @param pCallbackData Pointer to struct containing message details.
 * @param pUserData Optional user data pointer (not used here).
 * @return VK_FALSE indicating the Vulkan call triggering the message should not be aborted.
 *
 * Logs validation layer messages to std::cerr.
 */
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanEngine::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) { // pUserData could potentially point to the VulkanEngine instance if needed

    // Filter message severity (optional)
    // if (messageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    //    return VK_FALSE;
    // }

    std::cerr << "Validation layer: ";

    // Add severity prefix
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        std::cerr << "[ERROR] ";
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "[WARNING] ";
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        std::cerr << "[INFO] ";
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
         std::cerr << "[VERBOSE] ";
    }

     // Add type prefix (optional)
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
         std::cerr << "[General] ";
    } else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
         std::cerr << "[Validation] ";
    } else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
         std::cerr << "[Performance] ";
    }

    // Print the message
    std::cerr << pCallbackData->pMessage << std::endl;

    // Optional: Print details like object handles involved
    // if (pCallbackData->objectCount > 0) {
    //     std::cerr << "  Objects involved:" << std::endl;
    //     for (uint32_t i = 0; i < pCallbackData->objectCount; ++i) {
    //         std::cerr << "    - Object " << i << ": Type=" << pCallbackData->pObjects[i].objectType
    //                   << ", Handle=0x" << std::hex << pCallbackData->pObjects[i].objectHandle << std::dec;
    //         if(pCallbackData->pObjects[i].pObjectName) {
    //              std::cerr << ", Name=\"" << pCallbackData->pObjects[i].pObjectName << "\"";
    //         }
    //          std::cerr << std::endl;
    //     }
    // }


    // Must return VK_FALSE. Returning VK_TRUE would abort the Vulkan call that triggered the callback.
    return VK_FALSE;
}

} // namespace graphics