#pragma once

// needs to be at the top
#include <GL/gl3w.h>

#include <vector>
#include <unordered_map>
#include <string>
#include <fstream>

#include <arbor/morph/primitives.hpp>
#include <arbor/morph/segment_tree.hpp>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <utils.hpp>

struct point {
    glm::vec3 position;
    glm::vec3 normal;
};

constexpr float PHI = 1.61803398875;

static glm::vec4 next_color() {
    static glm::vec4 color = {0.1, 0.2, 0.4, 1.0f};
    color.x = std::fmod(color.x*PHI, 1.0f);
    color.y = std::fmod(color.y*PHI, 1.0f);
    color.z = std::fmod(color.z*PHI, 1.0f);
    return color;
}

struct renderable {
    size_t count        = 0;
    size_t instances    = 0;
    unsigned vao        = 0;
    bool active         = false;
    glm::vec4 color  = {0.2f, 0.2f, 0.2f, 1.0f};
};

struct geometry {
    geometry();
    geometry(const arb::segment_tree&);

    unsigned long render(float zoom, float phi,
                         float width, float height,
                         const std::vector<renderable>&, const std::vector<renderable>&);

    renderable make_marker(const std::vector<glm::vec3>& points, glm::vec4 color);
    renderable make_region(const std::vector<arb::msegment>& segments, glm::vec4 color);

    void load_geometry(const arb::segment_tree& tree);

    private:
        void maybe_make_fbo(int w, int h);
        std::vector<point> triangles = {};
        std::unordered_map<size_t, size_t> id_to_index = {};

        unsigned fbo     = 0;
        unsigned tex     = 0;
        unsigned region_program = 0;
        unsigned marker_program = 0;

        // Geometry
        size_t n_faces = 32;
        float rescale = -1;
        glm::vec3 root = {0.0f, 0.0f, 0.0f};

        // Viewport
        int width = -1;
        int height = -1;

        // Background
        glm::vec4 clear_color{214.0f/255, 214.0f/255, 214.0f/255, 1.00f};
};
