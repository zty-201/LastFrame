#version 330

uniform mat4 WorldView, WorldInverse, WorldProj, ModelTr, NormalTr;
uniform vec3 lightPos;

in vec4 vertex;
in vec3 vertexNormal;

out vec3 normalVec, lightVec, eyeVec;

void main()
{
    vec3 eye = (WorldInverse*vec4(0,0,0,1)).xyz;
    gl_Position = WorldProj*WorldView*ModelTr*vertex;
    
    vec3 worldPos = (ModelTr*vertex).xyz;
    normalVec = mat3(NormalTr) * vertexNormal; 
    lightVec = lightPos - worldPos;
    eyeVec = eye - worldPos;
}