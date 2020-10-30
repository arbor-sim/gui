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
#include <definition.hpp>

struct point {
  glm::vec3 position;
  glm::vec3 normal;
};

struct renderable {
  size_t count = 0;
  size_t instances = 0;
  unsigned vao = 0;
  bool active = false;
  glm::vec4 color;
  def_state state = def_state::good;
};

static glm::vec4 next_color() {
  static size_t nxt = 0;

  const static std::vector<glm::vec4>
    colors = {{166.0f/255.0f,206.0f/255.0f,227.0f/255.0f, 1.0f},
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
  return colors[nxt++];
}

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
