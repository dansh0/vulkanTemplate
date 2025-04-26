#include "Mesh.h"

namespace graphics {

Mesh::Mesh(std::unique_ptr<IGeometryBuffer> buffer)
    : Object3D()
    , buffer_(std::move(buffer))
    , needsBufferUpdate_(false)
{
}

void Mesh::setGeometry(std::shared_ptr<Geometry> geometry) {
    geometry_ = geometry;
    if (geometry_) {
        vertices_ = geometry_->getVertices();
        indices_ = geometry_->getIndices();
        needsBufferUpdate_ = true;
    }
}

void Mesh::setMaterial(std::shared_ptr<Material> material) {
    material_ = material;
}

void Mesh::onBeforeUpdate() {
    Object3D::onBeforeUpdate();
    
    if (needsBufferUpdate_ && buffer_) {
        buffer_->updateVertexBuffer(vertices_);
        buffer_->updateIndexBuffer(indices_);
        needsBufferUpdate_ = false;
    }
}

void Mesh::onAfterUpdate() {
    Object3D::onAfterUpdate();
}

void Mesh::setVertices(const std::vector<Vertex>& vertices) {
    vertices_ = vertices;
    needsBufferUpdate_ = true;
}

void Mesh::setIndices(const std::vector<uint32_t>& indices) {
    indices_ = indices;
    needsBufferUpdate_ = true;
}

void Mesh::clearGeometry() {
    vertices_.clear();
    indices_.clear();
    needsBufferUpdate_ = true;
}

std::unique_ptr<IGeometryBuffer>& Mesh::getBuffer() {
    return buffer_;
}

} // namespace graphics 