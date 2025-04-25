#include "Object3D.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <algorithm>  // For std::find

Object3D::Object3D(const std::string& name) 
    : name(name) {
}

void Object3D::addChild(std::shared_ptr<Object3D> child) {
    // Remove from current parent if any
    if (auto currentParent = child->getParent()) {
        currentParent->removeChild(child);
    }
    
    // Add to this object's children
    children.push_back(child);
    child->parent = shared_from_this();
    
    // Mark child's world matrix as needing update
    child->matrixNeedsUpdate = true;
}

void Object3D::removeChild(std::shared_ptr<Object3D> child) {
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end()) {
        children.erase(it);
        child->parent.reset();
        child->matrixNeedsUpdate = true;
    }
}

void Object3D::updateMatrix() {
    if (matrixNeedsUpdate) {
        // Call before update hook
        onBeforeUpdate();
        
        // Update local matrix
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
        
        // Create rotation matrix from Euler angles
        glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 rotation = rotationZ * rotationY * rotationX;
        
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
        
        matrix = translation * rotation * scaleMatrix;
        
        // Update world matrix
        if (auto p = parent.lock()) {
            matrixWorld = p->getMatrixWorld() * matrix;
        } else {
            matrixWorld = matrix;
        }
        
        // Mark as updated
        matrixNeedsUpdate = false;
        
        // Update all children
        for (auto& child : children) {
            child->matrixNeedsUpdate = true;
            child->updateMatrix();
        }
        
        // Call after update hook
        onAfterUpdate();
    }
} 