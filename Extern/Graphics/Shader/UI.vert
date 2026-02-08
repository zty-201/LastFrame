#version 320 es
precision highp float;
// ------------------------------------------------------------------
// INPUT VARIABLES --------------------------------------------------
// ------------------------------------------------------------------

layout(location = 0) in vec3 VS_IN_Position;
layout(location = 1) in vec2 VS_IN_TexCoord;

// ------------------------------------------------------------------
// OUTPUT VARIABLES -------------------------------------------------
// ------------------------------------------------------------------

out vec2 FS_IN_TexCoord;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform mat4 u_Model;      // Converts Quad (0 to 1) -> Pixel Coordinates
uniform mat4 u_Projection; // Converts Pixel Coordinates -> Screen (-1 to 1)

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    FS_IN_TexCoord = VS_IN_TexCoord;
    
    // Standard MVP transform (View is identity in 2D usually)
    gl_Position = u_Projection * u_Model * vec4(VS_IN_Position, 1.0);
}