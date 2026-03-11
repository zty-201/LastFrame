#version 320 es
precision highp float;
precision highp sampler2D; 
// ------------------------------------------------------------------
// OUTPUT VARIABLES  ------------------------------------------------
// ------------------------------------------------------------------

layout(location=0) out vec4 FS_OUT_Color;

// ------------------------------------------------------------------
// INPUT VARIABLES  -------------------------------------------------
// ------------------------------------------------------------------

in vec2 FS_IN_TexCoord;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform sampler2D s_Texture;
uniform vec4 u_Color; 
uniform float u_Luminance;
uniform bool u_HasVideoFrame;
// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
   vec4 texColor = texture(s_Texture, FS_IN_TexCoord);
   if (u_HasVideoFrame) {
       texColor.a = 1.0;
   }
    
    // Multiply texture by tint color
    FS_OUT_Color = texColor * u_Color * u_Luminance;
    
    // Optional: Discard fully transparent pixels unless a video is bound
    if (!u_HasVideoFrame && FS_OUT_Color.a < 0.01) discard; 

}
