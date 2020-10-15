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
    glm::vec3 position;
    glm::vec3 normal;
    float tag;     // texture index
};

struct renderable {
    size_t count     = 0;
    size_t instances = 0;
    unsigned vao     = 0;
    bool active      = false;
    glm::vec4 color  = {0.2f, 0.2f, 0.2f, 1.0f};
};

unsigned make_lut(std::vector<glm::vec4> colors);

struct geometry {
    std::vector<point> triangles = {};
    std::unordered_map<size_t, size_t> id_to_index = {};

    unsigned fbo     = 0;
    unsigned tex     = 0;
    unsigned region_program = 0;
    unsigned marker_program = 0;

    // Geometry
    size_t n_faces = 8;
    float rescale = -1;
    glm::vec3 root = {0.0f, 0.0f, 0.0f};

    // Viewport
    int width = -1;
    int height = -1;
    // Background
    glm::vec4 clear_color{214.0f/255, 214.0f/255, 214.0f/255, 1.00f};

    geometry();

    void maybe_make_fbo(int w, int h);
    void render(float zoom, float phi, float width, float height, std::vector<renderable> to_render, std::vector<renderable> billboard);

    renderable make_marker(const std::vector<glm::vec3>& points, glm::vec4 color);
    renderable make_region(const std::vector<arb::msegment>& segments, glm::vec4 color);

    void load_geometry(arb::segment_tree& tree);
};
