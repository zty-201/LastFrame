#version 310 es
precision highp float;

in vec3 FragPos;

uniform vec3 u_LightPos;
uniform float u_FarPlane;

void main() {
    // Calculate distance from fragment to light
    float lightDistance = length(FragPos - u_LightPos);
    
    // Map to [0, 1] range by dividing by far plane
    lightDistance = lightDistance / u_FarPlane;
    
    // Write to depth buffer
    gl_FragDepth = lightDistance;
}