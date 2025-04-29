#include "Scene.h"
#include <iostream>

/**
 * @brief Initializes the scene. Generates the sphere mesh and sets the initial radius.
 *
 * Keywords: Scene Initialization, Mesh Generation
 */
void Scene::init(const std::string& modelPath, const float scale) {
    // Load the model from OBJ file
    if (!ObjLoader::loadObj(modelPath, scale, vertices, indices)) {
        std::cerr << "Failed to load model: " << modelPath << std::endl;
        // You might want to handle this error more gracefully
        throw std::runtime_error("Failed to load model");
    }

    // Initialize physics state
    objPosition = glm::vec3(0.0f, -4.0f, 0.0f);
    objVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
    objRotation = glm::vec3(0.0f, 0.0f, 0.0f);
    objRotationVelocity = glm::vec3(0.0f, 0.5f, 0.0f);
}

/**
 * @brief Updates the scene state, primarily by calling the physics update.
 * @param deltaTime The time elapsed since the last frame in seconds.
 *
 * Keywords: Scene Update, Game Loop Update, Physics Step
 */
void Scene::update(float deltaTime) {
    // Update the physics simulation for the obj
    updatePhysics(deltaTime);
}

/**
 * @brief Updates the obj's position and handles collisions with the room boundaries.
 * @param deltaTime The time step for the physics update.
 *
 * Implements simple Euler integration for position and reflects velocity upon collision.
 *
 * Keywords: Physics Update, Collision Detection, Axis-Aligned Bounding Box (AABB), Restitution
 */
void Scene::updatePhysics(float deltaTime) {
    // --- Update Position ---
    // Basic Euler integration: new_position = old_position + velocity * time_step
    objPosition += objVelocity * deltaTime;

    // Check for collisions with room boundaries
    for (int i = 0; i < 3; i++) {
        // Check if we've hit a wall
        if (objPosition[i] > roomBounds[i] - objRadius) {
            objPosition[i] = roomBounds[i] - objRadius;
            objVelocity[i] = -objVelocity[i] * restitution;
        } else if (objPosition[i] < -roomBounds[i] + objRadius) {
            objPosition[i] = -roomBounds[i] + objRadius;
            objVelocity[i] = -objVelocity[i] * restitution;
        }
    }

    // Rotate
    objRotation += objRotationVelocity * deltaTime;

    // --- Optional: Apply Gravity ---
    // Uncomment to add a constant downward acceleration (adjust strength as needed).
    // Assumes Y is the vertical axis.
    // float gravity = 9.81f;
    // objVelocity.y -= gravity * deltaTime * 0.2f; // Apply gravity (scaled down for effect)
}

/**
 * @brief Cleans up the scene.
 *
 * Keywords: Scene Cleanup, Resource Release
 */
void Scene::cleanup() {
    // TODO: Implement cleanup
}


// --- Accessors ---

/**
 * @brief Gets the current object position.
 * @return objects's center position.
 */
glm::vec3 Scene::getObjPosition() const {
    return objPosition;
}

/**
 * @brief Gets the current object rotation.
 * @return object's rotation.
 */
glm::vec3 Scene::getObjRotation() const {
    return objRotation;
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