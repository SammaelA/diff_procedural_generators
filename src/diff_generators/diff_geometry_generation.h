#pragma once
#include <vector>
#include "vectors.h"
#include <functional>
struct ComplexModel;
struct Block;
namespace dgen
{
  #define FLOAT_PER_VERTEX (3+3+2) //vec3 pos, vec3 norm, vec2 tc
  struct ModelLayout
  {
    //default layout is (pos.x, pos.y, pos.z, norm.x, norm.y, norm.z, tc.x, tc.y)
    //if some value is -1, it means that model does not have such component in vertex
    ModelLayout(): ModelLayout(0, 3, 6, 8, 8) {}
    ModelLayout(int _p, int _n, int _tc, int _end, int _offset)
    {
      pos = _p;
      norm = _n;
      tc = _tc;
      end = _end;
      f_per_vert = _offset;
    }
    union
    {
      std::array<int, 4> offsets;
      struct
      {
        int pos;
        int norm;
        int tc;
        int end;
      };
    };
    int f_per_vert = 8;
  };
  typedef std::function<void(std::vector<dfloat> &, std::vector<dfloat> &)> generator_func;
  void print_jackobian(const std::vector<float> &jac, int x_n, int y_n, int lines = 100);
  void print_model(const std::vector<float> &res);
  void dgen_test(std::vector<float> &params, std::vector<float> &model);
  void dgen_test_internal(std::vector<float> &model, generator_func func, const std::vector<float> &check_params, 
                          const std::vector<float> &params, std::vector<float> *jacobian = nullptr);
  bool check_stability(generator_func func, const std::vector<float> &params, int iterations);
  bool check_robustness(generator_func func, const std::vector<float> &params_min, const std::vector<float> &params_max, int iterations);
  bool create_model_from_block(Block &bl, ComplexModel &mod);
  void transform(std::vector<dfloat> &vert, dmat43 mat, int floats_per_vertex = FLOAT_PER_VERTEX, int pos_start = 0, int norm_start = 3);
  void transform_by_scene_parameters(std::vector<dgen::dfloat> &params, int offset, std::vector<dgen::dfloat> &model);
  
  //create model of a cup with given parameters (dishes procedural generator)
  void create_cup(const std::vector<dfloat> &params, std::vector<dfloat> &model);
  //returns 0 is each parameters is in [min+edge_size, max-edge_size] interval
  dfloat parameters_limits_reg(const std::vector<dfloat> &params, const std::vector<float> &params_min, const std::vector<float> &params_max,
                               float edge_size = 0.01);
  //estimates how weird are parametes of this cup (0 for good parameters)
  dfloat parameters_cup_reg(const std::vector<dfloat> &params);
};