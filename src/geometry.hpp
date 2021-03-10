#pragma once

// needs to be at the top
#include <GL/gl3w.h>

#include <vector>
#include <unordered_map>
#include <string>
#include <fstream>

#include <arbor/morph/morphology.hpp>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>

#include "id.hpp"
#include "view_state.hpp"
#include <utils.hpp>
#include <definition.hpp>

struct point {
  glm::vec3 position = {0.0f, 0.0f, 0.0f};
  glm::vec3 normal   = {0.0f, 0.0f, 0.0f};
  glm::vec3 id       = {0.0f, 0.0f, 0.0f};
};

glm::vec4 next_color();

struct renderable {
  size_t    count     = 0;
  size_t    instances = 0;
  unsigned  vao       = 0;
  bool      active    = false;
  glm::vec4 color     = next_color();
};

struct object_id {
  size_t segment = 0;
  size_t branch  = 0;
};

struct render_ctx {
  unsigned fbo          =  0;
  unsigned post_fbo     =  0;
  unsigned tex          =  0;
  int width             = -1;
  int height            = -1;
  glm::vec4 clear_color = {214.0f/255, 214.0f/255, 214.0f/255, 1.00f};
};

struct geometry {
  geometry();

  void render(const view_state& view, const glm::vec2& size, const std::vector<renderable>&, const std::vector<renderable>&);
  void make_marker(const std::vector<glm::vec3>& points, renderable&);
  void make_region(const std::vector<size_t>& segments, renderable&);
  std::optional<object_id> get_id_at(const glm::vec2& pos, const view_state&, const glm::vec2& size, const std::vector<renderable>&);
  void clear();
  void load_geometry(const arb::morphology&);

  std::vector<point>    vertices = {};
  std::vector<unsigned> indices  = {};
  std::unordered_map<size_t, size_t> id_to_index  = {}; // map segment id to cylinder index
  std::unordered_map<size_t, size_t> id_to_branch = {}; // map segment id to branch id

  render_ctx pick;
  render_ctx cell;

  unsigned vbo            = 0;
  unsigned marker_vbo     = 0;
  unsigned region_program = 0;
  unsigned object_program = 0;
  unsigned marker_program = 0;

  // Geometry
  size_t n_faces     = 64;             // Faces on frustrum mantle
  size_t n_vertices  = n_faces*2 + 2;  // Vertices on frustrum incl cap
  size_t n_triangles = n_faces*4;      // Tris per frustrum: 4 per face for the mantle and cap
  size_t n_indices   = n_triangles*3;  // Points per frustrum draw list

  float rescale = -1;
  glm::vec3 root = {0.0f, 0.0f, 0.0f};
};
