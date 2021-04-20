#version 330 core

in vec3 position;
in vec3 normal;
in vec3 id;

uniform vec3 key;
uniform vec3 key_color;
uniform vec3 back;
uniform vec3 back_color;
uniform vec3 fill;
uniform vec3 fill_color;
uniform vec3 camera;
uniform vec4 object_color;

out vec4 color;

void main() {
    // ambient
    float ambient_str = 0.3f;
    vec3 ambient = ambient_str*key_color;

    // diffuse
    vec3 norm     = normalize(normal);
    vec3 key_dir  = normalize(key  - position);
    vec3 fill_dir = normalize(fill - position);
    vec3 back_dir = normalize(back - position);
    vec3 diffuse = max(dot(norm, key_dir),  0.0f)*key_color
                 + max(dot(norm, back_dir), 0.0f)*back_color
                 + max(dot(norm, fill_dir), 0.0f)*fill_color;

    // specular
    float specular_str = 0.05f;
    vec3 view_dir    = normalize(camera - position);
    vec3 reflect_dir = reflect(-key_dir, norm);
    float spec       = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
    vec3 specular    = specular_str*spec*key_color;

    color = vec4(ambient + diffuse + specular, 1.0f)*object_color;
}
