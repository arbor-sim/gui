#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 nrm;
layout (location = 2) in vec3 off;
layout (location = 3) in vec3 obj;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec3 normal;
out vec3 position;
out vec3 id;

void main() {
    gl_Position =  proj * view * (model * vec4(pos, 1.0f) + vec4(off, 0.0f));
    position = vec3(model * vec4(pos, 1.0f));
    id = obj;
}
