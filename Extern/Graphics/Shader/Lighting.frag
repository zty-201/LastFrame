#version 310 es

precision highp float;
precision highp int;

in vec2 TexCoord;

layout(location=0) out vec4 FragColor;

// G-buffer inputs
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

// Lighting
struct Light {
    vec3 position;
    vec3 L;
    vec3 La;
    vec3 Ld;
    vec3 Ls;
};

struct Material {
    float rough;
    float metal;
    vec3 color;
    vec3 Ka;
    vec3 Kd;
    vec3 Ks;
    float shininess;
    float effect;          
};

uniform Light light[1];
uniform Material material;
uniform mat4 V;

// FOG PARAMETERS
uniform vec3 fogColor;
uniform float fogDensity;
uniform float fogAbsorption;
uniform vec3 fogLightDirection;
uniform vec3 fogLightColor;
uniform float fogLightIntensity;
uniform float fogAnisotropy;

const float PI = 3.14159265358979323846;

// =====================================================
// FOG FUNCTIONS
// =====================================================

float phaseFunction(float cosAngle, float g)
{
    float g2 = g * g;
    float denom = 1.0 + g2 - 2.0 * g * cosAngle;
    return (1.0 - g2) / (4.0 * PI * denom * sqrt(denom));
}

vec3 calculateFog(vec3 viewSpacePos, vec3 litColor)
{
    float viewDepth = length(viewSpacePos);
    
    // Clamp max distance for stronger effect
    if (viewDepth > 80.0) {
        return fogColor;  // ← Objects far away = solid fog color
    }
    
    // Exponential fog falloff - much stronger
    float fogAmount = 1.0 - exp(-viewDepth * viewDepth * fogDensity * 0.01);
    
    // View direction for phase function
    vec3 viewDir = normalize(viewSpacePos);
    float cosAngle = dot(viewDir, normalize(fogLightDirection));
    float phase = phaseFunction(cosAngle, fogAnisotropy);
    
    // Light contribution through fog
    vec3 fogLighting = fogLightColor * fogLightIntensity * phase;
    
    // Mix fog color with light contribution
    vec3 finalFogColor = mix(fogLighting, fogColor, 0.7);  // ← Increase fog color influence
    
    // Stronger blending
    vec3 result = mix(litColor, finalFogColor, fogAmount * 1.2);
    
    return result;
}

// =====================================================
// PBR FUNCTIONS
// =====================================================

float ggxDistribution(float nDotH, float roughness) {
    float alpha2 = roughness * roughness * roughness * roughness;
    float d = (nDotH * nDotH) * (alpha2 - 1.0f) + 1.0f;
    return alpha2 / (PI * d * d);
}

float geomSmith(float nDotL, float roughness) {
    float k = (roughness + 1.0f) * (roughness + 1.0f) / 8.0f;
    float denom = nDotL * (1.0f - k) + k;
    return 1.0f / denom;
}

vec3 schlickFresnel(float lDotH, vec3 baseReflectivity) {
    return baseReflectivity + (1.0f - baseReflectivity) * pow(1.0f - lDotH, 5.0);
}

vec3 microfacetModel(vec3 position, vec3 normal, vec3 albedo, float metallic, float roughness) {
    vec3 lightI = light[0].L;
    vec3 lightPositionInView = (V * vec4(light[0].position, 1.0f)).xyz;

    vec3 l = lightPositionInView - position;
    float dist = length(l);
    l = normalize(l);
    lightI *= 100.0 / (dist * dist);

    vec3 v = normalize(-position);
    vec3 h = normalize(v + l);
    float nDotH = dot(normal, h);
    float lDotH = dot(l, h);
    float nDotL = max(dot(normal, l), 0.0f);
    float nDotV = max(dot(normal, v), 0.0f);
    
    // Base reflectivity for metals vs dielectrics
    vec3 f0 = vec3(0.04); 
    if (metallic > 0.5) f0 = albedo;
    
    vec3 specBrdf = ggxDistribution(nDotH, roughness) * 
                    schlickFresnel(lDotH, f0) * 
                    geomSmith(nDotL, roughness) * 
                    geomSmith(nDotV, roughness) * 0.25f;

    return (albedo / PI + specBrdf) * lightI * nDotL;
}

// =====================================================
// MAIN
// =====================================================

void main() {
    // 1. Retrieve data from G-buffer
    vec3 position = texture(gPosition, TexCoord).rgb;
    vec3 normal = texture(gNormal, TexCoord).rgb;
    vec4 albedoSpec = texture(gAlbedoSpec, TexCoord);
    vec3 albedo = albedoSpec.rgb;
    float metallic = albedoSpec.a;
    
    // 2. Calculate PBR lighting
    vec3 color = microfacetModel(position, normal, albedo, metallic, material.rough);
    
    // 3. Add ambient lighting
    color += albedo * material.Ka;
    
    // 4. APPLY FOG
    color = calculateFog(position, color);
    
    // 5. Apply gamma correction
    color = pow(color, vec3(1.0f/2.2f));
    
    FragColor = vec4(color, 1.0);
}