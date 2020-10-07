#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <fstream>

#include <arbor/morph/primitives.hpp>
#include <arbor/morph/segment_tree.hpp>

#include <GL/gl3w.h>
#include <glm/glm.hpp>

#include <utils.hpp>

struct point {
    float x, y, z;
    float tag;
};

struct renderable {
    std::vector<point> triangles = {};
    unsigned vao = 0;
    unsigned tex = 0;
    bool active = false;
    glm::vec4 color;
};

struct geometry {
    std::vector<point> triangles = {};
    std::unordered_map<size_t, size_t> id_to_index = {};

    unsigned fbo     = 0;
    unsigned tex     = 0;
    unsigned program = 0;

    // Geometry
    size_t n_faces = 8;
    float rescale = -1;
    point root = {0, 0, 0, 0};

    // Viewport
    int width = -1;
    int height = -1;
    // Background
    glm::vec4 clear_color{214.0f/255, 214.0f/255, 214.0f/255, 1.00f};

    // Model transformations
    glm::vec3 translation = {0.0f, 0.0f, 0.0f};
    glm::vec3 rotation    = {0.0f, 0.0f, 0.0f};
    glm::vec3 scale       = {1.0f, 1.0f, 1.0f};

    // Camera
    float distance   = 1.75f;
    glm::vec3 camera = {distance, 0.0f, 0.0f};
    glm::vec3 up     = {0.0f, 1.0f, 0.0f};
    glm::vec3 target = {0.0f, 0.0f, 0.0f};

    geometry();

    void maybe_make_fbo(int w, int h);
    void render(float zoom, float phi, float width, float height, std::vector<renderable> to_render, std::vector<renderable> billboard);
    renderable make_markers(const std::vector<point>& points, glm::vec4 color);

    renderable derive_renderable(const std::vector<arb::msegment>& segments, glm::vec4 color);

    void load_geometry(arb::segment_tree& tree);
};
