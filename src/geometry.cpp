#include "geometry.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// #define OGL_DEBUG
auto randf() { return (float)rand()/(float)RAND_MAX; }

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

auto make_vao(const std::vector<point>& tris) {
    log_info("Setting up VAO");
    unsigned vao = 0;
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
    return vao;
}

auto make_program() {
    log_info("Setting up shader program");
    auto vertex_shader   = make_shader("glsl/vertex.glsl",   GL_VERTEX_SHADER);
    auto fragment_shader = make_shader("glsl/fragment.glsl", GL_FRAGMENT_SHADER);

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

auto make_lut(std::vector<glm::vec4> colors) {
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

geometry::geometry(): program{make_program()} { }

void geometry::maybe_make_fbo(int w, int h) {
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

void geometry::render(float zoom, float phi, float width, float height, std::vector<renderable> to_render, std::vector<renderable> billboard) {
    if (triangles.size() == 0) return;
    gl_check_error("render init");
    glUseProgram(program);
    {
        // Set up model matrix and transfer to shader
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, translation);
        model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        int model_loc = glGetUniformLocation(program, "model");
        gl_check_error("render get model loc");

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
        log_debug("Rendering {} regions.", to_render.size());
        for (const auto& [tris, vao, tex, active, color]: to_render) {
            if (active) {
                glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
                // Push our LUT texture to the GPU
                GLuint tex_loc = glGetUniformLocation(program, "lut");
                glUniform1i(tex_loc, 0);
                glActiveTexture(GL_TEXTURE0 + 0);
                glBindTexture(GL_TEXTURE_1D, tex);
                // now render geometry
                glBindVertexArray(vao);
                glDrawArrays(GL_TRIANGLES, 0, tris.size());
                glBindVertexArray(0);
            }
        }
    }
    {
        // glDisable(GL_DEPTH_TEST);
        // Set up model matrix and transfer to shader
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, phi, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(model, translation);
        model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        int model_loc = glGetUniformLocation(program, "model");
        gl_check_error("render get model loc");

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

        // log_debug("Rendering {} billboards", billboard.size());
        for (const auto& [tris, vao, tex, active, color]: billboard) {
            if (active) {
                glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
                // Push our LUT texture to the GPU
                GLuint tex_loc = glGetUniformLocation(program, "lut");
                glUniform1i(tex_loc, 0);
                glActiveTexture(GL_TEXTURE0 + 0);
                glBindTexture(GL_TEXTURE_1D, tex);
                // now render geometry
                glBindVertexArray(vao);
                glDrawArrays(GL_TRIANGLES, 0, tris.size());
                glBindVertexArray(0);
            }
        }
        glUseProgram(0);
        gl_check_error("render end");
    }
}

renderable geometry::make_markers(const std::vector<point>& points, glm::vec4 color) {
    std::vector<point> tris;
    for (const auto& marker: points) {
        auto p = point{(marker.x - root.x)/rescale,
        (marker.y - root.y)/rescale,
        (marker.z - root.z)/rescale,
        0.0f};
        auto dx = 1.0f/40.0f;
        auto dy = dx/2.0f;
        // tris.push_back({0, 0, 0, 0}); tris.push_back({dx, 0, 0, 0}); tris.push_back({dx, dx, 0, 0});
        // tris.push_back({0, 0, 0, 0}); tris.push_back({dx, 0, 0, 0}); tris.push_back({dx, 0, dx, 0});
        tris.push_back({p.x, p.y, p.z, 0}); tris.push_back({p.x + dy, p.y + dx, p.z, 0}); tris.push_back({p.x + dx, p.y + dy, 0, 0});
        // tris.push_back({0, 0, 0, 0}); tris.push_back({0, dx, 0, 0}); tris.push_back({0, dx, dx, 0});
        // tris.push_back({0, 0, 0, 0}); tris.push_back({0, 0, dx, 0}); tris.push_back({dx, 0, dx, 0});
        // tris.push_back({0, 0, 0, 0}); tris.push_back({0, 0, dx, 0}); tris.push_back({0, dx, dx, 0});
    }
    auto vao = make_vao(tris);
    auto tex = make_lut({color});
    return {std::move(tris), vao, tex, true};
}

renderable geometry::derive_renderable(const std::vector<arb::msegment>& segments, glm::vec4 color) {
    std::vector<point> tris{};
    for (const auto& seg: segments) {
        const auto id  = seg.id;
        const auto idx = id_to_index[id];
        for (auto idy = 6*(n_faces + 1)*idx; idy < 6*(n_faces + 1)*(idx + 1); ++idy) {
            auto tri = triangles[idy];
            tri.tag = 0; // NOTE here we could make more colors happen
            tris.push_back(std::move(tri));
        }
    }
    auto vao = make_vao(tris);
    auto tex = make_lut({color});
    return {std::move(tris), vao, tex, true};
}

void geometry::load_geometry(arb::segment_tree& tree) {
    log_info("Making geometry");
    auto tmp = tree.segments()[0].prox;
    root = {(float) tmp.x, (float) tmp.y, (float) tmp.z, 0.0f}; // is always the root
    size_t index = 0;
    for (const auto& [id, prox, dist, tag]: tree.segments()) {
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
        const auto dphi = 2.0f*PI/n_faces;
        const auto rot  = glm::mat3(glm::rotate(glm::mat4(1.0f), dphi, c_dif));
        for (auto rx = 0ul; rx <= n_faces; ++rx) {
            auto v00 = static_cast<float>(prox.radius)*normal + c_prox;
            auto v01 = static_cast<float>(dist.radius)*normal + c_dist;
            normal = rot*normal;
            auto v10 = static_cast<float>(prox.radius)*normal + c_prox;
            auto v11 = static_cast<float>(dist.radius)*normal + c_dist;

            triangles.push_back({v00.x, v00.y, v00.z, 0.0f});
            triangles.push_back({v01.x, v01.y, v01.z, 0.0f});
            triangles.push_back({v10.x, v10.y, v10.z, 0.0f});
            triangles.push_back({v11.x, v11.y, v11.z, 0.0f});
            triangles.push_back({v01.x, v01.y, v01.z, 0.0f});
            triangles.push_back({v10.x, v10.y, v10.z, 0.0f});
        }
        id_to_index[id] = index;
        index++;
    }

    // Re-scale into [-1, 1]^3 box
    log_debug("Cylinders generated: {} ({} points)", triangles.size()/n_faces/2, triangles.size());
    for (auto& tri: triangles) {
        rescale = std::max(rescale, std::abs(tri.x));
        rescale = std::max(rescale, std::abs(tri.y));
        rescale = std::max(rescale, std::abs(tri.z));
    }
    for(auto& tri: triangles) {
        tri.x /= rescale;
        tri.y /= rescale;
        tri.z /= rescale;
    }
    log_debug("Geometry re-scaled by 1/{}", rescale);
    log_info("Making geometry: completed");
}
