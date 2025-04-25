#pragma once

#include <GLFW/glfw3.h>
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>

/**
 * @brief Handles window creation, management, and configuration persistence.
 * 
 * This class manages the GLFW window lifecycle, including creation, configuration,
 * and state persistence. It handles window position, size, and maximized state
 * through JSON configuration files.
 */
class Window {
public:
    /**
     * @brief Window configuration structure
     */
    struct Config {
        int x = 100;
        int y = 100;
        int width = 800;
        int height = 600;
        bool maximized = false;
    };

    /**
     * @brief Constructor
     * @param title Window title
     * @param configPath Path to window configuration file
     */
    Window(const std::string& title, const std::string& configPath = "build/window_config.json");

    /**
     * @brief Destructor
     */
    ~Window();

    /**
     * @brief Initialize the window
     * @return Pointer to the created GLFW window
     */
    GLFWwindow* init();

    /**
     * @brief Get the current window configuration
     * @return Current window configuration
     */
    Config getCurrentConfig() const;

    /**
     * @brief Save current window configuration to file
     */
    void saveConfig() const;

    /**
     * @brief Load window configuration from file
     * @return Loaded window configuration
     */
    Config loadConfig() const;

    /**
     * @brief Get the GLFW window handle
     * @return Pointer to the GLFW window
     */
    GLFWwindow* getHandle() const { return window; }

private:
    GLFWwindow* window = nullptr;
    std::string title;
    std::string configPath;

    // Key callback for handling ESC key
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
}; 