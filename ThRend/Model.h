#pragma once

#define _USE_MATH_DEFINES
#define GLM_ENABLE_EXPERIMENTAL

#include <glm.hpp>
#include <vector>
#include <string>
#include "owl/common/math/vec.h"
#include "owl/common/math/box.h"
               
struct Model
{
  std::vector<glm::vec3>  vertices;
  std::vector<owl::vec3i> triangles;
  std::vector<owl::vec4i> quads;
  std::vector<int>        tris_matIDs;
  std::vector<int>        quads_matIDs;
  std::vector<float>      temps;

  inline owl::box3f getBounds() const {
    owl::box3f bounds;
    for (auto vtx : vertices) bounds.extend((const owl::vec3f&)vtx);
    return bounds;
  }
};

void loadUCD(Model &mode, const std::string &fileName);

// OWLGroup mapToOWL(const Model &model);

