#version 330 core

in vec3 position;
in vec3 normal;
in vec3 id;

uniform vec3 light;
uniform vec3 camera;
uniform vec3 light_color;
uniform vec4 object_color;

out vec4 color;

void main() {
    color = vec4(id, 1.0f);
}
