// Standard Library Includes
#include <iostream>
#include <stdexcept>
#include <cstdlib> // For EXIT_SUCCESS, EXIT_FAILURE
#include <chrono>  // For delta time calculation

// Third-party Libraries (assumed to be in include paths)
#define GLFW_INCLUDE_VULKAN // Makes GLFW include Vulkan headers
#include <GLFW/glfw3.h>     // For windowing and input

// Project Includes
#include "VulkanEngine/VulkanEngine.h" // The core Vulkan logic wrapper
#include "Scene/Scene.h"              // The scene logic and data


// --- Constants ---
const uint32_t INITIAL_WIDTH = 800;
const uint32_t INITIAL_HEIGHT = 600;
const std::string APP_NAME = "Bouncing Ball (Refactored)";


/**
 * @brief Main application class orchestrating the window, engine, and scene.
 *
 * Sets up GLFW, creates the Vulkan engine and scene, and runs the main loop.
 * Handles window events and coordinates updates and rendering.
 */
class Application {
public:
    /**
     * @brief Runs the main application lifecycle.
     *
     * Initializes components, enters the main loop, and performs cleanup on exit.
     */
    void run() {
        initWindow();       // Setup GLFW window
        initScene();        // Initialize scene data (geometry, physics state)
        initVulkan();       // Initialize Vulkan engine using window and scene data
        mainLoop();         // Enter the main update/render loop
        cleanup();          // Release resources
    }

private:
    GLFWwindow* window = nullptr;         // GLFW window handle
    VulkanEngine* vulkanEngine = nullptr; // Pointer to the Vulkan engine instance
    Scene scene;                          // The scene object instance

    // Timing for delta time calculation
    std::chrono::high_resolution_clock::time_point lastFrameTime;

    /**
     * @brief Initializes the GLFW window.
     *
     * Configures window hints and creates the main application window.
     * Sets up user pointers and callbacks for resizing.
     */
    void initWindow() {
        glfwInit();                                   // Initialize GLFW library
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Tell GLFW *not* to create an OpenGL context
        //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Optional: Disable resizing

        window = glfwCreateWindow(INITIAL_WIDTH, INITIAL_HEIGHT, APP_NAME.c_str(), nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window!");
        }

        // Store 'this' pointer in the window's user pointer to access Application members in callbacks
        glfwSetWindowUserPointer(window, this);
        // Set the framebuffer resize callback function
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

        // Initialize frame timer
        lastFrameTime = std::chrono::high_resolution_clock::now();
         std::cout << "GLFW Window Initialized." << std::endl;

    }

    /**
     * @brief Initializes the scene logic and data.
     */
    void initScene() {
        scene.init(); // Generate sphere, set initial physics state
         std::cout << "Scene Initialized." << std::endl;
    }

    /**
     * @brief Initializes the Vulkan rendering engine.
     *
     * Creates the VulkanEngine instance, passing the GLFW window handle.
     * Calls the engine's main initialization function, providing scene data if needed
     * (e.g., for creating initial vertex/index buffers).
     */
    void initVulkan() {
        vulkanEngine = new VulkanEngine(window); // Create engine instance
        vulkanEngine->initVulkan(scene);         // Initialize Vulkan, passing scene data
    }

    /**
     * @brief Runs the main application loop.
     *
     * Continuously processes window events, updates scene logic (physics),
     * and tells the Vulkan engine to render a frame until the window is closed.
     */
    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents(); // Check for and process window events (input, resize, close)

            // Calculate delta time for physics and animations
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();
            lastFrameTime = currentTime;

            // Clamp delta time to prevent large simulation steps if frame rate drops drastically
             deltaTime = std::min(deltaTime, 0.1f); // e.g., max 100ms step

            // Update scene logic (e.g., physics simulation)
            scene.update(deltaTime);

            // Render the frame using the Vulkan engine, passing the current scene state
            if (vulkanEngine) {
                 try {
                     vulkanEngine->drawFrame(scene);
                 } catch (const std::exception& e) {
                    // Handle potential Vulkan runtime errors during rendering (e.g., device lost)
                     std::cerr << "Error during drawFrame: " << e.what() << std::endl;
                     // Depending on the error, might try to recover or just exit
                     break; // Exit loop on draw error
                 }
            }
        }
    }

    /**
     * @brief Cleans up application resources.
     *
     * Destroys the Vulkan engine instance (which handles its own internal cleanup)
     * and terminates GLFW.
     */
    void cleanup() {
         std::cout << "Starting Application Cleanup..." << std::endl;
        // Vulkan engine cleanup is crucial and should happen before window destruction
        if (vulkanEngine) {
            // The VulkanEngine destructor calls vkDeviceWaitIdle and its cleanup method
            delete vulkanEngine;
            vulkanEngine = nullptr;
        }

        // Destroy the GLFW window
        if (window) {
            glfwDestroyWindow(window);
            window = nullptr;
        }

        // Terminate GLFW library
        glfwTerminate();
         std::cout << "Application Cleanup Complete." << std::endl;

    }

    /**
     * @brief Static callback function for GLFW framebuffer resize events.
     * @param window The window that was resized.
     * @param width The new width in pixels.
     * @param height The new height in pixels.
     *
     * Retrieves the Application instance from the window's user pointer and notifies
     * the Vulkan engine that a resize occurred.
     */
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        // Retrieve the Application instance associated with this window
        auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        if (app && app->vulkanEngine) {
            // Notify the engine about the resize event
            app->vulkanEngine->notifyFramebufferResized();
        }
    }
};


// --- Entry Point ---
int main() {
    Application app; // Create the application instance

    try {
        app.run(); // Run the application lifecycle
    } catch (const std::exception& e) {
        // Catch and report any exceptions thrown during initialization or runtime
        std::cerr << "Unhandled Exception: " << e.what() << std::endl;

        // Pause console on error in Windows debug builds to see the message
        #if defined(_WIN32) && !defined(NDEBUG)
            system("pause");
        #endif

        return EXIT_FAILURE; // Indicate failure
    }

    return EXIT_SUCCESS; // Indicate successful execution
}