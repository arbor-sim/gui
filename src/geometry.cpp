#include "geometry.hpp"

#include <filesystem>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

auto randf() { return (float)rand()/(float)RAND_MAX; }

template<typename T>
unsigned make_buffer_object(const std::vector<T>& v, unsigned type) {
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(type, VBO);
    glBufferData(type, v.size()*sizeof(T), v.data(), GL_STATIC_DRAW);
    return VBO;
}

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
            glm::mat4 model,  glm::mat4 view,  glm::mat4 proj,
            glm::vec3 camera, glm::vec3 light, glm::vec3 light_color,
            const std::vector<renderable>& render) {
    gl_check_error("render init");
    glUseProgram(program);
    set_uniform(program, "model",       model);
    set_uniform(program, "view",        view);
    set_uniform(program, "proj",        proj);
    set_uniform(program, "camera",      camera);
    set_uniform(program, "light",       light);
    set_uniform(program, "light_color", light_color);
    for (const auto& v: render) {
        if (v.active) {
            set_uniform(program, "object_color", v.color);
            glBindVertexArray(v.vao);
            glDrawElementsInstanced(GL_TRIANGLES, v.count, GL_UNSIGNED_INT, 0, v.instances);
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

auto make_vao(unsigned vbo,
              const std::vector<unsigned>& idx,
              const std::vector<glm::vec3> off) {
    log_info("Setting up VAO");
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(point), (void*) (offsetof(point, position))); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(point), (void*) (offsetof(point, normal)));   glEnableVertexAttribArray(1);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(point), (void*) (offsetof(point, id)));       glEnableVertexAttribArray(3);

    unsigned ivbo = make_buffer_object(off, GL_ARRAY_BUFFER);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr); glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    unsigned int ebo = make_buffer_object(idx, GL_ELEMENT_ARRAY_BUFFER);

    glBindVertexArray(0);
    log_info("Setting up VAO: complete");
    return vao;
}


auto make_program(const std::string& dn, unsigned& program) {
    std::filesystem::path base = ARBORGUI_RESOURCES_BASE;
    log_info("Setting up shader program");
    auto vertex_shader   = make_shader(base / "glsl" / dn / "vertex.glsl",   GL_VERTEX_SHADER);
    auto fragment_shader = make_shader(base / "glsl" / dn / "fragment.glsl", GL_FRAGMENT_SHADER);

    glDeleteProgram(program);
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

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return program;
}

geometry::geometry() {
    make_program("region", region_program);
    make_program("branch", object_program);
    make_program("marker", marker_program);
    auto dx = 1.0f/40.0f;
    auto dy = dx/2.0f;
    std::vector<point> tris{{{0.0f,      0.0f,      0.0f}, {0.0f, 0.0f, 0.0f}},
                            {{0.0f + dy, 0.0f + dx, 0.0f}, {0.0f, 0.0f, 0.0f}},
                            {{0.0f + dx, 0.0f + dy, 0.0f}, {0.0f, 0.0f, 0.0f}}};
    marker_vbo = make_buffer_object(tris, GL_ARRAY_BUFFER); glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void make_fbo(int width, int height, unsigned int& fbo, unsigned int& post_fbo, unsigned int& tex) {
    glDeleteFramebuffers(1, &fbo);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    // create multisampled color attachment texture
    unsigned int ms_tex;
    glGenTextures(1, &ms_tex);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ms_tex);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, width, height, GL_TRUE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, ms_tex, 0);
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
    glDeleteFramebuffers(1, &post_fbo);
    glGenFramebuffers(1, &post_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, post_fbo);
    // create a color attachment texture
    glDeleteTextures(1, &tex);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) log_error("Intermediate framebuffer incomplete.");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDeleteRenderbuffers(1, &rbo);
    glDeleteTextures(1, &ms_tex);
}


void make_fbo(int w, int h, render_ctx& ctx) {
    gl_check_error("make fbo init");
    glViewport(0, 0, w, h);

    if ((w == ctx.width) && (h == ctx.height)) return;
    log_debug("Resizing {}x{}", w, h);
    ctx.width = w;
    ctx.height = h;
    ::make_fbo(w, h, ctx.fbo, ctx.post_fbo, ctx.tex);
}

void geometry::render(const view_state& vs,
                      const glm::vec2& size,
                      const std::vector<renderable>& regions,
                      const std::vector<renderable>& markers) {
    make_fbo(size.x, size.y, cell);

    glBindFramebuffer(GL_FRAMEBUFFER, cell.fbo);
    glClearColor(cell.clear_color.x, cell.clear_color.y, cell.clear_color.z, cell.clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    float distance   = 2.5f;
    auto camera      = distance*glm::vec3{0.0f, 0.0f, 1.0f};
    glm::vec3 up     = {0.0f, 1.0f, 0.0f};
    glm::vec3 shift  = {vs.offset.x/size.x, vs.offset.y/size.y, 0.0f};
    glm::mat4 view   = glm::lookAt(camera, vs.target + shift, up);
    glm::mat4 proj   = glm::perspective(glm::radians(vs.zoom), size.x/size.y, 0.1f, 100.0f);

    auto light = camera;
    auto light_color = glm::vec3{1.0f, 1.0f, 1.0f};

    {
        glFrontFace(GL_CW);
        glEnable(GL_CULL_FACE);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, vs.phi,   glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, vs.theta, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, vs.gamma, glm::vec3(0.0f, 0.0f, 1.0f));
        ::render(region_program, model, view, proj, camera, light, light_color, regions);
        glDisable(GL_CULL_FACE);
    }
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, -vs.gamma, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, -vs.theta, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, -vs.phi, glm::vec3(0.0f, 1.0f, 0.0f));
        view = glm::rotate(view,   vs.phi, glm::vec3(0.0f, 1.0f, 0.0f));
        view = glm::rotate(view, vs.theta, glm::vec3(1.0f, 0.0f, 0.0f));
        view = glm::rotate(view, vs.gamma, glm::vec3(0.0f, 0.0f, 1.0f));
        glDisable(GL_DEPTH_TEST);
        ::render(marker_program, model, view, proj, camera, light, light_color, markers);
        glEnable(GL_DEPTH_TEST);
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, cell.fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, cell.post_fbo);
    glBlitFramebuffer(0, 0, size.x, size.y, 0, 0, size.x, size.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
}

std::optional<object_id> geometry::get_id_at(const glm::vec2& pos,
                                             const view_state& vs,
                                             const glm::vec2& size,
                                             const std::vector<renderable>& regions) {
    make_fbo(size.x, size.y, pick);

    float distance  = 2.5f;
    auto camera     = distance*glm::vec3{0.0f, 0.0f, 1.0f};
    glm::vec3 up    = {0.0f, 1.0f, 0.0f};
    glm::vec3 shift = {vs.offset.x/size.x, vs.offset.y/size.y, 0.0f};
    glm::mat4 view  = glm::lookAt(camera, vs.target + shift, up);
    glm::mat4 proj  = glm::perspective(glm::radians(vs.zoom), size.x/size.y, 0.1f, 100.0f);

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

void geometry::make_marker(const std::vector<glm::vec3>& points, renderable& r) {
    std::vector<glm::vec3> off;
    for (const auto& marker: points) {
        off.emplace_back((marker.x - root.x)/rescale,
                         (marker.y - root.y)/rescale,
                         (marker.z - root.z)/rescale);
    }
    r.vao =  make_vao(marker_vbo, {0, 1, 2}, off);
    r.count = 3;
    r.instances = off.size();
    r.active = true;
}

void geometry::make_region(const std::vector<size_t>& segments, renderable& r) {
    std::vector<unsigned> idcs{};
    for (const auto& seg: segments) {
        const auto idx = id_to_index[seg];
        for (auto idy = 0; idy < n_indices; ++idy) idcs.push_back(indices[idy + n_indices*idx]);
    }
    assert(idcs.size() == n_indices*segments.size());
    r.vao = make_vao(vbo, idcs, {{0.0f, 0.0f, 0.0f}});
    r.count = idcs.size();
    r.instances = 1;
    r.active = true;
}

void geometry::load_geometry(const arb::morphology& morph) {
    clear();

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
    size_t index = 0;

    for (const auto& [id, prox, dist, tag]: segments) {
        // Shift to root and find vector along the segment
        const auto c_prox = glm::vec3{(prox.x - root.x), (prox.y - root.y), (prox.z - root.z)};
        const auto c_dist = glm::vec3{(dist.x - root.x), (dist.y - root.y), (dist.z - root.z)};
        const auto c_diff = c_prox - c_dist;
        const auto r_prox = (float) prox.radius;
        const auto r_dist = (float) dist.radius;

        // Make a normal to segment vector
        auto normal = glm::vec3{0.0, 0.0, 0.0};
        for (; normal == glm::vec3{0.0, 0.0, 0.0};) {
            auto t = glm::vec3{randf(), randf(), randf()};
            normal = glm::normalize(glm::cross(c_diff, t));
        }

        // Generate cylinder from n_faces triangles
        const auto rot = glm::mat3(glm::rotate(glm::mat4(1.0f), 2.0f*PI/n_faces, c_diff));
        glm::vec3 obj{((id/256/256)%256)/256.0f, ((id/256)%256)/256.0f, (id%256)/256.0f};
        vertices.push_back({c_prox,  glm::normalize(c_dist), obj});
        vertices.push_back({c_dist, -glm::normalize(c_dist), obj});
        for (auto face = 0ul; face < n_faces; ++face) {
            vertices.push_back({r_prox*normal + c_prox, normal, obj});
            vertices.push_back({r_dist*normal + c_dist, normal, obj});
            normal = rot*normal;
        }

        // Generate a quad from two triangles
        // 00  10  proximal
        //  *--*   rotation ->
        //  | /|
        //  |/ |
        //  *--*   rotation ->
        // 01  11  distal
        auto base = index*n_vertices;
        auto c0  = base;
        auto c1  = base + 1;
        for (auto face = 0ul; face < n_faces - 1; ++face) {
            auto p00 = base + 2*face + 2;
            auto p01 = base + 2*face + 3;
            auto p10 = base + 2*face + 4;
            auto p11 = base + 2*face + 5;
            // Face
            indices.push_back(p00); indices.push_back(p10); indices.push_back(p01);
            indices.push_back(p01); indices.push_back(p10); indices.push_back(p11);
            // Proximal cap
            indices.push_back(c0);  indices.push_back(p10); indices.push_back(p00);
            // Distal cap
            indices.push_back(c1);  indices.push_back(p01); indices.push_back(p11);
        }
        auto p00 = base + 2*(n_faces - 1) + 2;
        auto p01 = base + 2*(n_faces - 1) + 3;
        auto p10 = base + 4;
        auto p11 = base + 3;
        // Face
        indices.push_back(p00); indices.push_back(p10); indices.push_back(p01);
        indices.push_back(p01); indices.push_back(p10); indices.push_back(p11);
        // Proximal cap
        indices.push_back(c0);  indices.push_back(p10); indices.push_back(p00);
        // Distal cap
        indices.push_back(c1);  indices.push_back(p01); indices.push_back(p11);

        // Next
        id_to_index[id] = index++;
    }

    assert(vertices.size() == segments.size()*n_vertices);
    assert(indices.size()  == segments.size()*n_indices);

    // Re-scale into [-1, 1]^3 box
    log_debug("Cylinders generated: {} ({} points)", indices.size()/n_indices, vertices.size());
    for (auto& tri: vertices) {
        rescale = std::max(rescale, std::abs(tri.position.x));
        rescale = std::max(rescale, std::abs(tri.position.y));
        rescale = std::max(rescale, std::abs(tri.position.z));
    }
    for(auto& tri: vertices) tri.position /= rescale;
    log_debug("Geometry re-scaled by 1/{}", rescale);
    vbo = make_buffer_object(vertices, GL_ARRAY_BUFFER); glBindBuffer(GL_ARRAY_BUFFER, 0);
    log_info("Making geometry: completed");
}

void clear_ctx(render_ctx& ctx) {
    glDeleteFramebuffers(1, &ctx.fbo);      ctx.fbo = 0;
    glDeleteFramebuffers(1, &ctx.post_fbo); ctx.fbo = 0;
    glDeleteTextures(1, &ctx.tex);          ctx.tex = 0;
    ctx.width  = -1; ctx.height = -1;
}

void geometry::clear() {
    vertices.clear();
    indices.clear();
    id_to_index.clear();
    id_to_branch.clear();
    rescale = -1;
    clear_ctx(pick);
    clear_ctx(cell);
}
