#version 320 es
precision highp float;
precision highp int;
// ------------------------------------------------------------------
// INPUT VARIABLES --------------------------------------------------
// ------------------------------------------------------------------
layout(location = 0) in vec3 a_Position; 
layout(location = 1) in vec2 a_TexCoord;

// ------------------------------------------------------------------
// DATA STRUCTURES
// ------------------------------------------------------------------
struct Particle {
    vec4 position_life;
    vec4 velocity_size;
    vec4 color;
};

// ------------------------------------------------------------------
// BUFFERS
// ------------------------------------------------------------------

layout(std430, binding = 0) buffer ParticleBuffer {
    Particle particles[];
};

// ------------------------------------------------------------------
// UNIFORMS
// ------------------------------------------------------------------

uniform mat4 u_ViewProj;
uniform mat4 u_View;

// ------------------------------------------------------------------
// OUTPUT VARIABLES -------------------------------------------------
// ------------------------------------------------------------------
out vec2 v_TexCoord;
out vec4 v_Color;

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------
void main() {
    // gl_InstanceID is strictly an 'int' in ES, cast to 'uint' for SSBO indexing
    uint id = uint(gl_InstanceID);
    vec4 posLife = particles[id].position_life;

    // Cull dead particles
    if (posLife.w <= 0.0) {
        gl_Position = vec4(0.0);
        return;
    }

    v_Color = particles[id].color;
    v_TexCoord = a_TexCoord;
    float size = particles[id].velocity_size.w;

    // Camera Billboarding (Extract Right and Up vectors)
    vec3 cameraRight = vec3(u_View[0][0], u_View[1][0], u_View[2][0]);
    vec3 cameraUp    = vec3(u_View[0][1], u_View[1][1], u_View[2][1]);

    vec3 vertexPos = posLife.xyz 
                   + cameraRight * a_Position.x * size 
                   + cameraUp    * a_Position.y * size;

    gl_Position = u_ViewProj * vec4(vertexPos, 1.0);
}