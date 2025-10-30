#version 310 es

precision highp float;
precision highp int;

struct Light {
    vec3 position;
    vec3 L;
    vec3 La;
    vec3 Ld;
    vec3 Ls;
};

// SSBO
struct InstanceData {
    mat4 modelMatrix;
    vec4 tintColor;
    
    vec3 Ka;
    float roughness;
    
    vec3 Kd;
    float metallic;
    
    vec3 Ks;
    float shininess;
    
    uint effect;
    uint padding[3];
};

layout(std430, binding=0) readonly buffer InstanceBuffer {
    InstanceData instances[];
};

in vec3 Position;
in vec3 Normal;
in vec2 TexCoord;
in vec3 Vec;
in mat3 TBN;
in flat uint InstanceIndex; 

uniform Light light[1];
uniform mat4 V;
uniform int Pass; 

layout(binding=0) uniform sampler2D DiffuseTex;
layout(binding=1) uniform samplerCube CubeMapTex;
layout(binding=2) uniform sampler2D NormalMap;

uniform bool UseTexture;
uniform bool UseNormalMap;

const int CHECKERBOARD_SIZE = 50;
const float CHECKERBOARD_MIXLEVEL = 0.5f; 
const float DISCARD_RADIUS = 0.2;  

layout(location=0) out vec4 FragColor;

const float PI = 3.14159265358979323846;

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

vec3 pbrLighting(vec3 position, vec3 normal, vec3 albedo, float roughness, float metallic)
{
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

vec4 checkerboardTexture(vec2 uv, float size) {
    float u = uv.x - floor(uv.x);
    float v = uv.y - floor(uv.y);
    float scaledU = u * size;
    float scaledV = v * size;
    if (mod(floor(scaledU) + floor(scaledV), 2.0) == 0.0)
        return vec4(0.0, 0.0, 0.0, 1.0);
    else
        return vec4(1.0, 1.0, 1.0, 1.0);
}

void scenePass() 
{
    InstanceData instance = instances[InstanceIndex];
    
    vec3 normal;
    if (UseNormalMap) {
        vec3 normalMapSample = texture(NormalMap, TexCoord).rgb;
        normalMapSample = normalMapSample * 2.0 - 1.0;
        normal = normalize(TBN * normalMapSample);
    } else {
        normal = normalize(Normal);
    }
    
    vec3 color = instance.tintColor.rgb;
    if (UseTexture) {
        color = texture(DiffuseTex, TexCoord).rgb;
    }
    
    // Handle special effects
    if (instance.effect == 1u) {
        // Discard effect
        vec2 centers[3] = vec2[3](vec2(0.3f, 0.3f), vec2(0.7f, 0.3f), vec2(0.5f, 0.7f));
        float radius = DISCARD_RADIUS;
        for (int i = 0; i < 3; ++i) {
            float dist = distance(TexCoord, centers[i]);
            if (dist < radius)
                discard;
        }
    } 
    else if (instance.effect == 3u) {
        // Checkerboard effect
        vec4 checkerboard = checkerboardTexture(TexCoord, float(CHECKERBOARD_SIZE));
        color = mix(color, checkerboard.rgb, CHECKERBOARD_MIXLEVEL);
    }
    
    // Apply PBR lighting
    color = pbrLighting(Position, normal, color, instance.roughness, instance.metallic);
    
    // Add ambient
    color += instance.Ka * 0.1f;

    // Gamma correction
    color = pow(color, vec3(1.0f/2.2f));

    FragColor = vec4(color, 1.0f);
}

void skyboxPass() 
{
    vec3 color = texture(CubeMapTex, normalize(Vec)).rgb;
    color = pow(color, vec3(1.0f/2.2f));
    FragColor = vec4(color, 1.0f);
}

void main()
{
    if (Pass == 0) scenePass();
    else if (Pass == 1) skyboxPass();
    else if (Pass == 2) {
        FragColor = vec4(1.0, 1.0, 0.0, 1.0);
        return;
    }
}