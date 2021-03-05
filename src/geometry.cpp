#include "geometry.hpp"

#include <filesystem>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

auto randf() { return (float)rand()/(float)RAND_MAX; }

glm::vec4 next_color() {
  static size_t nxt = 0;

  constexpr glm::vec4 colors[] = {{166.0f/255.0f,206.0f/255.0f,227.0f/255.0f, 1.0f},
                                  { 31.0f/255.0f,120.0f/255.0f,180.0f/255.0f, 1.0f},
                                  {178.0f/255.0f,223.0f/255.0f,138.0f/255.0f, 1.0f},
                                  { 51.0f/255.0f,160.0f/255.0f, 44.0f/255.0f, 1.0f},
                                  {251.0f/255.0f,154.0f/255.0f,153.0f/255.0f, 1.0f},
                                  {227.0f/255.0f, 26.0f/255.0f, 28.0f/255.0f, 1.0f},
                                  {253.0f/255.0f,191.0f/255.0f,111.0f/255.0f, 1.0f},
                                  {255.0f/255.0f,127.0f/255.0f,  0.0f/255.0f, 1.0f},
                                  {202.0f/255.0f,178.0f/255.0f,214.0f/255.0f, 1.0f},
                                  {106.0f/255.0f, 61.0f/255.0f,154.0f/255.0f, 1.0f},
                                  {255.0f/255.0f,255.0f/255.0f,153.0f/255.0f, 1.0f},
                                  {177.0f/255.0f, 89.0f/255.0f, 40.0f/255.0f, 1.0f}};
  constexpr size_t count = sizeof(colors)/sizeof(glm::vec4);
  log_debug("Loaded {}/{} colours.", nxt, count);
  nxt = (nxt + 1) % count;
  return colors[nxt];
}

#ifndef NDEBUG
void gl_check_error(const std::string& where) {
    auto rc = glGetError();
    if (rc != GL_NO_ERROR) {
        log_error("OpenGL error @ {}: {}", where, rc);
    }
}
#else
void gl_check_error(const std::string&) {}
#endif

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
            const map_type<renderable>& render) {
    gl_check_error("render init");
    glUseProgram(program);
    set_uniform(program, "model", model);
    set_uniform(program, "view", view);
    set_uniform(program, "proj", proj);
    set_uniform(program, "camera", camera);
    set_uniform(program, "light", light);
    set_uniform(program, "light_color", light_color);
    for (const auto& [k, v]: render) {
        if (v.active) {
            set_uniform(program, "object_color", v.color);
            glBindVertexArray(v.vao);
            glDrawArraysInstanced(GL_TRIANGLES, 0, v.count, v.instances);
            glBindVertexArray(0);
        }
    }
    glUseProgram(0);
    gl_check_error("render end");
}

auto make_shader(const std::filesystem::path& fn, unsigned shader_type) {
    log_info("Loading shader {}", fn.c_str());
    auto src = slurp(fn);
    auto c_src = src.c_str();

    unsigned int shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &c_src, NULL);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        char info[512];
        glGetShaderInfoLog(shader, sizeof(info), NULL, info);
        log_error("Shader {} failed to compile\n{}", fn.c_str(), info);
    }
    log_info("Loading shader {}: complete", fn.c_str());
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
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(point), (void*) (offsetof(point, id)));
    glEnableVertexAttribArray(3);

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

auto make_vao(const std::vector<point>& tris) { return make_vao_instanced(tris, {{0.0f, 0.0f, 0.0f}}); }

auto make_program(const std::string& dn) {
    std::filesystem::path base = ARBORGUI_RESOURCES_BASE;
    log_info("Setting up shader program");
    auto vertex_shader   = make_shader(base / "glsl" / dn / "vertex.glsl", GL_VERTEX_SHADER);
    auto fragment_shader = make_shader(base / "glsl" / dn / "fragment.glsl", GL_FRAGMENT_SHADER);

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
    object_program{make_program("branch")},
    marker_program{make_program("marker")} {}

geometry::geometry(const arb::morphology& morph):
    region_program{make_program("region")},
    object_program{make_program("branch")},
    marker_program{make_program("marker")} {
    load_geometry(morph);
}

void make_fbo(int width, int height, unsigned int& fbo, unsigned int& post_fbo, unsigned int& tex) {
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    // create multisampled color attachment texture
    unsigned int textureColorBufferMultiSampled;
    glGenTextures(1, &textureColorBufferMultiSampled);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, width, height, GL_TRUE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled, 0);
    // create multisampled renderbuffer object for depth and stencil attachments
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) log_error("Framebuffer incomplete.");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // configure post-processing framebuffer
    glGenFramebuffers(1, &post_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, post_fbo);
    // create a color attachment texture
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) log_error("Intermediate framebuffer incomplete.");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void geometry::make_fbo(int w, int h) {
    gl_check_error("make fbo init");
    glViewport(0, 0, w, h);

    if ((w == width) && (h == height)) return;
    log_debug("Resizing {}x{}", w, h);
    width = w;
    height = h;
    ::make_fbo(w, h, fbo, post_fbo, tex);
    ::make_fbo(w, h, pick.fbo, pick.post_fbo, pick.tex);
}

void geometry::render(const view_state& vs,
                      const glm::vec2& size,
                      const map_type<renderable>& regions,
                      const map_type<renderable>& markers) {
    make_fbo(size.x, size.y);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    float distance   = 2.5f;
    auto camera      = distance*glm::vec3{0.0f, 0.0f, 1.0f};
    glm::vec3 up     = {0.0f, 1.0f, 0.0f};
    glm::vec3 shift = {vs.offset.x/size.x, vs.offset.y/size.y, 0.0f};
    glm::mat4 view = glm::lookAt(camera, (target - root)/rescale + shift, up);
    glm::mat4 proj = glm::perspective(glm::radians(vs.zoom), size.x/size.y, 0.1f, 100.0f);

    auto light = camera;
    auto light_color = glm::vec3{1.0f, 1.0f, 1.0f};

    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, vs.phi, glm::vec3(0.0f, 1.0f, 0.0f));
        ::render(region_program, model, view, proj, camera, light, light_color, regions);
    }
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, -vs.phi, glm::vec3(0.0f, 1.0f, 0.0f));
        view  = glm::rotate(view,   vs.phi, glm::vec3(0.0f, 1.0f, 0.0f));
        glDisable(GL_DEPTH_TEST);
        ::render(marker_program, model, view, proj, camera, light, light_color, markers);
        glEnable(GL_DEPTH_TEST);
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, post_fbo);
    glBlitFramebuffer(0, 0, size.x, size.y, 0, 0, size.x, size.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
}

std::optional<object_id> geometry::get_id_at(const glm::vec2& pos,
                                             const view_state& vs,
                                             const glm::vec2& size,
                                             const map_type<renderable>& regions) {
    make_fbo(size.x, size.y);

    float distance   = 2.5f;
    auto camera      = distance*glm::vec3{0.0f, 0.0f, 1.0f};
    glm::vec3 up     = {0.0f, 1.0f, 0.0f};
    glm::vec3 shift = {vs.offset.x/size.x, vs.offset.y/size.y, 0.0f};
    glm::mat4 view = glm::lookAt(camera, (target - root)/rescale + shift, up);
    glm::mat4 proj = glm::perspective(glm::radians(vs.zoom), size.x/size.y, 0.1f, 100.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, pick.fbo);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, vs.phi, glm::vec3(0.0f, 1.0f, 0.0f));
        ::render(object_program, model, view, proj, camera, {}, {}, regions);
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, pick.fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pick.post_fbo);
    glBlitFramebuffer(0, 0, size.x, size.y, 0, 0, size.x, size.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    glm::vec3 color_at;
    glBindFramebuffer(GL_FRAMEBUFFER, pick.post_fbo);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glReadPixels(pos.x, -pos.y, 1, 1, GL_RGB, GL_FLOAT, &color_at.x);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (color_at == glm::vec3{1.0f, 1.0f, 1.0f}) return {};
    if ((color_at.x < 0.0f) || (color_at.y < 0.0f) || (color_at.z < 0.0f)) return {};
    size_t segment = color_at.x*256*256*256 + color_at.y*256*256 + color_at.z*256;
    if (id_to_branch.contains(segment)) return {{segment, id_to_branch[segment]}};
    return {};
}

renderable geometry::make_marker(const std::vector<glm::vec3>& points, glm::vec4 color) {
    std::vector<glm::vec3> off;
    for (const auto& marker: points) {
        off.emplace_back((marker.x - root.x)/rescale,
                         (marker.y - root.y)/rescale,
                         (marker.z - root.z)/rescale);
    }
    log_debug("Size of offset {}", off.size());
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
    const auto n_pt = 6*(n_faces + 1);
    for (const auto& seg: segments) {
        const auto idx = id_to_index[seg.id];
        for (auto idy = n_pt*idx; idy < n_pt*(idx + 1); ++idy) tris.push_back(triangles[idy]);
    }
    auto vao = make_vao(tris);
    return {tris.size(), 1, vao, true, color};
}

void geometry::load_geometry(const arb::morphology& morph) {
    std::vector<arb::msegment> segments;
    for (auto branch = 0ul; branch < morph.num_branches(); ++branch) {
        for (const auto& segment: morph.branch_segments(branch)) {
            segments.push_back(segment);
            id_to_branch[segment.id] = branch;
        }
    }
    log_info("Making geometry");
    if (segments.empty()) {
        log_info("Empty!");
        return;
    }
    log_info("Got {} segments", segments.size());
    auto tmp = segments[0].prox; // is always the root
    root = {(float) tmp.x, (float) tmp.y, (float) tmp.z};
    target = root;
    size_t index = 0;

    for (const auto& [id, prox, dist, tag]: segments) {
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

        glm::vec3 obj{((id/256/256)%256)/256.0f, ((id/256)%256)/256.0f, (id%256)/256.0f};

        for (auto rx = 0ul; rx <= n_faces; ++rx) {
            // Normal to line segment; used to extrude cylinder
            auto normal_next = rot*normal;

            // Generate a quad from two triangles
            // 00  10  proximal
            //  *--*   rotation ->
            //  | /|
            //  |/ |
            //  *--*   rotation ->
            // 01  11  distal
            auto v00 = static_cast<float>(prox.radius)*normal      + c_prox;
            auto v01 = static_cast<float>(dist.radius)*normal      + c_dist;
            auto v10 = static_cast<float>(prox.radius)*normal_next + c_prox;
            auto v11 = static_cast<float>(dist.radius)*normal_next + c_dist;

            // Find surface normals; these are cheating
            auto n0 = normal;
            auto n1 = normal_next;

            // Make surface quad
            triangles.push_back({v00, n0, obj});
            triangles.push_back({v01, n0, obj});
            triangles.push_back({v10, n1, obj});
            triangles.push_back({v11, n1, obj});
            triangles.push_back({v01, n0, obj});
            triangles.push_back({v10, n1, obj});

            // Step to next segment
            normal = normal_next;
        }
        id_to_index[id] = index;
        index++;
    }

    // Re-scale into [-1, 1]^3 box
    log_debug("Cylinders generated: {} ({} points)", triangles.size()/n_faces/6, triangles.size());
    for (auto& tri: triangles) {
        rescale = std::max(rescale, std::abs(tri.position.x));
        rescale = std::max(rescale, std::abs(tri.position.y));
        rescale = std::max(rescale, std::abs(tri.position.z));
    }
    for(auto& tri: triangles) tri.position /= rescale;
    log_debug("Geometry re-scaled by 1/{}", rescale);
    log_info("Making geometry: completed");
}
