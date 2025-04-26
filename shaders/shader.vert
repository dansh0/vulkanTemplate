#version 450

// Uniform Buffer Object containing matrices
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// Input attributes from vertex buffer
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

// Output to fragment shader
layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragColor;

void main() {
    // Transform position to world space
    vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
    
    // Calculate final position in clip space
    gl_Position = ubo.proj * ubo.view * worldPos;
    
    // Pass through attributes
    fragPosition = worldPos.xyz;
    fragNormal = mat3(ubo.model) * inNormal; // Transform normal to world space
    fragColor = inColor;
}