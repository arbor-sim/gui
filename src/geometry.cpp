#include "geometry.hpp"

#include <filesystem>
#include <unordered_map>
#include <unordered_set>

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
    glBufferData(GL_PIXEL_PACK_BUFFER, sizeof(glm::vec4), NULL, GL_STREAM_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    return pbo;
}

// intiate transfer
void fetch_pixel_buffer(unsigned pbo, const glm::vec2& pos) {
    ZoneScopedN(__FUNCTION__);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
    glReadPixels(pos.x, pos.y, 1, 1, GL_RGBA, GL_FLOAT, 0); // where, size, format, type, offset
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

// finalise transfer and obtain value
auto get_value_pixel_buffer(unsigned pbo) {
    ZoneScopedN(__FUNCTION__);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
    glm::vec4 out = {-1, -1, -1, -1};
    float* ptr = (float*) glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
    std::memcpy(&out.x, ptr, sizeof(out));
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    return glm::vec3(out);
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

inline void set_uniform(unsigned program, const std::string& name, const float& data) {
    auto loc = glGetUniformLocation(program, name.c_str());
    glUniform1f(loc, data);
    gl_check_error(fmt::format("setting uniform scalar: {}", name));
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
                   const glm::vec3& camera, const glm::vec3& light_color,
                   const std::vector<renderable>& render) {
    ZoneScopedN(__FUNCTION__);
    gl_check_error("render init");
    auto light = glm::vec4(camera, 1.0f) + glm::vec4{0.0f, 1.5f, 0.0f, 0.0f};
    auto key   = glm::rotate(glm::mat4(1.0f), 0.25f*PI, glm::vec3{0.0f, 1.0f, 0.0f})*light;
    auto fill  = glm::rotate(glm::mat4(1.0f), 0.74f*PI, glm::vec3{0.0f, 1.0f, 0.0f})*light;
    auto back  = glm::rotate(glm::mat4(1.0f), 1.25f*PI, glm::vec3{0.0f, 1.0f, 0.0f})*light;
    glUseProgram(program);
    set_uniform(program, "model",      model);
    set_uniform(program, "view",       view);
    set_uniform(program, "camera",     camera);
    set_uniform(program, "key",        glm::vec3(key));
    set_uniform(program, "key_color",  light_color);
    set_uniform(program, "back",       glm::vec3(back));
    set_uniform(program, "back_color", light_color*0.2f);
    set_uniform(program, "fill",       glm::vec3(fill));
    set_uniform(program, "fill_color", light_color*0.5f);
    for (const auto& v: render) {
        if (v.active) {
            set_uniform(program, "object_color", v.color);
            set_uniform(program, "zorder", v.zorder*0.01f);
            glBindVertexArray(v.vao);
            glDrawElementsInstanced(GL_TRIANGLES, v.count, GL_UNSIGNED_INT, 0, v.instances);
            glBindVertexArray(0);
        }
    }
    glUseProgram(0);
    gl_check_error("render end");
}

inline void render(unsigned program,
                   const glm::mat4& model, const glm::mat4& view,
                   const std::vector<renderable>& render) {
    ZoneScopedN(__FUNCTION__);
    gl_check_error("render init");
    glUseProgram(program);
    set_uniform(program, "model", model);
    set_uniform(program, "view",  view);
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
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(point), (void*) (offsetof(point, id)));       glEnableVertexAttribArray(2);

    unsigned ivbo = make_buffer_object(off, GL_ARRAY_BUFFER);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr); glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

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
    {
        std::vector<point> tris{{{0.0f,      0.0f,      0.0f}, {0.0f, 0.0f, 0.0f}},
                                {{0.0f + dy, 0.0f + dx, 0.0f}, {0.0f, 0.0f, 0.0f}},
                                {{0.0f + dx, 0.0f + dy, 0.0f}, {0.0f, 0.0f, 0.0f}}};
        marker_vbo = make_buffer_object(tris, GL_ARRAY_BUFFER);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    make_ruler({0,0,0}, 0.1f);
}

void geometry::render(const view_state& vs,
                      const std::vector<renderable>& regions, const std::vector<renderable>& markers,
                      const glm::vec2& pick_pos) {
    ZoneScopedN(__FUNCTION__);
    make_fbo(vs.size.x, vs.size.y, cell);
    make_fbo(vs.size.x, vs.size.y, pick);

    float distance  = 2.5f;
    auto camera     = distance*glm::vec3{0.0f, 0.0f, 1.0f};
    glm::vec3 up    = {0.0f, 1.0f, 0.0f};
    glm::vec3 shift = {vs.offset.x/vs.size.x, vs.offset.y/vs.size.y, 0.0f};
    glm::mat4 view  = glm::lookAt(camera, vs.target + shift, up);
    glm::mat4 proj  = glm::perspective(glm::radians(vs.zoom), vs.size.x/vs.size.y, 0.1f, 100.0f);
    view = proj*view; // pre-multiply view matrices

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
            ::render(object_program, model, view, regions);
            glDisable(GL_CULL_FACE);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        finalise_msaa_fbo(pick, vs.size);
        // Initialise pixel read
        glBindFramebuffer(GL_FRAMEBUFFER, pick.post_fbo);
        fetch_pixel_buffer(pbo, pick_pos);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Render main scene
    {
        ZoneScopedN("render-cell");
        auto light = camera + glm::vec3{0.0f, 1.0f, 0.0f};
        auto light_color = glm::vec3{1.0f, 1.0f, 1.0f};
        glBindFramebuffer(GL_FRAMEBUFFER, cell.fbo);
        glClearColor(cell.clear_color.x, cell.clear_color.y, cell.clear_color.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Render Frustra ...
        {
            ZoneScopedN("render-regions-opengl");
            glFrontFace(GL_CW);
            glEnable(GL_CULL_FACE);
            ::render(region_program, model, view, camera, light_color, regions);
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
            ::render(marker_program, imodel, iview, markers);
            glEnable(GL_DEPTH_TEST);
        }
        {
            glFrontFace(GL_CW);
            glEnable(GL_CULL_FACE);
            ::render(region_program, model, view, camera, light_color, axes);
            glDisable(GL_CULL_FACE);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        finalise_msaa_fbo(cell, vs.size);
    }
}

constexpr size_t id_scale = 256.0f;

glm::vec3 pack_id(size_t id) {
    glm::vec3 result;
    result.x = ((id/id_scale/id_scale)%id_scale)/float{id_scale - 1};
    result.y =          ((id/id_scale)%id_scale)/float{id_scale - 1};
    result.z =                     (id%id_scale)/float{id_scale - 1};
    return result;
}

size_t unpack_id(const glm::vec3& id) {
    size_t result = 0;
    result += id.x*float{id_scale - 1}*id_scale*id_scale;
    result += id.y*float{id_scale - 1}*id_scale;
    result += id.z*float{id_scale - 1};
    return result;
}

std::optional<object_id> geometry::get_id() {
    ZoneScopedN(__FUNCTION__);
    auto color = get_value_pixel_buffer(pbo);
    if ((color == pick.clear_color) || (color.x < 0.0f) || (color.y < 0.0f) || (color.z < 0.0f)) return {};
    auto id = unpack_id(color);
    if (!id_to_branch.contains(id) || !id_to_index.contains(id)) return {};
    auto branch  = id_to_branch[id];
    auto segment = id_to_index[id];
    return {{branch, segments[segment], &branch_to_ids[branch]}};
}

void geometry::make_marker(const std::vector<glm::vec3>& points, renderable& r) {
    ZoneScopedN(__FUNCTION__);
    std::vector<glm::vec3> off;
    for (const auto& marker: points) off.emplace_back((marker - root)/rescale);
    glDeleteVertexArrays(1, &r.vao);
    r.vao       = make_vao(marker_vbo, {0, 1, 2}, off);
    r.count     = 3;
    r.instances = off.size();
    r.active    = true;
}

void geometry::make_region(const std::vector<arb::msegment>& segs, renderable& r) {
    ZoneScopedN(__FUNCTION__);
    std::vector<unsigned> idcs{};
    for (const auto& seg: segs) {
        const auto idx = id_to_index[seg.id];
        for (auto idy = n_indices*idx; idy < n_indices*(idx + 1); ++idy) {
            idcs.push_back(indices[idy]);
        }
    }
    glDeleteVertexArrays(1, &r.vao);
    r.vao       = make_vao(vbo, idcs, {{0.0f, 0.0f, 0.0f}});
    r.count     = idcs.size();
    r.instances = 1;
    r.active    = true;
}

void make_frustrum_vertices(const glm::vec3& p, float rp,
                            const glm::vec3& d, float rd,
                            size_t id,
                            std::vector<point>& vertices,
                            size_t n_faces) {
    auto diff = p - d;
    // Make a normal to segment vector
    auto normal = glm::vec3{0.0f, 0.0f, 0.0f};
    while (normal == glm::vec3{0.0f, 0.0f, 0.0f}) {
        auto t = glm::vec3{randf(), randf(), randf()};
        normal = glm::cross(diff, t);
        normal = glm::normalize(normal);
    }
    // Generate cylinder from n_faces triangles
    auto rot  = glm::mat3(glm::rotate(glm::mat4(1.0f), 2.0f*PI/n_faces, diff));
    auto obj  = pack_id(id);
    auto up   =  glm::normalize(d);
    auto down = -up;
    vertices.push_back({p, down, obj});
    vertices.push_back({d, up,   obj});
    for (auto face = 0ul; face < n_faces; ++face) {
        vertices.push_back({rp*normal + p, normal, obj});
        vertices.push_back({rp*normal + p, down,   obj});
        vertices.push_back({rd*normal + d, normal, obj});
        vertices.push_back({rd*normal + d, up,     obj});
        normal = rot*normal;
    }
}

void make_frustrum_indices(size_t base, std::vector<unsigned>& indices, size_t n_faces) {
    // A frustrum is generated from triangles like this:
    //        *   c0
    //       / \
    // p0   *---*  p1
    // f00  *---*  f10    proximal
    //      |  /|
    //      | / |         rotation ->
    //      |/  |
    // f01  *---*  f11    distal
    // d0   *---*  d1
    //       \ /
    //        *   c1
    auto c0 = base, c1 = c0 + 1;
    for (auto face = 0ul; face < n_faces - 1; ++face) {
        auto
            p0  = base + 4*face + 3, p1  = p0  + 4,
            f00 = base + 4*face + 2, f10 = f00 + 4,
            f01 = base + 4*face + 4, f11 = f01 + 4,
            d0  = base + 4*face + 5, d1  = d0  + 4;
        indices.push_back(f00); indices.push_back(f10); indices.push_back(f01); // Face 0
        indices.push_back(f01); indices.push_back(f10); indices.push_back(f11); // Face 1
        indices.push_back(c0);  indices.push_back(p1);  indices.push_back(p0);  // Proximal cap
        indices.push_back(c1);  indices.push_back(d0);  indices.push_back(d1);  // Distal cap
    }
    auto
        p0  = base + 4*(n_faces - 1) + 3, p1  = base + 3,
        f00 = base + 4*(n_faces - 1) + 2, f10 = base + 2,
        f01 = base + 4*(n_faces - 1) + 4, f11 = base + 4,
        d0  = base + 4*(n_faces - 1) + 5, d1  = base + 5;
    indices.push_back(f00); indices.push_back(f10); indices.push_back(f01); // Face 0
    indices.push_back(f01); indices.push_back(f10); indices.push_back(f11); // Face 1
    indices.push_back(c0);  indices.push_back(p1);  indices.push_back(p0);  // Proximal cap
    indices.push_back(c1);  indices.push_back(d0);  indices.push_back(d1);  // Distal cap
}


void geometry::make_ruler(const glm::vec3& point, float scale) {
    ZoneScopedN(__FUNCTION__);

    glDeleteBuffers(1, &ax_vbo);

    ax_vertices.clear();
    x_ax_indices.clear();
    y_ax_indices.clear();
    z_ax_indices.clear();

    auto f = 0.001f, d = scale;
    auto N = 64, V = N*4 + 2;
    auto p = point;

    if (rescale > 0) { d /= rescale; p /= rescale; }

    make_frustrum_vertices(p + glm::vec3{0,0,d}, f, p + glm::vec3{0,0,-d}, f, 0, ax_vertices, N); make_frustrum_indices(0*V, x_ax_indices, N);
    make_frustrum_vertices(p + glm::vec3{0,d,0}, f, p + glm::vec3{0,-d,0}, f, 0, ax_vertices, N); make_frustrum_indices(1*V, y_ax_indices, N);
    make_frustrum_vertices(p + glm::vec3{d,0,0}, f, p + glm::vec3{-d,0,0}, f, 0, ax_vertices, N); make_frustrum_indices(2*V, z_ax_indices, N);

    ax_vbo = make_buffer_object(ax_vertices, GL_ARRAY_BUFFER);

    for (auto& r: axes) glDeleteVertexArrays(1, &r.vao);
    axes.clear();
    renderable x, y, z;

    x.vao       = make_vao(ax_vbo, x_ax_indices, {{0,0,0}});
    x.count     = x_ax_indices.size();
    x.instances = 1;
    x.active    = true;
    x.color     = {1, 0, 0, 1};

    y.vao       = make_vao(ax_vbo, y_ax_indices, {{0,0,0}});
    y.count     = y_ax_indices.size();
    y.instances = 1;
    y.active    = true;
    y.color     = {0, 1, 0, 1};

    z.vao       = make_vao(ax_vbo, z_ax_indices, {{0,0,0}});
    z.count     = z_ax_indices.size();
    z.instances = 1;
    z.active    = true;
    z.color     = {0, 0, 1, 1};

    axes = {{x, y, z}};
};

void geometry::load_geometry(const arb::morphology& morph) {
    ZoneScopedN(__FUNCTION__);
    clear();
    n_vertices  = n_faces*4 + 2;  // Faces: 4 vertices 2 are shared. Caps: three per face, center is shared
    n_triangles = n_faces*4;      // Each face is a quad made from 2 tris, caps have one tri per face
    n_indices   = n_triangles*3;  // Three indices (reference to vertex) per tri
    {
        ZoneScopedN("look up tables");
        auto index = 0ul;
        for (auto branch = 0ul; branch < morph.num_branches(); ++branch) {
            std::vector<size_t> tmp;
            for (const auto& segment: morph.branch_segments(branch)) {
                auto id = segment.id;
                segments.push_back(segment);
                id_to_branch[id] = branch;
                id_to_index[id]  = index;
                tmp.push_back(id);
                index++;
            }
            std::sort(tmp.begin(), tmp.end());
            auto it = tmp.begin();
            if (it == tmp.end()) {
                log_info("Empty branch {}", branch);
                continue;
            }
            auto lo = *it, hi = *it;
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

    log_info("Making geometry");
    if (segments.empty()) {
        log_info("Empty geometry");
        return;
    }
    root = {(float) segments[0].prox.x, (float) segments[0].prox.y, (float) segments[0].prox.z};
    {
        vertices.reserve(segments.size()*n_vertices);
        indices.reserve(segments.size()*n_indices);
        ZoneScopedN("Frustra");
        for (auto ix = 0ul; ix < segments.size(); ++ix) {
            const auto& [id, prox, dist, tag] = segments[ix];
            // Shift to root and find vector along the segment
            auto c_prox = glm::vec3{prox.x, prox.y, prox.z} - root;
            auto c_dist = glm::vec3{dist.x, dist.y, dist.z} - root;
            make_frustrum_vertices(c_prox, prox.radius, c_dist, dist.radius, id, vertices, n_faces);
            make_frustrum_indices(ix*n_vertices, indices, n_faces);
        }
        if (vertices.size() != segments.size()*n_vertices) log_error("Size mismatch: vertices ./. segments");
        if (indices.size()  != segments.size()*n_indices)  log_error("Size mismatch: indices ./. segments");
    }
    log_debug("Frustra generated: {} ({} points)", indices.size()/n_indices, vertices.size());

    {
        ZoneScopedN("Rescaling");
        // Re-scale into [-1, 1]^3 box
        for (const auto& tri: vertices) {
            rescale = std::max(rescale, std::abs(tri.position.x));
            rescale = std::max(rescale, std::abs(tri.position.y));
            rescale = std::max(rescale, std::abs(tri.position.z));
        }
        for(auto& tri: vertices) tri.position /= rescale;
        log_debug("Geometry re-scaled by 1/{}", rescale);
    }
    vbo = make_buffer_object(vertices, GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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
