#version 450

// Uniform Buffer Object containing matrices
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// Input attributes from vertex buffer
layout(location = 0) in vec3 inPosition; // Now vec3
layout(location = 1) in vec3 inColor;

// Output to fragment shader
layout(location = 0) out vec3 fragColor;

void main() {
    // Calculate final position in clip space
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    // Pass color through
    fragColor = inColor;
}