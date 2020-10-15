#version 330 core

in      float     index;

uniform vec4 object_color;

out     vec4      color;

void main() {
   color = object_color;
}
