#pragma once

#include "Mesh.h"
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
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                // Position
                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0] * scale,
                    attrib.vertices[3 * index.vertex_index + 1] * scale,
                    attrib.vertices[3 * index.vertex_index + 2] * scale
                };

                // Normal (if available)
                if (index.normal_index >= 0) {
                    vertex.normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]
                    };
                } else {
                    // If no normal, use a default up vector
                    vertex.normal = {0.0f, 1.0f, 0.0f};
                }

                // // Texture coordinates (if available)
                // if (index.texcoord_index >= 0) {
                //     vertex.texCoord = {
                //         attrib.texcoords[2 * index.texcoord_index + 0],
                //         attrib.texcoords[2 * index.texcoord_index + 1]
                //     };
                // } else {
                //     // If no texture coordinates, use default
                //     vertex.texCoord = {0.0f, 0.0f};
                // }

                // Color (if available)
                if (attrib.colors.size() > 0) {
                    vertex.color = {
                        attrib.colors[3 * index.vertex_index + 0],
                        attrib.colors[3 * index.vertex_index + 1],
                        attrib.colors[3 * index.vertex_index + 2]
                    };
                } else {
                    // If no color, use white
                    vertex.color = {1.0f, 1.0f, 1.0f};
                }

                vertices.push_back(vertex);
                indices.push_back(static_cast<uint32_t>(indices.size()));
            }
        }

        std::cout << "Loaded OBJ file: " << filename << std::endl;
        std::cout << "Vertices: " << vertices.size() << std::endl;
        std::cout << "Indices: " << indices.size() << std::endl;

        return true;
    }
}; 