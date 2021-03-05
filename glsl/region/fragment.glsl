#version 330 core

in vec3  position;
in vec3  normal;

uniform vec3 light;
uniform vec3 camera;
uniform vec3 light_color;
uniform vec4 object_color;

out vec4 color;

void main() {
    // ambient
    float ambient_str = 0.3f;
    vec3 ambient = ambient_str*light_color;

    // diffuse
    vec3 norm      = normalize(normal);
    vec3 light_dir = normalize(light - position);
    float diff     = max(dot(norm, light_dir), 0.0f);
    vec3 diffuse   = diff*light_color;

    // specular
    float specular_str = 0.05f;
    vec3 view_dir    = normalize(camera - position);
    vec3 reflect_dir = reflect(-light_dir, norm);
    float spec       = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
    vec3 specular    = specular_str * spec * light_color;

    color = vec4(ambient + diffuse + specular, 1.0f) * object_color;
}
