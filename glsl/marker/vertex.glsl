#version 410 core

layout (location = 0) in vec3  pos;
layout (location = 1) in vec3  nrm;
layout (location = 2) in vec3  obj;
layout (location = 3) in vec3  off;

uniform mat4  model;
uniform mat4  view;
uniform float scale;

void main() {
    gl_Position =  view * model * vec4(off, 1.0f) + view * vec4(scale * pos, 0.0f);
}
