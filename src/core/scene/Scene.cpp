#include "Scene.h"
#include "../../geometry/loaders/ObjLoader.h"
#include <iostream>

namespace graphics {

Scene::Scene() {
    // Initialize camera with default values
    camera_ = std::make_shared<Camera>();
    camera_->setPosition(glm::vec3(0.0f, 0.0f, 5.0f));
    camera_->setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    camera_->setUp(glm::vec3(0.0f, 1.0f, 0.0f));
}

Scene::~Scene() {
    // Clear all objects
    objects_.clear();
}

/**
 * @brief Initializes the scene with a model and buffer implementation
 * @param modelPath Path to the model file
 * @param buffer The buffer implementation to use for geometry
 */
void Scene::init(const std::string& modelPath, std::unique_ptr<IGeometryBuffer> buffer) {
    // Create geometry and load from file
    auto geometry = std::make_shared<Geometry>();
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    if (!ObjLoader::loadObj(modelPath, 0.05f, vertices, indices)) {
        std::cerr << "Failed to load model: " << modelPath << std::endl;
        throw std::runtime_error("Failed to load model");
    }

    // Set the loaded data
    geometry->setVertices(vertices);
    geometry->setIndices(indices);

    // Create main mesh with the provided buffer
    mainMesh_ = std::make_shared<Mesh>(std::move(buffer));
    mainMesh_->setGeometry(geometry);

    // Create object and add it to scene
    auto object = std::make_shared<Object3D>();
    object->addChild(mainMesh_);
    addObject(object);
}

/**
 * @brief Updates the scene state
 * @param deltaTime The time elapsed since the last frame in seconds
 */
void Scene::update(float deltaTime) {
    // Update all objects in the scene
    for (auto& object : objects_) {
        object->updateMatrix();
    }
}

void Scene::addObject(std::shared_ptr<Object3D> object) {
    if (object) {
        objects_.push_back(object);
    }
}

void Scene::removeObject(std::shared_ptr<Object3D> object) {
    if (object) {
        objects_.erase(
            std::remove(objects_.begin(), objects_.end(), object),
            objects_.end()
        );
    }
}

std::shared_ptr<Object3D> Scene::getObject(size_t index) const {
    if (index < objects_.size()) {
        return objects_[index];
    }
    return nullptr;
}

glm::mat4 Scene::getMainMeshTransform() const {
    if (mainMesh_) {
        return mainMesh_->getMatrix();
    }
    return glm::mat4(1.0f);
}

glm::vec3 Scene::getMainMeshPosition() const {
    if (mainMesh_) {
        return mainMesh_->getPosition();
    }
    return glm::vec3(0.0f);
}

glm::vec3 Scene::getMainMeshRotation() const {
    if (mainMesh_) {
        return mainMesh_->getRotation();
    }
    return glm::vec3(0.0f);
}

} // namespace graphics