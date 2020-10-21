#version 330 core

uniform vec4 object_color;
uniform vec3 light;
uniform vec3 light_color;
uniform vec3 camera;

out     vec4      color;

void main() {
   color = object_color;
}
