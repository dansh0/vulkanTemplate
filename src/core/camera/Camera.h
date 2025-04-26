#pragma once

#include <glm/glm.hpp>
#include <memory>

/**
 * @brief Camera class for managing view and projection matrices
 * 
 * The Camera class handles the view and projection transformations
 * needed for 3D rendering. It supports perspective projection and
 * provides methods for camera movement and orientation.
 */
class Camera {
public:
    Camera();
    ~Camera();

    // --- Camera Properties ---
    void setPosition(const glm::vec3& position);
    void setTarget(const glm::vec3& target);
    void setUp(const glm::vec3& up);
    void setFOV(float fov);
    void setAspectRatio(float aspect);
    void setNearFar(float near, float far);

    // --- Matrix Access ---
    const glm::mat4& getViewMatrix() const;
    const glm::mat4& getProjectionMatrix() const;
    const glm::mat4& getViewProjectionMatrix() const;

    // --- Camera Movement ---
    void move(const glm::vec3& offset);
    void rotate(float yaw, float pitch);
    void lookAt(const glm::vec3& target);

private:
    void updateViewMatrix();
    void updateProjectionMatrix();

    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;

    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::mat4 viewProjectionMatrix;
}; 