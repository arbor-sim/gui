#include "geometry.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

auto randf() { return (float)rand()/(float)RAND_MAX; }

void gl_check_error(const std::string& where) {
#ifndef NDEBUG
    auto rc = glGetError();
    if (rc != GL_NO_ERROR) {
        log_error("OpenGL error @ {}: {}", where, rc);
    } else {
//        log_debug("OpenGL OK @ {}", where);
    }
#endif
}

void set_uniform(unsigned program, const std::string& name, const glm::vec3& data) {
    auto loc = glGetUniformLocation(program, name.c_str());
    glUniform3fv(loc, 1, glm::value_ptr(data));
    gl_check_error(fmt::format("setting uniform vec3: {}", name));
}

void set_uniform(unsigned program, const std::string& name, const glm::vec4& data) {
    auto loc = glGetUniformLocation(program, name.c_str());
    glUniform4fv(loc, 1, glm::value_ptr(data));
    gl_check_error(fmt::format("setting uniform vec4: {}", name));
}

void set_uniform(unsigned program, const std::string& name, const glm::mat4& data) {
    auto loc = glGetUniformLocation(program, name.c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(data));
    gl_check_error(fmt::format("setting uniform mat4: {}", name));
}

void render(unsigned program,
            glm::mat4 model,
            glm::mat4 view,
            glm::mat4 proj,
            glm::vec3 camera,
            glm::vec3 light,
            glm::vec3 light_color,
            std::vector<renderable> render) {
    gl_check_error("render init");
    glUseProgram(program);
    set_uniform(program, "model", model);
    set_uniform(program, "view", view);
    set_uniform(program, "proj", proj);
    set_uniform(program, "camera", camera);
    set_uniform(program, "light", light);
    set_uniform(program, "light_color", light_color);
    // Render
    for (const auto& [count, instances, vao, active, color]: render) {
        if (active) {
            set_uniform(program, "object_color", color);
            glBindVertexArray(vao);
            glDrawArraysInstanced(GL_TRIANGLES, 0, count, instances);
            glBindVertexArray(0);
        }
    }
    glUseProgram(0);
    gl_check_error("render end");
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

auto make_vao_instanced(const std::vector<point>& tris, const std::vector<glm::vec3> offset) {
    log_info("Setting up VAO");
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, tris.size()*sizeof(point), tris.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(point), (void*) (offsetof(point, position)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(point), (void*) (offsetof(point, normal)));
    glEnableVertexAttribArray(1);

    unsigned int instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(point)*offset.size(), offset.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribDivisor(2, 1);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    log_info("Setting up VAO: complete");
    return vao;
}

auto make_vao(const std::vector<point>& tris) {
    return make_vao_instanced(tris, {{0.0f, 0.0f, 0.0f}});
}

auto make_program(const std::string& dn) {
    log_info("Setting up shader program");
    auto vertex_shader   = make_shader(fmt::format("glsl/{}/vertex.glsl", dn), GL_VERTEX_SHADER);
    auto fragment_shader = make_shader(fmt::format("glsl/{}/fragment.glsl", dn), GL_FRAGMENT_SHADER);

    unsigned program = glCreateProgram();
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
    return program;
}

unsigned make_lut(std::vector<glm::vec4> colors) {
    // make and bind texture
    unsigned lut = 0;
    glGenTextures(1, &lut);
    glBindTexture(GL_TEXTURE_1D, lut);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, colors.size(), 0, GL_RGBA, GL_FLOAT, colors.data());
    // set texture filtering
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // unbind
    glBindTexture(GL_TEXTURE_1D, 0);
    return lut;
}

geometry::geometry():
    region_program{make_program("region")},
    marker_program{make_program("marker")} {}

geometry::geometry(const arb::segment_tree& tree):
    region_program{make_program("region")},
    marker_program{make_program("marker")} {
    load_geometry(tree);
}

void geometry::maybe_make_fbo(int w, int h) {
    gl_check_error("make fbo init");
    glViewport(0, 0, w, h);

    if ((w == width) && (h == height)) return;
    log_debug("Resizing {}x{}", w, h);
    width = w;
    height = h;

    if (fbo) { glDeleteFramebuffers(1, &fbo); fbo = 0; }
    if (tex) { glDeleteTextures(1, &tex); tex = 0; }

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl_check_error("mk fbo");
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    gl_check_error("mk tex");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    gl_check_error("set tex to fbo");
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    gl_check_error("mk rbo");
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
    gl_check_error("set rbo to fbo");
    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) log_error("FBO incomplete, status={}", status);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    gl_check_error("end fbo init");
}

unsigned long
geometry::render(float zoom,
                 float phi,
                 float width, float height,
                 const std::vector<renderable>& regions,
                 const std::vector<renderable>& markers) {
    // re-build fbo, if needed
    maybe_make_fbo(width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // Set up transformations
    // * view
    float distance   = 2.5f;
    auto camera      = distance*glm::vec3{std::sin(phi), 0.0f, std::cos(phi)};
    glm::vec3 up     = {0.0f, 1.0f, 0.0f};
    glm::vec3 target = {0.0f, 0.0f, 0.0f};
    glm::mat4 view = glm::lookAt(camera, target, up);
    // * projection
    glm::mat4 proj = glm::perspective(glm::radians(zoom), width/height, 0.1f, 100.0f);

    auto light = camera;
    auto light_color = glm::vec3{1.0f, 1.0f, 1.0f};

    {
        glm::mat4 model = glm::mat4(1.0f);
        ::render(region_program, model, view, proj, camera, light, light_color, regions);
    }
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, phi, glm::vec3(0.0f, 1.0f, 0.0f));
        glDisable(GL_DEPTH_TEST);
        ::render(marker_program, model, view, proj, camera, light, light_color, markers);
        glEnable(GL_DEPTH_TEST);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return static_cast<unsigned long>(tex);
}

renderable geometry::make_marker(const std::vector<glm::vec3>& points, glm::vec4 color) {
    std::vector<glm::vec3> off;
    for (const auto& marker: points) {
        off.emplace_back((marker.x - root.x)/rescale,
                         (marker.y - root.y)/rescale,
                         (marker.z - root.z)/rescale);
    }
    auto dx = 1.0f/40.0f;
    auto dy = dx/2.0f;
    std::vector<point> tris{{{0.0f,      0.0f,      0.0f}, {0.0f, 0.0f, 0.0f}},
                            {{0.0f + dy, 0.0f + dx, 0.0f}, {0.0f, 0.0f, 0.0f}},
                            {{0.0f + dx, 0.0f + dy, 0.0f}, {0.0f, 0.0f, 0.0f}}};

    auto vao = make_vao_instanced(tris, off);
    return {tris.size(), off.size(), vao, true, color};
}

renderable geometry::make_region(const std::vector<arb::msegment>& segments, glm::vec4 color) {
    std::vector<point> tris{};
    for (const auto& seg: segments) {
        const auto id  = seg.id;
        const auto idx = id_to_index[id];
        for (auto idy = 6*(n_faces + 1)*idx; idy < 6*(n_faces + 1)*(idx + 1); ++idy) {
            tris.push_back(triangles[idy]);
        }
    }
    auto vao = make_vao(tris);
    return {tris.size(), 1, vao, true, color};
}

void geometry::load_geometry(const arb::segment_tree& tree) {
    log_info("Making geometry");
    if (tree.segments().size() == 0) {
        log_info("Empty!");
        return;
    }
    auto tmp = tree.segments()[0].prox;
    root = {(float) tmp.x, (float) tmp.y, (float) tmp.z}; // is always the root
    size_t index = 0;
    for (const auto& [id, prox, dist, tag]: tree.segments()) {
        // Shift to root and find vector along the segment
        const auto c_prox = glm::vec3{(prox.x - root.x), (prox.y - root.y), (prox.z - root.z)};
        const auto c_dist = glm::vec3{(dist.x - root.x), (dist.y - root.y), (dist.z - root.z)};
        const auto c_dif = c_prox - c_dist;

        // Make a normal to segment vector
        auto normal = glm::vec3{0.0, 0.0, 0.0};
        for (; normal == glm::vec3{0.0, 0.0, 0.0};) {
            auto t = glm::vec3{randf(), randf(), randf()};
            normal = glm::normalize(glm::cross(c_dif, t));
        }

        // Generate cylinder from n_faces triangles
        const auto dphi = 2.0f*PI/n_faces;
        const auto rot  = glm::mat3(glm::rotate(glm::mat4(1.0f), dphi, c_dif));
        for (auto rx = 0ul; rx <= n_faces; ++rx) {
            auto normal_next = rot*normal;
            auto v00 = static_cast<float>(prox.radius)*normal + c_prox;
            auto v01 = static_cast<float>(dist.radius)*normal + c_dist;
            auto v10 = static_cast<float>(prox.radius)*normal_next + c_prox;
            auto v11 = static_cast<float>(dist.radius)*normal_next + c_dist;

            triangles.push_back({v00, normal});
            triangles.push_back({v01, normal});
            triangles.push_back({v10, normal_next});
            triangles.push_back({v11, normal_next});
            triangles.push_back({v01, normal});
            triangles.push_back({v10, normal_next});

            normal = normal_next;
        }
        id_to_index[id] = index;
        index++;
    }

    // Re-scale into [-1, 1]^3 box
    log_debug("Cylinders generated: {} ({} points)", triangles.size()/n_faces/2, triangles.size());
    for (auto& tri: triangles) {
        rescale = std::max(rescale, std::abs(tri.position.x));
        rescale = std::max(rescale, std::abs(tri.position.y));
        rescale = std::max(rescale, std::abs(tri.position.z));
    }
    for(auto& tri: triangles) {
        tri.position.x /= rescale;
        tri.position.y /= rescale;
        tri.position.z /= rescale;
    }
    log_debug("Geometry re-scaled by 1/{}", rescale);
    log_info("Making geometry: completed");
}
