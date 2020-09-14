#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// #define OGL_DEBUG

auto randf() { return (float)rand()/(float)RAND_MAX; }

struct point {
    float x, y, z;
    float tag;
};

void gl_check_error(const std::string& where) {
    #ifdef OGL_DEBUG
    auto rc = glGetError();
    if (rc != GL_NO_ERROR) {
        log_error("OpenGL error @ {}: {}", where, rc);
    } else {
        log_debug("OpenGL OK @ {}", where);
    }
    #endif
}

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
    std::vector<point> triangles;
    std::vector<int> tags;
    std::unordered_map<size_t, size_t> id_to_index;

    unsigned vao     = 0;
    unsigned fbo     = 0;
    unsigned tex     = 0;
    unsigned program = 0;

    size_t n_faces = 8;

    int width = -1;
    int height = -1;

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

        glBufferData(GL_ARRAY_BUFFER, triangles.size()*sizeof(point), triangles.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(point), (void*) nullptr);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(point), (void*) (offsetof(point, tag)));
        glEnableVertexAttribArray(1);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        log_info("Setting up VAO: complete");
    }

    void maybe_make_fbo(int w, int h) {
        gl_check_error("make fbo init");
        glViewport(0, 0, w, h);

        if ((w == width) && (h == height)) return;
        log_debug("Resizing {}x{}", w, h);
        width = w;
        height = h;

        if (fbo) glDeleteFramebuffers(1, &fbo);
        fbo = 0;

        if (tex) glDeleteTextures(1, &tex);
        tex = 0;

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        // create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
        unsigned int rbo;
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) log_error("FBO incomplete");
        // glBindFramebuffer(GL_FRAMEBUFFER, 0);
        gl_check_error("end fbo init");
    }

    void render(float zoom, float phi, float width, float height) {
        gl_check_error("render init");
        glUseProgram(program);
        // Set up model matrix and transfer to shader
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, translation);
        model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        int model_loc = glGetUniformLocation(program, "model");
        gl_check_error("render get model loc");
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
        // gl_check_error("render set model");

        // Set up camera and push
        camera = distance*glm::vec3{std::sin(phi), 0.0f, std::cos(phi)};
        glm::mat4 view = glm::lookAt(camera, target, up);
        int view_loc = glGetUniformLocation(program, "view");
        glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
        gl_check_error("render set view");

        // Set up projection and push
        glm::mat4 proj = glm::perspective(glm::radians(zoom), width/height, 0.1f, 100.0f);
        int proj_loc = glGetUniformLocation(program, "proj");
        glUniformMatrix4fv(proj_loc, 1, GL_FALSE, glm::value_ptr(proj));
        gl_check_error("render set projection");

        // Render
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, triangles.size());
        glBindVertexArray(0);
        glUseProgram(0);
        gl_check_error("render end");
    }

    void load_geometry(parameters& p) {
        log_info("Making geometry");
        const auto& root = p.tree.segments()[0].prox;
        size_t index = 0;
        for (const auto& [id, prox, dist, tag]: p.tree.segments()) {
            // Shift to root and find vector along the segment
            const auto c_prox = glm::vec3{(prox.x - root.x), (prox.y - root.y), (prox.z - root.z)};
            const auto c_dist = glm::vec3{(dist.x - root.x), (dist.y - root.y), (dist.z - root.z)};
            const auto c_dif = c_prox - c_dist;

            // Make a normal to segment vector
            auto normal = glm::vec3{0.0, 0.0, 0.0};
            for (; normal == glm::vec3{0.0, 0.0, 0.0};) {
                auto t = glm::vec3{ randf(), randf(), randf()};
                normal = glm::normalize(glm::cross(c_dif, t));
            }

            // Generate cylinder from n_faces triangles
            const auto dphi = 2.0f*3.141f/n_faces;
            const auto rot  = glm::mat3(glm::rotate(glm::mat4(1.0f), dphi, c_dif));
            for (auto rx = 0ul; rx <= n_faces; ++rx) {
                auto v00 = static_cast<float>(prox.radius)*normal + c_prox;
                auto v01 = static_cast<float>(dist.radius)*normal + c_dist;
                normal = rot*normal;
                auto v10 = static_cast<float>(prox.radius)*normal + c_prox;
                auto v11 = static_cast<float>(dist.radius)*normal + c_dist;

                triangles.push_back({v00.x, v00.y, v00.z, (float) tag});
                triangles.push_back({v01.x, v01.y, v01.z, (float) tag});
                triangles.push_back({v10.x, v10.y, v10.z, (float) tag});
                triangles.push_back({v11.x, v11.y, v11.z, (float) tag});
                triangles.push_back({v01.x, v01.y, v01.z, (float) tag});
                triangles.push_back({v10.x, v10.y, v10.z, (float) tag});
            }
            id_to_index[id] = index;
            index++;
            tags.push_back(tag);
        }

        // Re-scale into [-1, 1]^3 box
        log_debug("Cylinders generated: {} ({} points)", triangles.size()/n_faces/2, triangles.size());
        auto scale = 0.0f;
        for (auto& tri: triangles) {
            scale = std::max(scale, std::abs(tri.x));
            scale = std::max(scale, std::abs(tri.y));
            scale = std::max(scale, std::abs(tri.z));
        }
        for(auto& tri: triangles) {
            tri.x /= scale;
            tri.y /= scale;
            tri.z /= scale;
        }
        log_debug("Geometry re-scaled by 1/{}", scale);
        log_info("Making geometry: completed");
    }

    void set_colour_by_tag() {
        for (auto idx = 0ul; idx < tags.size(); ++idx) {
            const auto tag = tags[idx];
            for (auto idy = 6*(n_faces + 1)*idx; idy < 6*(idx + 1)*(n_faces + 1); ++idy) {
                triangles[idy].tag = tag;
            }
        }
        make_vao();
    }

    void set_colour_by_region(const std::vector<arb::msegment>& segments) {
        for (auto& tri: triangles) tri.tag = 0.0;
        for (const auto& seg: segments) {
            const auto id  = seg.id;
            const auto idx = id_to_index[id];
            for (auto idy = 6*(n_faces + 1)*idx; idy < 6*(n_faces + 1)*(idx + 1); ++idy) {
                triangles[idy].tag = 1.0;
            }
        }
        make_vao();
    }
};
