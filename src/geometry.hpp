#pragma once

// needs to be at the top
#include <GL/gl3w.h>

#include <vector>
#include <unordered_map>
#include <string>
#include <fstream>
#include <memory>

#include <arbor/morph/morphology.hpp>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "id.hpp"
#include "view_state.hpp"
#include "utils.hpp"
#include "definition.hpp"

struct point {
  glm::vec3 position = {0.0f, 0.0f, 0.0f};
  glm::vec3 normal   = {0.0f, 0.0f, 0.0f};
  glm::vec3 id       = {0.0f, 0.0f, 0.0f};
};

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
  arb::msegment data;
  std::vector<std::pair<size_t, size_t>> segment_ids;
};

struct render_ctx {
  unsigned fbo          =  0;
  unsigned post_fbo     =  0;
  unsigned tex          =  0;
  unsigned rbo          =  0;
  unsigned ms_tex       =  0;
  int width             = -1;
  int height            = -1;
  glm::vec3 clear_color = {1.0f, 1.0f, 1.0f};
};

// TODO Split this into rendering and actual geometry. ATM these are coupled in `get_object_id` via the `id_to_*` tables.
struct geometry {
  geometry();

  void render(const view_state& view, const glm::vec2& size, const std::vector<renderable>&, const std::vector<renderable>&);
  void make_marker(const std::vector<glm::vec3>& points, renderable&);
  void make_region(const std::vector<arb::msegment>& segments, renderable&);
  std::optional<object_id> get_id();
  void clear();
  void load_geometry(const arb::morphology&);

  std::vector<arb::msegment> segments;
  std::vector<point>    vertices;
  std::vector<unsigned> indices;
  std::unordered_map<size_t, size_t> id_to_index;   // map segment id to cylinder index
  std::unordered_map<size_t, size_t> id_to_branch;  // map segment id to branch id
  std::unordered_map<size_t, std::vector<std::pair<size_t, size_t>>> branch_to_ids; // map branch to segment ids

  glm::vec2 pick_pos;

  render_ctx pick;
  render_ctx cell;

  unsigned pbo            = 0;
  unsigned vbo            = 0;
  unsigned marker_vbo     = 0;
  unsigned region_program = 0;
  unsigned object_program = 0;
  unsigned marker_program = 0;

  // Geometry
  size_t n_faces     = 64;             // Faces on frustrum mantle
  size_t n_vertices  = n_faces*2 + 2;  // Faces needs 4 vertices, but 2 are shared w/ the next. Caps need three per face,
                                       // but center is shared for all, and two are shared with the faces
  size_t n_triangles = n_faces*4;      // Each face is a quad made from 2 tris, caps have one tri per face
  size_t n_indices   = n_triangles*3;  // Three indices (reference to vertex) per tri

  float rescale = -1;
  glm::vec3 root = {0.0f, 0.0f, 0.0f};
};
