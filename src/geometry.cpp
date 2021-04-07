#include "geometry.hpp"

#include <filesystem>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "utils.hpp"

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

namespace {
unsigned make_pixel_buffer() {
    ZoneScopedN(__FUNCTION__);
    unsigned pbo = 0;
    glGenBuffers(1, &pbo);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
    glBufferData(GL_PIXEL_PACK_BUFFER, sizeof(glm::vec3), NULL, GL_STREAM_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    return pbo;
}

// intiate transfer
void fetch_pixel_buffer(unsigned pbo, const glm::vec2& pos) {
    ZoneScopedN(__FUNCTION__);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
    glReadPixels(pos.x, pos.y, 1, 1, GL_RGB, GL_FLOAT, 0); // where, size, format, type, offset
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

// finalise transfer and obtain value
auto get_value_pixel_buffer(unsigned pbo) {
    ZoneScopedN(__FUNCTION__);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
    glm::vec3 out = {-1, -1, -1};
    float* ptr = (float*) glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
    std::memcpy(&out.x, ptr, sizeof(glm::vec3));
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    return out;
}

auto randf() { return (float)rand()/(float)RAND_MAX; }

template<typename T>
unsigned make_buffer_object(const std::vector<T>& v, unsigned type) {
    ZoneScopedN(__FUNCTION__);
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(type, VBO);
    glBufferData(type, v.size()*sizeof(T), v.data(), GL_STATIC_DRAW);
    return VBO;
}

inline void finalise_msaa_fbo(render_ctx& ctx, const glm::vec2& size) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, ctx.fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ctx.post_fbo);
    glBlitFramebuffer(0, 0, size.x, size.y, 0, 0, size.x, size.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

inline void set_uniform(unsigned program, const std::string& name, const glm::vec3& data) {
    auto loc = glGetUniformLocation(program, name.c_str());
    glUniform3fv(loc, 1, glm::value_ptr(data));
    gl_check_error(fmt::format("setting uniform vec3: {}", name));
}

inline void set_uniform(unsigned program, const std::string& name, const glm::vec4& data) {
    auto loc = glGetUniformLocation(program, name.c_str());
    glUniform4fv(loc, 1, glm::value_ptr(data));
    gl_check_error(fmt::format("setting uniform vec4: {}", name));
}

inline void set_uniform(unsigned program, const std::string& name, const glm::mat4& data) {
    auto loc = glGetUniformLocation(program, name.c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(data));
    gl_check_error(fmt::format("setting uniform mat4: {}", name));
}

inline void render(unsigned program,
                   const glm::mat4& model,  const glm::mat4& view,
                   const glm::vec3& camera, const glm::vec3& light, const glm::vec3& light_color,
                   const std::vector<renderable>& render) {
    ZoneScopedN(__FUNCTION__);
    gl_check_error("render init");
    glUseProgram(program);
    set_uniform(program, "model",       model);
    set_uniform(program, "view",        view);
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

inline auto make_shader(const std::filesystem::path& fn, unsigned shader_type) {
    log_debug("Loading shader {}", fn.c_str());
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
    log_debug("Loading shader {}: complete", fn.c_str());
    return shader;
}

inline auto make_vao(unsigned vbo, const std::vector<unsigned>& idx, const std::vector<glm::vec3>& off) {
    ZoneScopedN(__FUNCTION__);
    log_debug("Setting up VAO");
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
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    log_debug("Setting up VAO: complete");
    return vao;
}

inline auto make_program(const std::string& dn, unsigned& program) {
    ZoneScopedN(__FUNCTION__);
    std::filesystem::path base = ARBORGUI_RESOURCES_BASE;
    log_info("Setting up shader program {}", dn.c_str());
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
    log_debug("Setting up shader program: complete");

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return program;
}

void make_fbo(int w, int h, render_ctx& ctx) {
    ZoneScopedN(__FUNCTION__);
    gl_check_error("make fbo init");
    glViewport(0, 0, w, h);

    if ((w == ctx.width) && (h == ctx.height)) return;
    log_debug("Resizing {}x{} --> {}x{}", ctx.width, ctx.height, w, h);
    ctx.width = w; ctx.height = h;

    glDeleteFramebuffers(1, &ctx.fbo);
    glGenFramebuffers(1, &ctx.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, ctx.fbo);
    // create multisampled color attachment texture
    glGenTextures(1, &ctx.ms_tex);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ctx.ms_tex);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, ctx.width, ctx.height, GL_TRUE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, ctx.ms_tex, 0);
    // create multisampled renderbuffer object for depth and stencil attachments
    glGenRenderbuffers(1, &ctx.rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, ctx.rbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, ctx.width, ctx.height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, ctx.rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) log_error("Framebuffer incomplete.");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // configure post-processing framebuffer
    glDeleteFramebuffers(1, &ctx.post_fbo);
    glGenFramebuffers(1, &ctx.post_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, ctx.post_fbo);
    // create a color attachment texture
    glDeleteTextures(1, &ctx.tex);
    glGenTextures(1, &ctx.tex);
    glBindTexture(GL_TEXTURE_2D, ctx.tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ctx.width, ctx.height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctx.tex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) log_error("Intermediate framebuffer incomplete.");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

}

geometry::geometry() {
    ZoneScopedN(__FUNCTION__);
    make_program("region", region_program);
    make_program("branch", object_program);
    make_program("marker", marker_program);
    auto dx = 1.0f/40.0f;
    auto dy = dx/2.0f;
    cell.clear_color = {214.0f/255, 214.0f/255, 214.0f/255};
    std::vector<point> tris{{{0.0f,      0.0f,      0.0f}, {0.0f, 0.0f, 0.0f}},
                            {{0.0f + dy, 0.0f + dx, 0.0f}, {0.0f, 0.0f, 0.0f}},
                            {{0.0f + dx, 0.0f + dy, 0.0f}, {0.0f, 0.0f, 0.0f}}};
    marker_vbo = make_buffer_object(tris, GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void geometry::render(const view_state& vs, const glm::vec2& size,
                      const std::vector<renderable>& regions, const std::vector<renderable>& markers) {
    ZoneScopedN(__FUNCTION__);
    make_fbo(size.x, size.y, cell);
    make_fbo(size.x, size.y, pick);

    float distance   = 2.5f;
    auto camera      = distance*glm::vec3{0.0f, 0.0f, 1.0f};
    glm::vec3 up     = {0.0f, 1.0f, 0.0f};
    glm::vec3 shift  = {vs.offset.x/size.x, vs.offset.y/size.y, 0.0f};
    glm::mat4 view   = glm::lookAt(camera, vs.target + shift, up);
    glm::mat4 proj   = glm::perspective(glm::radians(vs.zoom), size.x/size.y, 0.1f, 100.0f);
    view = proj*view;


    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, vs.phi,   glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, vs.theta, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, vs.gamma, glm::vec3(0.0f, 0.0f, 1.0f));

    if (!pbo) pbo = make_pixel_buffer();

    // Render flat colours to side fbo for picking
    {
        ZoneScopedN("render-pick");
        glBindFramebuffer(GL_FRAMEBUFFER, pick.fbo);
        glClearColor(pick.clear_color.x, pick.clear_color.y, pick.clear_color.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        {
            ZoneScopedN("render-pick-opengl");
            glFrontFace(GL_CW);
            glEnable(GL_CULL_FACE);
            ::render(object_program, model, view, camera, {}, {}, regions);
            glDisable(GL_CULL_FACE);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        finalise_msaa_fbo(pick, size);
        // Initialise pixel read
        glBindFramebuffer(GL_FRAMEBUFFER, pick.post_fbo);
        fetch_pixel_buffer(pbo, pick_pos);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Render main scene
    {
        ZoneScopedN("render-cell");
        auto light = camera;
        auto light_color = glm::vec3{1.0f, 1.0f, 1.0f};
        glBindFramebuffer(GL_FRAMEBUFFER, cell.fbo);
        glClearColor(cell.clear_color.x, cell.clear_color.y, cell.clear_color.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Render Frustra ...
        {
            ZoneScopedN("render-regions-opengl");
            glFrontFace(GL_CW);
            glEnable(GL_CULL_FACE);
            ::render(region_program, model, view, camera, light, light_color, regions);
            glDisable(GL_CULL_FACE);
        }
        // ... and markers
        {
            ZoneScopedN("render-markers-opengl");
            glm::mat4 imodel = glm::mat4(1.0f);
            imodel = glm::rotate(imodel, -vs.gamma, glm::vec3(0.0f, 0.0f, 1.0f));
            imodel = glm::rotate(imodel, -vs.theta, glm::vec3(1.0f, 0.0f, 0.0f));
            imodel = glm::rotate(imodel, -vs.phi,   glm::vec3(0.0f, 1.0f, 0.0f));
            auto iview = view;
            iview = glm::rotate(iview, vs.phi,   glm::vec3(0.0f, 1.0f, 0.0f));
            iview = glm::rotate(iview, vs.theta, glm::vec3(1.0f, 0.0f, 0.0f));
            iview = glm::rotate(iview, vs.gamma, glm::vec3(0.0f, 0.0f, 1.0f));
            glDisable(GL_DEPTH_TEST);
            ::render(marker_program, imodel, iview, camera, light, light_color, markers);
            glEnable(GL_DEPTH_TEST);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        finalise_msaa_fbo(cell, size);
    }
}

glm::vec3 pack_id(size_t id)      { return {((id/256/256)%256)/256.0f, ((id/256)%256)/256.0f, (id%256)/256.0f}; }
size_t    unpack_id(glm::vec3 id) { return (id.x*256.0f*256.0f*256.0f) + (id.y*256.0f*256.0f) + (id.z*256.0f); }

std::optional<object_id> geometry::get_id() {
    ZoneScopedN(__FUNCTION__);
    auto color_at = get_value_pixel_buffer(pbo);
    if (color_at == pick.clear_color) return {};
    if ((color_at.x < 0.0f) || (color_at.y < 0.0f) || (color_at.z < 0.0f)) return {};
    size_t segment = unpack_id(color_at);
    if (id_to_branch.contains(segment)) return {{segment, id_to_branch[segment], segments[segment], branch_to_ids[id_to_branch[segment]]}};
    auto c = pack_id(segment);
    return {};
}

void geometry::make_marker(const std::vector<glm::vec3>& points, renderable& r) {
    ZoneScopedN(__FUNCTION__);
    std::vector<glm::vec3> off;
    for (const auto& marker: points) {
        off.emplace_back((marker.x - root.x)/rescale,
                         (marker.y - root.y)/rescale,
                         (marker.z - root.z)/rescale);
    }
    r.vao = make_vao(marker_vbo, {0, 1, 2}, off);
    r.count = 3;
    r.instances = off.size();
    r.active = true;
}

void geometry::make_region(const std::vector<size_t>& segments, renderable& r) {
    ZoneScopedN(__FUNCTION__);
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
    ZoneScopedN(__FUNCTION__);
    clear();
    {
        ZoneScopedN("look up tables");
        for (auto branch = 0ul; branch < morph.num_branches(); ++branch) {
            std::vector<size_t> tmp;
            for (const auto& segment: morph.branch_segments(branch)) {
                segments.push_back(segment);
                id_to_branch[segment.id] = branch;
                tmp.push_back(segment.id);
            }
            if (!tmp.empty()) { // this should be never empty
                std::sort(tmp.begin(), tmp.end());
                auto it = tmp.begin();
                auto lo = *it;
                auto hi = *it;
                it++;
                while (it != tmp.end()) {
                    if (*it <= hi) log_error("Duplicate in branch -> id map");
                    if (*it == hi + 1) {
                        hi = *it++;
                    } else {
                        branch_to_ids[branch].emplace_back(lo, hi);
                        it++;
                        lo = hi = *it++;
                    }
                }
                branch_to_ids[branch].emplace_back(lo, hi);
            }
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
    vertices.reserve(segments.size()*n_vertices);
    indices.reserve(segments.size()*n_indices);
    {
        ZoneScopedN("Frustra");
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
            glm::vec3 obj = pack_id(id);
            vertices.push_back({c_prox, -glm::normalize(c_dist), obj});
            vertices.push_back({c_dist,  glm::normalize(c_dist), obj});
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
    vbo = make_buffer_object(vertices, GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    log_info("Making geometry: completed");
}

void geometry::clear() {
    ZoneScopedN(__FUNCTION__);
    vertices.clear();
    indices.clear();
    id_to_index.clear();
    id_to_branch.clear();
    segments.clear();
    branch_to_ids.clear();
    rescale = -1;
}
