#version 310 es

//Specify precision for es. use highp for more accurate lighting.
precision highp float;
precision highp int;

uniform int materialType;

struct Light 
{
    vec3 position;
    vec3 L;
    vec3 La;
    vec3 Ld;
    vec3 Ls;
};

struct Material 
{
    float rough;
    float metal;
    vec3 color;
    vec3 Ka;
    vec3 Kd;
    vec3 Ks;
    float shininess;
    float effect;          
};

struct Fog 
{
    float minDist;            
    float maxDist;            
    vec3 color;            
};

in vec3 Position;
in vec3 Normal;
in vec2 TexCoord;
in vec3 Vec;

uniform Light light[1];
uniform Material material;
uniform mat4 V;
uniform int Pass; 
uniform Fog fog;

layout(binding=0) uniform sampler2D DiffuseTex;     // For regular objects
layout(binding=1) uniform samplerCube CubeMapTex;   // For skybox

uniform bool UseTexture;

const int CARTOON_LEVELS = 3;
const int CHECKERBOARD_SIZE = 50;
const float CHECKERBOARD_MIXLEVEL = 0.5f; 
const int DISCARD_SCALE = 10;  
const float DISCARD_RADIUS = 0.2;  

layout(location=0) out vec4 FragColor;

const float PI = 3.14159265358979323846;

float ggxDistribution(float nDotH) 
{
    float alpha2 = material.rough * material.rough * material.rough * material.rough;
    float d = (nDotH * nDotH) * (alpha2 - 1.0f) + 1.0f;
    return alpha2 / (PI * d * d);
}

float geomSmith(float nDotL) 
{
    float k = (material.rough + 1.0f) * (material.rough + 1.0f) / 8.0f;
    float denom = nDotL * (1.0f - k) + k;
    return 1.0f / denom;
}

vec3 schlickFresnel(float lDotH) 
{
    vec3 f0 = vec3(0.04f); // Dielectrics
    if (material.metal == 1.0f)
        f0 = material.color;
    return f0 + (1.0f - f0) * pow(1.0f - lDotH, 5.0);
}

vec3 microfacetModel(vec3 position, vec3 n) 
{  
    vec3 diffuseBrdf = material.color;
    
    // Sample texture if enabled
    if (UseTexture) {
        vec4 texColor = texture(DiffuseTex, TexCoord);
        diffuseBrdf *= texColor.rgb;
    }

    vec3 lightI = light[0].L;
    vec3 lightPositionInView = (V * vec4(light[0].position, 1.0f)).xyz;

    vec3 l = lightPositionInView - position;
    float dist = length(l);
    l = normalize(l);
    lightI *= 100.0 / (dist * dist);

    vec3 v = normalize(-position);
    vec3 h = normalize(v + l);
    float nDotH = dot(n, h);
    float lDotH = dot(l, h);
    float nDotL = max(dot(n, l), 0.0f);
    float nDotV = dot(n, v);
    vec3 specBrdf = 0.25f * ggxDistribution(nDotH) * schlickFresnel(lDotH) 
                            * geomSmith(nDotL) * geomSmith(nDotV);

    return (diffuseBrdf + PI * specBrdf) * lightI * nDotL;
}

float cartoon(float value)
{
    const int levels = CARTOON_LEVELS;
    const float scaleFactor = 1.0f / float(levels);
    return floor(value * float(levels)) * scaleFactor;
}

vec3 blinnPhongCartoon(vec3 position, vec3 n)
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

    vec3 ambient = light[0].La * material.Ka;
    vec3 diffuse = light[0].Ld * material.Kd * cartoon(sDotN);
    vec3 specular = light[0].Ls * material.Ks * cartoon(pow(max(hDotN, 0.0f), material.shininess));

    if (materialType == 0){
        diffuse *= attenuation;
        specular *= attenuation;
    }
   
    return ambient + diffuse + specular;
}

vec3 BlinnPhong(vec3 position, vec3 normal, Light light, Material material, mat4 view)
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

    vec3 ambient = light.La * material.Ka;
    vec3 diffuse = light.Ld * material.Kd * NdotL;
    vec3 specular = (NdotL > 0.0f) ? light.Ls * material.Ks * pow(NdotH, material.shininess) : vec3(0.0f);

    // Sample texture if enabled
    if (UseTexture) {
        vec4 texColor = texture(DiffuseTex, TexCoord);
        diffuse *= texColor.rgb;
    }

    return ambient + diffuse + specular;
}

vec2 vec2uv(vec3 v)
{
    if (abs(v.y) > max(abs(v.x), abs(v.z)))
        return vec2(0.0f, 0.0f);

    float absX = abs(v.x);
    float absZ = abs(v.z);
    vec2 uv;

    if (absX > absZ) {
        float t = 1.0f / absX;
        vec3 P = t * v;
        if (v.x > 0.0f) {
            uv = vec2((P.z + 1.0f) * 0.5f, (P.y + 1.0f) * 0.5f);
        } else {
            uv = vec2((1.0f - P.z) * 0.5f, (P.y + 1.0f) * 0.5f);
        }
    }
    else if (absZ > absX) {
        float t = 1.0f / absZ;
        vec3 P = t * v;
        if (v.z < 0.0f) {
            uv = vec2((P.x + 1.0f) * 0.5f, (P.y + 1.0f) * 0.5f);
        } else {
            uv = vec2((1.0f - P.x) * 0.5f, (P.y + 1.0f) * 0.5f);
        }
    }
    else {
        if (v.z < 0.0f) {
            if (v.x < 0.0f) {
                uv = vec2((v.x + 1.0f) * 0.5f, (v.y + 1.0f) * 0.5f);
            } else {
                uv = vec2((v.z + 1.0f) * 0.5f, (v.y + 1.0f) * 0.5f);
            }
        } else {
            if (v.x >= 0.0f) {
                uv = vec2((1.0f - v.x) * 0.5f, (v.y + 1.0f) * 0.5f);
            } else {
                uv = vec2((v.z + 1.0f) * 0.5f, (v.y + 1.0f) * 0.5f);
            }
        }
    }
    return uv;
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
    vec3 color;

    if (materialType == 0) {
        color = microfacetModel(Position, normalize(Normal));    
    } else if (materialType == 1) {
        color = BlinnPhong(Position, normalize(Normal), light[0], material, V);
    }

    if (material.effect == 1.0f) {
        vec2 centers[3] = vec2[3](vec2(0.3f, 0.3f), vec2(0.7f, 0.3f), vec2(0.5f, 0.7f));
        float radius = DISCARD_RADIUS;
        for (int i = 0; i < 3; ++i) {
            float dist = distance(TexCoord, centers[i]);
            if (dist < radius)
                discard;
        }
    } else if (material.effect == 2.0f) {
        color = blinnPhongCartoon(Position, normalize(Normal));
    } else if (material.effect == 3.0f) {
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
}