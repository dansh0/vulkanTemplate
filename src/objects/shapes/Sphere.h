#pragma once

#include "../../common/Vertex.h"       // Include the Vertex definition
#include <vector>
#include <cstdint>      // For uint32_t

/**
 * @brief Generates vertex and index data for a UV sphere.
 *
 * Creates a sphere centered at the origin with the specified radius,
 * divided into a grid of sectors (longitude lines) and stacks (latitude lines).
 * The vertices are assigned alternating black and white colors based on their grid position.
 *
 * @param radius The radius of the sphere.
 * @param sectors The number of longitudinal divisions (around the equator). Higher values increase detail horizontally. Must be >= 3.
 * @param stacks The number of latitudinal divisions (from pole to pole). Higher values increase detail vertically. Must be >= 2.
 * @param outVertices A reference to a std::vector<Vertex> that will be cleared and filled with the generated vertex data.
 * @param outIndices A reference to a std::vector<uint32_t> that will be cleared and filled with the generated index data for triangle list rendering.
 *
 * Keywords: Procedural Geometry, UV Sphere, Mesh Generation, Spherical Coordinates
 */
void generateSphere(float radius, int sectors, int stacks,
                    std::vector<Vertex>& outVertices,
                    std::vector<uint32_t>& outIndices);