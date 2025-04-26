#pragma once

#include "../object3D/Object3D.h"
#include "../camera/Camera.h"
#include "../../geometry/mesh/Mesh.h"
#include <memory>
#include <vector>
#include <string>

namespace graphics {

/**
 * @brief Main scene class that manages all objects and cameras
 * 
 * The Scene class is responsible for managing the scene graph, cameras,
 * and providing update/render functionality for all objects.
 */
class Scene {
public:
    Scene();
    ~Scene();

    // --- Scene Management ---
    void init(const std::string& modelPath, std::unique_ptr<IGeometryBuffer> buffer);
    void update(float deltaTime);

    // --- Object Management ---
    void addObject(std::shared_ptr<Object3D> object);
    void removeObject(std::shared_ptr<Object3D> object);
    const std::vector<std::shared_ptr<Object3D>>& getObjects() const { return objects_; }
    std::shared_ptr<Object3D> getObject(size_t index) const;
    size_t getObjectCount() const { return objects_.size(); }

    // --- Camera Management ---
    std::shared_ptr<Camera> getCamera() const { return camera_; }
    void setCamera(std::shared_ptr<Camera> newCamera) { camera_ = newCamera; }

    // --- Mesh Management ---
    std::shared_ptr<Mesh> getMainMesh() const { return mainMesh_; }
    void setMainMesh(std::shared_ptr<Mesh> mesh) { mainMesh_ = mesh; }

    // --- Transform Information ---
    glm::mat4 getMainMeshTransform() const;
    glm::vec3 getMainMeshPosition() const;
    glm::vec3 getMainMeshRotation() const;

private:
    std::shared_ptr<Camera> camera_;
    std::vector<std::shared_ptr<Object3D>> objects_;
    std::shared_ptr<Mesh> mainMesh_;
};

} // namespace graphics