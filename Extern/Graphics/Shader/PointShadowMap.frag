#version 320 es
precision highp float;

// ------------------------------------------------------------------
// INPUT VARIABLES --------------------------------------------------
// ------------------------------------------------------------------
in vec3 FragPos;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------
uniform vec3 u_LightPos;
uniform float u_FarPlane;

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------
void main() {
    // Calculate distance from fragment to light
    float lightDistance = length(FragPos - u_LightPos);
    
    // Map to [0, 1] range by dividing by far plane
    lightDistance = lightDistance / u_FarPlane;
    
    // Write to depth buffer
    gl_FragDepth = lightDistance;
}