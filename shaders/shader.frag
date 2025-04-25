#version 450

// Input from vertex shader
layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragColor;

// Output color
layout(location = 0) out vec4 outColor;

void main() {

    vec3 objCol = vec3(1.0, 1.0, 1.0); // Base material color
    vec3 lightPos = vec3(3.0, 3.0, 3.0);
    vec3 lightPos2 = vec3(-3.0, 3.0, 3.0);
    vec3 lightCol = vec3(1.0, 1.0, 1.0); // Light color
    float ambiStrength = 0.3; // Ambient light strength
    float internalDiffStength = 0.3; // Diffuse light strength for internal light
    float diffStrength = 0.05; // Diffuse light strength
    float diffStrength2 = 0.05;
    float specStrength = 0.0; // Specular light strength
    float specPow = 0.1; // Specular light power (spread)

    // Ambient Lighting
    vec3 ambiLight = lightCol * ambiStrength;
    
    // Internal Diffuse Lighting
    vec3 internalDiffDir = normalize(-fragPosition);
    vec3 internalDiffLight = lightCol * internalDiffStength * max(dot(fragNormal, internalDiffDir), 0.0);
    
    // Diffuse Lighting
    vec3 diffDir = normalize(lightPos - fragPosition);
    vec3 diffLight = lightCol * diffStrength * max(dot(fragNormal, diffDir), 0.0);
    
    // Diffuse Lighting 2
    vec3 diffDir2 = normalize(lightPos2 - fragPosition);
    vec3 diffLight2 = lightCol * diffStrength2 * max(dot(fragNormal, diffDir2), 0.0);
    
    // Specular Lighting
    // vec3 reflDir = reflect(-diffDir, fragNormal);
    // float specFact = pow(max(dot(-cameraDir, reflDir), 0.0), specPow);
    // vec3 specLight = lightCol * specStrength * specFact;
    
    // Phong Combined Lighting
    vec3 combLight = ambiLight + internalDiffLight + diffLight + diffLight2;// + specLight;
    vec3 col = combLight * objCol;



    // Output the interpolated color with full alpha
    outColor = vec4(col, 1.0);
}