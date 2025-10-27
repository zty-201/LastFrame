#version 310 es

precision highp float;
precision highp int;

uniform int materialType;

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

struct Fog {
    float minDist;            
    float maxDist;            
    vec3 color;            
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
uniform Fog fog;

layout(binding=0) uniform sampler2D DiffuseTex;
layout(binding=1) uniform samplerCube CubeMapTex;
layout(binding=2) uniform sampler2D NormalMap;

uniform bool UseTexture;
uniform bool UseNormalMap;

const int CARTOON_LEVELS = 3;
const int CHECKERBOARD_SIZE = 50;
const float CHECKERBOARD_MIXLEVEL = 0.5f; 
const float DISCARD_RADIUS = 0.2;  

layout(location=0) out vec4 FragColor;

const float PI = 3.14159265358979323846;

float cartoon(float value)
{
    const int levels = CARTOON_LEVELS;
    const float scaleFactor = 1.0f / float(levels);
    return floor(value * float(levels)) * scaleFactor;
}

vec3 blinnPhongCartoon(vec3 position, vec3 n, InstanceData instance)
{
    vec3 lightPosView = vec3(V * vec4(light[0].position, 1.0f));

    vec3 L = lightPosView - position;
    if (length(L) < 1e-6f) return vec3(0.0f); 
    L = normalize(L);

    vec3 normal = normalize(n);
    vec3 V = normalize(-position);
    vec3 H = normalize(L + V);

    float sDotN = max(dot(normal, L), 0.0f);
    float hDotN = max(dot(normal, H), 0.0f);

    float dist = length(lightPosView - position);
    float attenuation = 1.0 / (1.0 + 0.1 * dist + 0.02 * (dist * dist));

    vec3 ambient = light[0].La * instance.Ka;
    vec3 diffuse = light[0].Ld * instance.Kd * cartoon(sDotN);
    vec3 specular = light[0].Ls * instance.Ks * cartoon(pow(max(hDotN, 0.0f), instance.shininess));

    if (materialType == 0){
        diffuse *= attenuation;
        specular *= attenuation;
    }
   
    return ambient + diffuse + specular;
}

vec3 BlinnPhong(vec3 position, vec3 normal, Light light, InstanceData instance, mat4 view)
{
    vec3 lightPosView = vec3(view * vec4(light.position, 1.0f));

    vec3 L = lightPosView - position;
    if (length(L) < 1e-6f) return vec3(0.0f); 
    L = normalize(L);

    normal = normalize(normal);
    vec3 V = normalize(-position);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(normal, L), 0.0f);
    float NdotH = max(dot(normal, H), 0.0f);

    vec3 ambient = light.La * instance.Ka;
    vec3 diffuse = light.Ld * instance.Kd * NdotL;
    vec3 specular = (NdotL > 0.0f) ? light.Ls * instance.Ks * pow(NdotH, instance.shininess) : vec3(0.0f);

    if (UseTexture) {
        vec4 texColor = texture(DiffuseTex, TexCoord);
        diffuse = texColor.rgb;
    }

    return ambient + diffuse + specular;
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
    InstanceData instance = instances[InstanceIndex];  // Read per-instance data
    
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
    
    // Use instance effect instead of uniform material.effect
    if (instance.effect == 1u) {
        vec2 centers[3] = vec2[3](vec2(0.3f, 0.3f), vec2(0.7f, 0.3f), vec2(0.5f, 0.7f));
        float radius = DISCARD_RADIUS;
        for (int i = 0; i < 3; ++i) {
            float dist = distance(TexCoord, centers[i]);
            if (dist < radius)
                discard;
        }
        
        if (materialType == 0) {
            color = BlinnPhong(Position, normal, light[0], instance, V);
        }
    } 
    else if (instance.effect == 2u) {
        color = blinnPhongCartoon(Position, normal, instance);
    } 
    else if (instance.effect == 3u) {
        vec4 checkerboard = checkerboardTexture(TexCoord, float(CHECKERBOARD_SIZE));
        color = mix(color, checkerboard.rgb, CHECKERBOARD_MIXLEVEL);
    }

    float dist = length(Position);
    float fogFactor = clamp((dist - fog.minDist) / (fog.maxDist - fog.minDist), 0.0f, 1.0f);
    color = mix(color, fog.color, fogFactor);

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