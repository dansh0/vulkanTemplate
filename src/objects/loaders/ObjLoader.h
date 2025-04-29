#pragma once

#include "../../common/Vertex.h"
#include <tiny_obj_loader/tiny_obj_loader.h>
#include <string>
#include <vector>
#include <iostream>

/**
 * @brief Utility class for loading OBJ files and converting them to our Mesh format
 * 
 * This class handles loading OBJ files using tiny_obj_loader and converting them
 * to our internal Vertex/Index format. It supports loading both geometry and
 * material properties.
 */
class ObjLoader {
private:
    /**
     * @brief Calculates the normal of a triangle given its three vertices
     * @param v0 First vertex position
     * @param v1 Second vertex position
     * @param v2 Third vertex position
     * @return Normalized normal vector
     */
    static glm::vec3 calculateTriangleNormal(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2) {
        // Create two vectors from the triangle's edges
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        
        // Calculate cross product to get normal
        glm::vec3 normal = glm::cross(edge1, edge2);
        
        // Normalize the result
        return glm::normalize(normal);
    }

public:
    /**
     * @brief Loads an OBJ file and converts it to our Mesh format
     * @param filename Path to the OBJ file
     * @param vertices Output vector for vertices
     * @param indices Output vector for indices
     * @return true if loading was successful, false otherwise
     */
    static bool loadObj(const std::string& filename, 
                       const float scale,
                       std::vector<Vertex>& vertices, 
                       std::vector<uint32_t>& indices) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        // Load the OBJ file
        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str())) {
            std::cerr << "Failed to load OBJ file: " << filename << std::endl;
            if (!warn.empty()) std::cerr << "WARN: " << warn << std::endl;
            if (!err.empty()) std::cerr << "ERR: " << err << std::endl;
            return false;
        }

        // Clear output vectors
        vertices.clear();
        indices.clear();

        // For each shape in the OBJ file
        for (const auto& shape : shapes) {
            // For each face in the shape
            for (size_t f = 0; f < shape.mesh.indices.size(); f += 3) {
                // Get the three vertices of the triangle
                tinyobj::index_t idx0 = shape.mesh.indices[f + 0];
                tinyobj::index_t idx1 = shape.mesh.indices[f + 1];
                tinyobj::index_t idx2 = shape.mesh.indices[f + 2];

                // Get vertex positions
                glm::vec3 v0 = {
                    attrib.vertices[3 * idx0.vertex_index + 0] * scale,
                    attrib.vertices[3 * idx0.vertex_index + 1] * scale,
                    attrib.vertices[3 * idx0.vertex_index + 2] * scale
                };
                glm::vec3 v1 = {
                    attrib.vertices[3 * idx1.vertex_index + 0] * scale,
                    attrib.vertices[3 * idx1.vertex_index + 1] * scale,
                    attrib.vertices[3 * idx1.vertex_index + 2] * scale
                };
                glm::vec3 v2 = {
                    attrib.vertices[3 * idx2.vertex_index + 0] * scale,
                    attrib.vertices[3 * idx2.vertex_index + 1] * scale,
                    attrib.vertices[3 * idx2.vertex_index + 2] * scale
                };

                // Calculate normal for this triangle
                glm::vec3 normal = calculateTriangleNormal(v0, v1, v2);

                // Add vertices with the calculated normal
                for (size_t i = 0; i < 3; i++) {
                    tinyobj::index_t idx = shape.mesh.indices[f + i];
                    Vertex vertex{};

                    // Position
                    vertex.pos = {
                        attrib.vertices[3 * idx.vertex_index + 0] * scale,
                        attrib.vertices[3 * idx.vertex_index + 1] * scale,
                        attrib.vertices[3 * idx.vertex_index + 2] * scale
                    };

                    // Use calculated normal
                    vertex.normal = normal;

                    // Color (if available)
                    if (attrib.colors.size() > 0) {
                        vertex.color = {
                            attrib.colors[3 * idx.vertex_index + 0],
                            attrib.colors[3 * idx.vertex_index + 1],
                            attrib.colors[3 * idx.vertex_index + 2]
                        };
                    } else {
                        // If no color, use white
                        vertex.color = {1.0f, 1.0f, 1.0f};
                    }

                    vertices.push_back(vertex);
                    indices.push_back(static_cast<uint32_t>(indices.size()));
                }
            }
        }

        std::cout << "Loaded OBJ file: " << filename << std::endl;
        std::cout << "Vertices: " << vertices.size() << std::endl;
        std::cout << "Indices: " << indices.size() << std::endl;

        return true;
    }
}; 