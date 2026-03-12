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

uniform int u_UseSubtitleReveal;
uniform float u_SubtitleRevealProgress;
uniform float u_SubtitleRevealSoftness;
// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    vec4 texColor = texture(s_Texture, FS_IN_TexCoord);

    // Base tint + luminance
    vec4 finalColor = texColor * u_Color;
    finalColor.rgb *= u_Luminance;

    // Left-to-right subtitle reveal
    if (u_UseSubtitleReveal == 1)
    {
        float reveal = 1.0 - smoothstep(
            u_SubtitleRevealProgress - u_SubtitleRevealSoftness,
            u_SubtitleRevealProgress,
            FS_IN_TexCoord.x
        );

        finalColor.rgb *= reveal;
        finalColor.a *= reveal;

	FS_OUT_Color = finalColor;
    }

    else
    {

    	 FS_OUT_Color = texColor * u_Color * u_Luminance;

    }

    // Optional: Discard fully transparent pixels to save fill rate
    if (FS_OUT_Color.a < 0.01) discard;
}