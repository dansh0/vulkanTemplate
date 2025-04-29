#pragma once

#include "../objects/mesh/Mesh.h" // Include Vertex definition
#include "../objects/loaders/ObjLoader.h" // Include OBJ loader
#include <glm/glm.hpp>
#include <vector>
#include <cstdint> // For uint32_t
#include <string>

/**
 * @brief Manages the scene objects, physics, and geometry data.
 *
 * This class is responsible for:
 * - Loading and managing 3D models from OBJ files
 * - Storing and updating the physics state of objects (position, velocity)
 * - Defining the boundaries of the scene
 * - Providing methods to initialize, update, and retrieve data needed for rendering
 *
 * Keywords: Scene Management, Game State, Physics Simulation, Model Loading
 */
class Scene {
public:
    /**
     * @brief Initializes the scene, loading models and setting initial physics state.
     * @param modelPath Path to the OBJ file to load
     * @param scale Scale factor for the model
     * Should be called once after the Scene object is created.
     */
    void init(const std::string& modelPath, const float scale);

    /**
     * @brief Updates the physics state of the scene based on elapsed time.
     * @param deltaTime The time elapsed since the last update, in seconds.
     *
     * This method moves objects, checks for collisions, and applies effects like gravity (optional).
     */
    void update(float deltaTime);

    /**
     * @brief Cleans up the scene.
     *
     * Keywords: Scene Cleanup, Resource Release
     */
    void cleanup();

    /**
     * @brief Gets the current position of the bouncing object.
     * @return glm::vec3 representing the object's center position.
     */
    glm::vec3 getObjPosition() const;

    /**
     * @brief Gets the current rotation of the bouncing object.
     * @return glm::vec3 representing the object's rotation.
     */
    glm::vec3 getObjRotation() const;

    /**
     * @brief Provides direct access to the vertex data for rendering.
     * @return Const reference to the vector of vertices.
     *
     * The VulkanEngine will use this to create and populate the vertex buffer.
     */
    const std::vector<Vertex>& getVertices() const;

    /**
     * @brief Provides direct access to the index data for rendering.
     * @return Const reference to the vector of indices.
     *
     * The VulkanEngine will use this to create and populate the index buffer.
     */
    const std::vector<uint32_t>& getIndices() const;

private:
    // --- Geometry Data ---
    std::vector<Vertex> vertices;   // Vertex data for the model
    std::vector<uint32_t> indices;  // Index data for the model

    // --- Physics State ---
    glm::vec3 objPosition = glm::vec3(0.0f, 0.0f, 0.0f);       // Current position
    glm::vec3 objVelocity = glm::vec3(1.5f, 2.5f, -1.8f);      // Current velocity (m/s or units/s)
    glm::vec3 objRotation = glm::vec3(0.0f, 0.0f, 0.0f);      // Current rotation
    glm::vec3 objRotationVelocity = glm::vec3(0.0f, 0.0f, 0.0f);      // Current rotation velocity
    float objRadius = 0.5f;                                  // Radius used for collision
    glm::vec3 roomBounds = glm::vec3(5.0f, 4.0f, 5.0f);       // Half-extents (center to wall distance)
    float restitution = 0.78f;                                // Coefficient of restitution (bounciness)

    /**
     * @brief Updates the object's physics state, including position and velocity based on collisions.
     * @param deltaTime Time step for the physics update.
     *
     * Internal helper function called by update().
     */
    void updatePhysics(float deltaTime);
};