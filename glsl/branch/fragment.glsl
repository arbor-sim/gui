#version 330 core

in  vec3 id;
out vec4 color;

void main() {
    color = vec4(id, 1.0f);
}
