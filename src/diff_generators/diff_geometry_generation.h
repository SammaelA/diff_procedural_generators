#pragma once
#include <vector>
#include "vectors.h"
struct ComplexModel;
struct Block;
namespace dgen
{
  #define FLOAT_PER_VERTEX (3+3+2) //vec3 pos, vec3 norm, vec2 tc
  void print_jackobian(const std::vector<float> &jac, int x_n, int y_n, int lines = 100);
  void print_model(const std::vector<float> &res);
  void dgen_test(std::vector<float> &model);
  bool create_model_from_block(Block &bl, ComplexModel &mod);
  void transform(std::vector<dfloat> &vert, dmat43 mat, int floats_per_vertex = FLOAT_PER_VERTEX, int pos_start = 0, int norm_start = 3);
  void create_cup(std::vector<dfloat> &params, std::vector<dfloat> &model);
};