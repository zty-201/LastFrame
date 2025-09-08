/////////////////////////////////////////////////////////////////////////
// Pixel shader for lighting
////////////////////////////////////////////////////////////////////////
#version 330

out vec4 FragColor;

// These definitions agree with the ObjectIds enum in scene.h
const int     nullId	= 0;
const int     skyId	= 1;
const int     seaId	= 2;
const int     groundId	= 3;
const int     treeId	= 4;
const int     debugId	= 5;

float pi = 3.14159;
float pi2 = 2*pi;

in vec3 normalVec, lightVec, eyeVec;
in vec2 texCoord;

uniform int objectId;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;

void main()
{
    if (objectId == debugId) {
        FragColor.xyz = diffuse;
        return; }
    
    vec3 ONE = vec3(1.0, 1.0, 1.0);
    vec3 N = normalize(normalVec);
    vec3 L = normalize(lightVec);
    vec3 V = normalize(eyeVec);
    vec3 H = normalize(L+V);
    float NL = max(dot(N,L),0.0);
    float NV = max(dot(N,V),0.0);
    float HN = max(dot(H,N),0.0);

    vec3 I = ONE;
    vec3 Ia = 0.2*ONE;
    vec3 Kd = diffuse; 
    
    // A checkerboard pattern to break up larte flat expanses.  Remove when using textures.
    if (objectId==groundId || objectId==seaId) {
        ivec2 uv = ivec2(floor(100.0*texCoord));
        if ((uv[0]+uv[1])%2==0)
            Kd *= 0.9; }
    
   // Lighting is diffuse + ambient + specular
    vec3 fragColor = Ia*Kd;
    fragColor += I*Kd*NL;
    fragColor += I*specular*pow(HN,shininess); 
    FragColor.xyz = fragColor;
}
