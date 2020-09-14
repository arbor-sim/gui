#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in float tag;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

uniform float lut[18] = float[18]( 27.0f, 158.0f, 119.0f,
                                  217.0f,  95.0f,   2.0f,
                                  117.0f, 112.0f, 179.0f,
                                  231.0f,  41.0f, 138.0f,
                                  102.0f, 166.0f,  30.0f,
                                   30.0f,  30.0f,  30.0f);

out vec3 color;

void main() {
    gl_Position =  proj * view * model * vec4(pos, 1.0f);
    color = vec3(lut[3*int(tag)],
                 lut[3*int(tag) + 1],
                 lut[3*int(tag) + 2])/255.0f;
}
