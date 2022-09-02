#version 410 core

layout (location = 0) in vec3  pos;
layout (location = 1) in vec3  nrm;
layout (location = 2) in vec3  obj;
layout (location = 3) in vec3  off;
layout (location = 4) in float col;

uniform mat4 model;
uniform mat4 view;

out vec3 normal;
out vec3 position;
out float alpha;

void main() {
    alpha = col;
    gl_Position = view*model*vec4(pos, 1.0f);
    normal = nrm;
    position = vec3(model*vec4(pos, 1.0f));
}
