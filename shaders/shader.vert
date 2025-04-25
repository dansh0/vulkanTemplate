#version 450

// Uniform Buffer Object containing matrices
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// Input attributes from vertex buffer
layout(location = 0) in vec3 inPosition; // Now vec3
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

// Output to fragment shader
layout(location = 0) out vec3 outPosition;  
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outColor;

void main() {
    // Calculate final position in clip space
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    // Pass color through
    outColor = inColor;
    outNormal = inNormal;
    outPosition = inPosition;
}