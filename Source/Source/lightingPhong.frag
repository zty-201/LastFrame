#version 330

out vec4 FragColor;

in vec3 normalVec, lightVec, eyeVec;
uniform int objectId;

void main()
{
    if (objectId == 5) {
        // Simple diffuse lighting
        vec3 N = normalize(normalVec);
        vec3 L = normalize(lightVec);
        float NdotL = max(dot(N, L), 0.0);
        
        vec3 baseColor = vec3(0.0, 1.0, 0.0); // Green
        vec3 finalColor = baseColor * (0.3 + 0.7 * NdotL); // Ambient + diffuse
        
        FragColor = vec4(finalColor, 1.0);
    } else {
        FragColor = vec4(1.0, 0.0, 1.0, 1.0); // Bright magenta
    }
}