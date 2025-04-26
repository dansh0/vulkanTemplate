#include "Sphere.h"
#include <glm/gtc/constants.hpp> // For glm::pi()
#include <cmath>                 // For cosf, sinf
#include <iostream>              // For debug output

// Implementation of the sphere generation function.
// See Sphere.h for detailed documentation.
void generateSphere(float radius, int sectors, int stacks,
                    std::vector<Vertex>& outVertices,
                    std::vector<uint32_t>& outIndices)
{
    outVertices.clear();
    outIndices.clear();

    // Basic validation
    if (radius <= 0.0f || sectors < 3 || stacks < 2) {
        std::cerr << "Warning: Invalid parameters for sphere generation (radius=" << radius
                  << ", sectors=" << sectors << ", stacks=" << stacks << "). Aborting." << std::endl;
        return;
    }

    float x, y, z, xy; // Vertex position components

    // Calculate angular steps for sectors (longitude) and stacks (latitude)
    float sectorStep = 2.0f * glm::pi<float>() / sectors;
    float stackStep = glm::pi<float>() / stacks;
    float sectorAngle, stackAngle;

    // --- Generate Vertices ---
    // Iterate through stacks (latitude: from +pi/2 at the top pole to -pi/2 at the bottom pole)
    for (int i = 0; i <= stacks; ++i)
    {
        stackAngle = glm::pi<float>() / 2.0f - i * stackStep; // Angle from the xy-plane
        xy = radius * cosf(stackAngle);             // Radius projected onto the xy-plane (r * cos(latitude))
        z = radius * sinf(stackAngle);              // Height along the z-axis (r * sin(latitude))

        // Iterate through sectors (longitude: from 0 to 2pi)
        // We add (sectors + 1) vertices per stack to duplicate the first vertex
        // at the end, simplifying texture mapping (though not used here) and index generation.
        for (int j = 0; j <= sectors; ++j)
        {
            sectorAngle = j * sectorStep; // Angle in the xy-plane (longitude)

            // Vertex position (x, y, z) calculated from spherical coordinates
            x = xy * cosf(sectorAngle); // r * cos(latitude) * cos(longitude)
            y = xy * sinf(sectorAngle); // r * cos(latitude) * sin(longitude)

            // Determine color based on grid position for a checkerboard pattern
            // XORing the parity of stack and sector indices creates the pattern.
            bool isWhite = (i % 2 == 0) ^ (j % 2 == 0);
            glm::vec3 color = isWhite ? glm::vec3(1.0f, 1.0f, 1.0f) : glm::vec3(0.1f, 0.1f, 0.1f);

            // Add the vertex. Note the coordinate mapping: (x, z, y) maps the mathematical
            // sphere (where Z is typically up) to a coordinate system often used in graphics
            // where Y is up/down on screen and Z is depth. Adjust if your view matrix assumes differently.
            outVertices.push_back({{x, z, y}, color});
        }
    }

    // --- Generate Indices for Triangles ---
    // Iterate through the quads formed by stacks and sectors to create triangles.
    uint32_t k1, k2; // Indices of vertices in the current and next stack
    for (int i = 0; i < stacks; ++i)
    {
        k1 = i * (sectors + 1);     // Start index of current stack
        k2 = k1 + sectors + 1;      // Start index of next stack

        for (int j = 0; j < sectors; ++j, ++k1, ++k2)
        {
            // 2 triangles per sector face (forming a quad)
            // Quad vertices: k1, k1+1, k2, k2+1

            // Triangle 1: k1 -> k2 -> k1+1
            // Skip triangle generation for the top pole (i == 0 leads to degenerate triangles)
            if (i != 0)
            {
                outIndices.push_back(k1);
                outIndices.push_back(k2);
                outIndices.push_back(k1 + 1);
            }

            // Triangle 2: k1+1 -> k2 -> k2+1
            // Skip triangle generation for the bottom pole (i == stacks - 1 leads to degenerate triangles)
            if (i != (stacks - 1))
            {
                outIndices.push_back(k1 + 1);
                outIndices.push_back(k2);
                outIndices.push_back(k2 + 1);
            }
        }
    }

    // Optional: Output generated mesh stats
    // std::cout << "Generated sphere: " << outVertices.size() << " vertices, " << outIndices.size() << " indices." << std::endl;
}