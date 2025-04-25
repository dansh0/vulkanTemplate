#pragma once

#include "../Geometry/Mesh.h" // Include Vertex definition
#include <glm/glm.hpp>
#include <vector>
#include <cstdint> // For uint32_t

/**
 * @brief Manages the scene objects, physics, and geometry data.
 *
 * This class is responsible for:
 * - Holding the geometry data (vertices, indices) for scene objects (currently just the ball).
 * - Storing and updating the physics state of objects (position, velocity).
 * - Defining the boundaries of the scene.
 * - Providing methods to initialize, update, and retrieve data needed for rendering.
 *
 * Keywords: Scene Management, Game State, Physics Simulation, Geometry Storage
 */
class Scene {
public:
    /**
     * @brief Initializes the scene, generating geometry and setting initial physics state.
     *
     * Should be called once after the Scene object is created.
     */
    void init();

    /**
     * @brief Updates the physics state of the scene based on elapsed time.
     * @param deltaTime The time elapsed since the last update, in seconds.
     *
     * This method moves objects, checks for collisions, and applies effects like gravity (optional).
     */
    void update(float deltaTime);

    /**
     * @brief Gets the current position of the bouncing ball.
     * @return glm::vec3 representing the ball's center position.
     */
    glm::vec3 getBallPosition() const;

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
    std::vector<Vertex> vertices;   // Vertex data for the ball mesh
    std::vector<uint32_t> indices;  // Index data for the ball mesh

    // --- Physics State ---
    glm::vec3 ballPosition = glm::vec3(0.0f, 0.0f, 0.0f);       // Current position
    glm::vec3 ballVelocity = glm::vec3(1.5f, 2.5f, -1.8f);      // Current velocity (m/s or units/s)
    float ballRadius = 0.5f;                                  // Radius used for collision
    glm::vec3 roomBounds = glm::vec3(5.0f, 4.0f, 5.0f);       // Half-extents (center to wall distance)
    float restitution = 0.78f;                                // Coefficient of restitution (bounciness)

    /**
     * @brief Updates the ball's physics state, including position and velocity based on collisions.
     * @param deltaTime Time step for the physics update.
     *
     * Internal helper function called by update().
     */
    void updatePhysics(float deltaTime);
};