#include "Window.h"
#include <iostream>

Window::Window(const std::string& title, const std::string& configPath)
    : title(title), configPath(configPath) {}

Window::~Window() {
    if (window) {
        glfwDestroyWindow(window);
    }
}

GLFWwindow* Window::init() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Load window config
    Config config = loadConfig();

    // Create window with config
    window = glfwCreateWindow(config.width, config.height, title.c_str(), nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window!");
    }

    // Set window position and state
    glfwSetWindowPos(window, config.x, config.y);
    if (config.maximized) {
        glfwMaximizeWindow(window);
    }

    // Set window close callback to save config
    glfwSetWindowCloseCallback(window, [](GLFWwindow* window) {
        auto userPointer = glfwGetWindowUserPointer(window);
        if (userPointer) {
            auto windowInstance = static_cast<Window*>(userPointer);
            windowInstance->saveConfig();
        }
    });

    // Set key callback for ESC key
    glfwSetKeyCallback(window, keyCallback);

    // Store this instance in window's user pointer for callbacks
    glfwSetWindowUserPointer(window, this);

    return window;
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        std::cout << "ESC key pressed. Closing window..." << std::endl;
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

Window::Config Window::getCurrentConfig() const {
    Config config;
    glfwGetWindowPos(window, &config.x, &config.y);
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    config.width = width;
    config.height = height;
    config.maximized = glfwGetWindowAttrib(window, GLFW_MAXIMIZED);
    return config;
}

void Window::saveConfig() const {
    Config config = getCurrentConfig();
    nlohmann::json jsonConfig;
    jsonConfig["x"] = config.x;
    jsonConfig["y"] = config.y;
    jsonConfig["width"] = config.width;
    jsonConfig["height"] = config.height;
    jsonConfig["maximized"] = config.maximized;

    std::ofstream file(configPath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open window config file for writing: " + configPath);
    }
    file << jsonConfig.dump(4); // Pretty print with 4-space indentation
}

Window::Config Window::loadConfig() const {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        return Config(); // Return default config if file doesn't exist
    }

    nlohmann::json config;
    file >> config;

    Config windowConfig;
    windowConfig.x = config["x"].get<int>();
    windowConfig.y = config["y"].get<int>();
    windowConfig.width = config["width"].get<int>();
    windowConfig.height = config["height"].get<int>();
    windowConfig.maximized = config["maximized"].get<bool>();

    return windowConfig;
} 