#version 330 core

layout (location = 0) in vec3  pos;
layout (location = 1) in vec3  nrm;
layout (location = 2) in vec3  off;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main() {
    gl_Position =  proj * view * (model * vec4(pos, 1.0f) + vec4(off, 0.0f));
}
