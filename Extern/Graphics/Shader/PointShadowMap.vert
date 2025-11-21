#version 310 es
precision highp float;

layout(location = 0) in vec3 a_Position;

uniform mat4 u_LightMatrix;
uniform mat4 u_Model;

out vec3 FragPos;

void main() {
    FragPos = vec3(u_Model * vec4(a_Position, 1.0));
    gl_Position = u_LightMatrix * vec4(FragPos, 1.0);
}