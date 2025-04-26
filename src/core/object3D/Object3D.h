#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <string>
#include <vector>
#include <memory>

/**
 * @brief Base class for all 3D objects in the scene
 * 
 * This class provides the fundamental properties and methods that all 3D objects
 * need, including transform hierarchy, visibility, and basic update functionality.
 */
class Object3D : public std::enable_shared_from_this<Object3D> {
public:
    Object3D(const std::string& name = "");
    virtual ~Object3D() = default;

    // --- Transform Properties ---
    /**
     * @brief Gets the position of the object in local space
     * @return The position as a vec3
     */
    glm::vec3 getPosition() const { return position; }

    /**
     * @brief Gets the rotation of the object in local space
     * @return The rotation as a vec3 (in radians)
     */
    glm::vec3 getRotation() const { return rotation; }

    /**
     * @brief Gets the scale of the object in local space
     * @return The scale as a vec3
     */
    glm::vec3 getScale() const { return scale; }

    /**
     * @brief Sets the position of the object in local space
     * @param pos The new position
     */
    void setPosition(const glm::vec3& pos) { position = pos; matrixNeedsUpdate = true; }

    /**
     * @brief Sets the rotation of the object in local space
     * @param rot The new rotation (in radians)
     */
    void setRotation(const glm::vec3& rot) { rotation = rot; matrixNeedsUpdate = true; }

    /**
     * @brief Sets the scale of the object in local space
     * @param scl The new scale
     */
    void setScale(const glm::vec3& scl) { scale = scl; matrixNeedsUpdate = true; }

    // --- Hierarchy ---
    void addChild(std::shared_ptr<Object3D> child);
    void removeChild(std::shared_ptr<Object3D> child);
    const std::vector<std::shared_ptr<Object3D>>& getChildren() const { return children; }
    std::shared_ptr<Object3D> getParent() const { return parent.lock(); }

    // --- Matrix Updates ---
    void updateMatrix();
    const glm::mat4& getMatrix() const { return matrix; }
    const glm::mat4& getMatrixWorld() const { return matrixWorld; }

    // --- Object Properties ---
    const std::string& getName() const { return name; }
    void setName(const std::string& newName) { name = newName; }
    bool isVisible() const { return visible; }
    void setVisible(bool v) { visible = v; }

protected:
    // --- Protected Methods ---
    virtual void onBeforeUpdate() {}  // Override in derived classes
    virtual void onAfterUpdate() {}   // Override in derived classes

private:
    // --- Private Members ---
    std::string name;
    bool visible = true;
    bool matrixNeedsUpdate = true;
    
    // Transform properties
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};  // Euler angles in radians
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    
    // Transform matrices
    glm::mat4 matrix;        // Local transform matrix
    glm::mat4 matrixWorld;   // World transform matrix
    
    // Hierarchy
    std::weak_ptr<Object3D> parent;
    std::vector<std::shared_ptr<Object3D>> children;
}; 