#include "Scene.h"
#include "../Geometry/Sphere.h" // Include the sphere generation function
#include <algorithm>           // For std::min

/**
 * @brief Initializes the scene. Generates the sphere mesh and sets the initial radius.
 *
 * Keywords: Scene Initialization, Mesh Generation
 */
void Scene::init() {
    // --- Generate Sphere Mesh ---
    float sphereRadius = 0.5f;
    int sphereSectors = 24;
    int sphereStacks = 12;
    // IMPORTANT: Update the physics radius to match the generated geometry
    ballRadius = sphereRadius;
    // Generate the mesh data directly into the Scene's member variables
    generateSphere(sphereRadius, sphereSectors, sphereStacks, vertices, indices);
}

/**
 * @brief Updates the scene state, primarily by calling the physics update.
 * @param deltaTime The time elapsed since the last frame in seconds.
 *
 * Keywords: Scene Update, Game Loop Update, Physics Step
 */
void Scene::update(float deltaTime) {
    // Update the physics simulation for the ball
    updatePhysics(deltaTime);
}

/**
 * @brief Updates the ball's position and handles collisions with the room boundaries.
 * @param deltaTime The time step for the physics update.
 *
 * Implements simple Euler integration for position and reflects velocity upon collision.
 *
 * Keywords: Physics Update, Collision Detection, Axis-Aligned Bounding Box (AABB), Restitution
 */
void Scene::updatePhysics(float deltaTime) {
    // --- Update Position ---
    // Basic Euler integration: new_position = old_position + velocity * time_step
    ballPosition += ballVelocity * deltaTime;

    // --- Collision Detection & Response ---
    // Check collision against each axis (X, Y, Z) of the room boundaries
    for (int i = 0; i < 3; ++i) { // Iterate axes 0=x, 1=y, 2=z

        // Check collision with the positive wall (e.g., +X wall)
        if (ballPosition[i] + ballRadius > roomBounds[i]) {
            // 1. Correct position: Place ball exactly at the boundary
            ballPosition[i] = roomBounds[i] - ballRadius;
            // 2. Reflect velocity: Reverse the velocity component along this axis
            //    and dampen it by the coefficient of restitution.
            ballVelocity[i] *= -restitution;
        }
        // Check collision with the negative wall (e.g., -X wall)
        else if (ballPosition[i] - ballRadius < -roomBounds[i]) {
            // 1. Correct position: Place ball exactly at the boundary
            ballPosition[i] = -roomBounds[i] + ballRadius;
            // 2. Reflect velocity: Reverse and dampen.
            ballVelocity[i] *= -restitution;
        }
    }

    // --- Optional: Apply Gravity ---
    // Uncomment to add a constant downward acceleration (adjust strength as needed).
    // Assumes Y is the vertical axis.
    // float gravity = 9.81f;
    // ballVelocity.y -= gravity * deltaTime * 0.2f; // Apply gravity (scaled down for effect)
}


// --- Accessors ---

/**
 * @brief Gets the current ball position.
 * @return Ball's center position.
 */
glm::vec3 Scene::getBallPosition() const {
    return ballPosition;
}

/**
 * @brief Gets the vertex data.
 * @return Const reference to the vertex vector.
 */
const std::vector<Vertex>& Scene::getVertices() const {
    return vertices;
}

/**
 * @brief Gets the index data.
 * @return Const reference to the index vector.
 */
const std::vector<uint32_t>& Scene::getIndices() const {
    return indices;
}