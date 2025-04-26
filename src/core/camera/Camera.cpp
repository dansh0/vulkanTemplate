#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera() 
    : position(0.0f, 0.0f, 5.0f)
    , target(0.0f, 0.0f, 0.0f)
    , up(0.0f, 1.0f, 0.0f)
    , fov(45.0f)
    , aspectRatio(16.0f / 9.0f)
    , nearPlane(0.1f)
    , farPlane(100.0f)
{
    updateViewMatrix();
    updateProjectionMatrix();
}

Camera::~Camera() {}

void Camera::setPosition(const glm::vec3& pos) {
    position = pos;
    updateViewMatrix();
}

void Camera::setTarget(const glm::vec3& tgt) {
    target = tgt;
    updateViewMatrix();
}

void Camera::setUp(const glm::vec3& u) {
    up = u;
    updateViewMatrix();
}

void Camera::setFOV(float f) {
    fov = f;
    updateProjectionMatrix();
}

void Camera::setAspectRatio(float aspect) {
    aspectRatio = aspect;
    updateProjectionMatrix();
}

void Camera::setNearFar(float near, float far) {
    nearPlane = near;
    farPlane = far;
    updateProjectionMatrix();
}

const glm::mat4& Camera::getViewMatrix() const {
    return viewMatrix;
}

const glm::mat4& Camera::getProjectionMatrix() const {
    return projectionMatrix;
}

const glm::mat4& Camera::getViewProjectionMatrix() const {
    return viewProjectionMatrix;
}

void Camera::move(const glm::vec3& offset) {
    position += offset;
    target += offset;
    updateViewMatrix();
}

void Camera::rotate(float yaw, float pitch) {
    // TODO: Implement camera rotation
    updateViewMatrix();
}

void Camera::lookAt(const glm::vec3& tgt) {
    target = tgt;
    updateViewMatrix();
}

void Camera::updateViewMatrix() {
    viewMatrix = glm::lookAt(position, target, up);
    viewProjectionMatrix = projectionMatrix * viewMatrix;
}

void Camera::updateProjectionMatrix() {
    projectionMatrix = glm::perspective(
        glm::radians(fov),
        aspectRatio,
        nearPlane,
        farPlane
    );
    viewProjectionMatrix = projectionMatrix * viewMatrix;
} 