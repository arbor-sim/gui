#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct point {
    float x, y, z;
    float tag;
};

auto make_shader(const std::string& fn, unsigned shader_type) {
    log_info("Loading shader {}", fn);
    std::ifstream fd(fn);
    std::string src(std::istreambuf_iterator<char>(fd), {});
    auto src_str = src.c_str();

    unsigned int shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &src_str, NULL);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        char info[512];
        glGetShaderInfoLog(shader, sizeof(info), NULL, info);
        log_error("Shader {} failed to compile\n{}", fn, info);
    }
    log_info("Loading shader {}: complete", fn);
    return shader;
}

struct geometry {
    std::vector<point> tris;
    unsigned vao = 0;
    unsigned program = 0;

    // Model transformations
    glm::vec3 translation = {0.0f, 0.0f, 0.0f};
    glm::vec3 rotation    = {0.0f, 0.0f, 0.0f};
    glm::vec3 scale       = {1.0f, 1.0f, 1.0f};

    // Camera
    float distance   = 1.75f;
    glm::vec3 camera = {distance, 0.0f, 0.0f};
    glm::vec3 up     = {0.0f, 1.0f, 0.0f};
    glm::vec3 target = {0.0f, 0.0f, 0.0f};


    geometry(parameters& p) {
        load_geometry(p);
        make_vao();
        make_program();
    }

    void make_program() {
        log_info("Setting up shader program");
        auto vertex_shader   = make_shader("glsl/vertex.glsl",   GL_VERTEX_SHADER);
        auto fragment_shader = make_shader("glsl/fragment.glsl", GL_FRAGMENT_SHADER);

        program = glCreateProgram();
        glAttachShader(program, vertex_shader);
        glAttachShader(program, fragment_shader);
        glLinkProgram(program);

        int success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if(!success) {
            char info[512];
            glGetProgramInfoLog(program, 512, NULL, info);
            log_error("Shader program failed to link\n{}", info);
        }
        log_info("Setting up shader program: complete");
    }

    void make_vao() {
        log_info("Setting up VAO");
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        unsigned int VBO;
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferData(GL_ARRAY_BUFFER, tris.size()*sizeof(point), tris.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(point), (void*) nullptr);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(point), (void*) (offsetof(point, tag)));
        glEnableVertexAttribArray(1);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        log_info("Setting up VAO: complete");
    }

    void render(float zoom, float phi, float width, float height) {

        // Set up model matrix and transfer to shader
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, translation);
        model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        int model_loc = glGetUniformLocation(program, "model");
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));

        // Set up camera and push
        camera.x = distance*std::sin(phi);
        camera.y = distance*0.0f;
        camera.z = distance*std::cos(phi);

        glm::mat4 view = glm::lookAt(camera, target, up);
        int view_loc = glGetUniformLocation(program, "view");
        glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));

        // Set up projection and push
        glm::mat4 proj = glm::perspective(glm::radians(zoom), width/height, 0.1f, 100.0f);
        int proj_loc = glGetUniformLocation(program, "proj");
        glUniformMatrix4fv(proj_loc, 1, GL_FALSE, glm::value_ptr(proj));

        // Render
        glUseProgram(program);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, tris.size());
    }

    void load_geometry(parameters& p) {
        log_info("Making geometry");
        auto N = 8;
        auto root = p.swc[0];
        for (auto ix = 0ul; ix < p.swc.size(); ++ix) {
            auto rec = p.swc[ix];
            if (rec.parent_id >= 0) {
                auto par = p.swc[rec.parent_id];

                auto c_par = glm::vec3{(par.x - root.x), (par.y - root.y), (par.z - root.z)};
                auto c_rec = glm::vec3{(rec.x - root.x), (rec.y - root.y), (rec.z - root.z)};
                auto c_dif = c_par - c_rec;

                float r_rec = rec.r;
                float r_par = par.r;

                auto n = glm::vec3{0, 0, 0};
                for (; n == glm::vec3{0, 0, 0};) {
                    auto t = glm::vec3{(float)rand()/(float)RAND_MAX, (float)rand()/(float)RAND_MAX, (float)rand()/(float)RAND_MAX};
                    n = glm::normalize(glm::cross(c_dif, t));
                }

                for (auto rx = 0ul; rx < N; ++rx) {
                    auto rot0 = glm::rotate(glm::mat4(1.0f),  rx     *2.0f*3.141f/N, c_dif);
                    auto rot1 = glm::rotate(glm::mat4(1.0f), (rx + 1)*2.0f*3.141f/N, c_dif);
                    auto v00 = r_par*glm::mat3(rot0)*n + c_par;
                    auto v01 = r_rec*glm::mat3(rot0)*n + c_rec;
                    auto v10 = r_par*glm::mat3(rot1)*n + c_par;
                    auto v11 = r_rec*glm::mat3(rot1)*n + c_rec;

                    tris.push_back({v00.x, v00.y, v00.z, (float) par.tag});
                    tris.push_back({v01.x, v01.y, v01.z, (float) rec.tag});
                    tris.push_back({v10.x, v10.y, v10.z, (float) par.tag});
                    tris.push_back({v11.x, v11.y, v11.z, (float) rec.tag});
                    tris.push_back({v01.x, v01.y, v01.z, (float) rec.tag});
                    tris.push_back({v10.x, v10.y, v10.z, (float) par.tag});
                }
            }
        }
        log_debug("Cylinders generated: {} ({} points)", p.swc.size(), tris.size());
        auto scale = 0.0f;
        for (auto& tri: tris) {
            scale = std::max(scale, std::abs(tri.x));
            scale = std::max(scale, std::abs(tri.y));
            scale = std::max(scale, std::abs(tri.z));
        }
        for(auto& tri: tris) {
            tri.x /= scale;
            tri.y /= scale;
            tri.z /= scale;
        }
        log_debug("Geometry re-scaled by 1/{}", scale);
        log_info("Making geometry: completed");
    }
};
