#version 330 core

out     vec4      color;
in      float     index;
uniform sampler1D lut;

void main() {
   color = texture(lut, index).rgba;
}
