#include "Object.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <algorithm>  // For std::find

Object::Object(const std::string& name) 
    : name(name) {
    updateMatrix();
}

void Object::addChild(std::shared_ptr<Object> child) {
    if (child) {
        children.push_back(child);
        child->parent = weak_from_this();
    }
}

void Object::removeChild(std::shared_ptr<Object> child) {
    if (child) {
        auto it = std::find(children.begin(), children.end(), child);
        if (it != children.end()) {
            children.erase(it);
            child->parent.reset();
        }
    }
}

void Object::updateMatrix() {
    if (!matrixNeedsUpdate) return;

    // Create translation matrix
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);

    // Create rotation matrix from euler angles (in radians)
    // Order: YXZ (yaw, pitch, roll)
    glm::mat4 rotationMatrix = glm::eulerAngleYXZ(rotation.y, rotation.x, rotation.z);

    // Create scale matrix
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

    // Combine matrices: T * R * S
    matrix = translation * rotationMatrix * scaleMatrix;

    // Update world matrix if we have a parent
    if (auto parentPtr = parent.lock()) {
        matrixWorld = parentPtr->getMatrixWorld() * matrix;
    } else {
        matrixWorld = matrix;
    }

    // Update children
    for (auto& child : children) {
        child->matrixNeedsUpdate = true;
        child->updateMatrix();
    }

    matrixNeedsUpdate = false;
} 