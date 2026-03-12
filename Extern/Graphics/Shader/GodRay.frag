#version 320 es
precision highp float;
precision highp sampler2D; 
// ------------------------------------------------------------------
// OUTPUT VARIABLES  ------------------------------------------------
// ------------------------------------------------------------------

out vec4 FS_OUT_Color;

// ------------------------------------------------------------------
// INPUT VARIABLES  -------------------------------------------------
// ------------------------------------------------------------------

in vec2 FS_IN_TexCoord;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform sampler2D s_Texture;
uniform vec2 u_SunPos;
uniform float u_Intensity;

// Mobile-Optimized Hardcoded Constants
const int NUM_SAMPLES = 32;   // Drop to 16 if Android performance dips
const float DENSITY = 0.95;   // How tightly packed the rays are
const float WEIGHT = 0.05;    // Brightness of each individual sample
const float DECAY = 0.96;     // How fast the rays fade out
const float EXPOSURE = 0.6;   // Final brightness multiplier
// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main() {
    
    // Calculate the vector pointing from the current pixel to the sun
    vec2 deltaTextCoord = FS_IN_TexCoord - u_SunPos;
    
    // Divide the vector into small steps based on our sample count
    deltaTextCoord *= 1.0 / float(NUM_SAMPLES) * DENSITY;

    vec2 currentUV = FS_IN_TexCoord;
    vec4 color = texture(s_Texture, currentUV);
    float illuminationDecay = 1.0;

    // The Radial Blur Loop
    for(int i = 0; i < NUM_SAMPLES; i++) {
        currentUV -= deltaTextCoord;
        vec4 sampleColor = texture(s_Texture, currentUV);
        
        
        sampleColor *= illuminationDecay * WEIGHT;
        color += sampleColor;
        
        illuminationDecay *= DECAY;
    }

    // Output the final blurred color, scaled by whether the sun is on-screen
    FS_OUT_Color = color * EXPOSURE * u_Intensity;
}