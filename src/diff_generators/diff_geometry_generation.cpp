#include "diff_geometry_generation.h"
#include "common_utils/utility.h"
#include "graphics_utils/modeling.h"
#include <cppad/cppad.hpp>
#include "tinyEngine/engine.h"
namespace dgen
{
  void add_vertex(std::vector<dfloat> &vert, int n, const dvec3 &pos, const dvec3 &norm, const dvec2 &tc, bool only_pos)
  {
    int sz = n*FLOAT_PER_VERTEX;
    vert.resize(sz + FLOAT_PER_VERTEX);
    vert[sz+0] = pos[0];
    vert[sz+1] = pos[1];
    vert[sz+2] = pos[2];
    if (!only_pos)
    {
      vert[sz+3] = norm[0];
      vert[sz+4] = norm[1];
      vert[sz+5] = norm[2]; 

      vert[sz+6] = tc[0];
      vert[sz+7] = tc[1];
    }
  }

  inline std::vector<float> get_triangle()
  {
    std::vector<float> res = {
      0,0,0, 0,0,1, 0,0,
      1,0,0, 0,0,1, 0,1,
      0,1,0, 0,0,1, 1,0
    };
    return res;
  }

  inline std::vector<float> get_cube_expl()
  {
    std::vector<float> res = {
      0.00,0.00,0.00, -1.00,-0.00,-0.00, 0.00,0.00,
      0.00,1.00,0.00, -1.00,-0.00,-0.00, 0.00,1.00,
      0.00,0.00,1.00, -1.00,-0.00,-0.00, 1.00,0.00,
      0.00,1.00,1.00, -1.00,-0.00,-0.00, 1.00,1.00,
      0.00,0.00,1.00, -1.00,-0.00,-0.00, 1.00,0.00,
      0.00,1.00,0.00, -1.00,-0.00,-0.00, 0.00,1.00,

      0.00,1.00,0.00, 0.00,1.00, 0.00, 0.00,1.00,
      0.00,1.00,1.00, 0.00,1.00, 0.00, 1.00,1.00,
      1.00,1.00,0.00, 0.00,1.00, 0.00, 0.00,1.00,
      1.00,1.00,1.00, 0.00,1.00, 0.00, 1.00,1.00,
      1.00,1.00,0.00, 0.00,1.00, 0.00, 0.00,1.00,
      0.00,1.00,1.00, 0.00,1.00, 0.00, 1.00,1.00,

      1.00,1.00,1.00, 1.00,-0.00,-0.00, 1.00,1.00,
      1.00,0.00,1.00, 1.00,-0.00,-0.00, 1.00,0.00,
      1.00,1.00,0.00, 1.00,-0.00,-0.00, 0.00,1.00,
      1.00,0.00,0.00, 1.00,-0.00,-0.00, 0.00,0.00,
      1.00,1.00,0.00, 1.00,-0.00,-0.00, 0.00,1.00,
      1.00,0.00,1.00, 1.00,-0.00,-0.00, 1.00,0.00,
      
      0.00,0.00,0.00, 0.00,-1.00,0.00, 0.00,0.00,
      0.00,0.00,1.00, 0.00,-1.00,0.00, 1.00,0.00,
      1.00,0.00,0.00, 0.00,-1.00,0.00, 0.00,0.00,
      1.00,0.00,1.00, 0.00,-1.00,0.00, 1.00,0.00,
      0.00,0.00,1.00, 0.00,-1.00,0.00, 1.00,0.00,
      1.00,0.00,0.00, 0.00,-1.00,0.00, 0.00,0.00,

      0.00,0.00,0.00, 0.00,-0.00,-1.00, 0.00,0.00,
      1.00,0.00,0.00, 0.00,-0.00,-1.00, 0.00,0.00,
      0.00,1.00,0.00, 0.00,-0.00,-1.00, 0.00,1.00,
      1.00,1.00,0.00, 0.00,-0.00,-1.00, 0.00,1.00,
      0.00,1.00,0.00, 0.00,-0.00,-1.00, 0.00,1.00,
      1.00,0.00,0.00, 0.00,-0.00,-1.00, 0.00,0.00,

      0.00,0.00,1.00, 0.00,-0.00,1.00, 1.00,0.00,
      1.00,0.00,1.00, 0.00,-0.00,1.00, 1.00,0.00,
      0.00,1.00,1.00, 0.00,-0.00,1.00, 1.00,1.00,
      1.00,1.00,1.00, 0.00,-0.00,1.00, 1.00,1.00,
      0.00,1.00,1.00, 0.00,-0.00,1.00, 1.00,1.00,
      1.00,0.00,1.00, 0.00,-0.00,1.00, 1.00,0.00
    };
    return res;
  }

  void model_to_simple_model(Mesh *m, std::vector<float> &s_model)
  {
    s_model.resize(m->indices.size()*FLOAT_PER_VERTEX);
    int pos = 0;
    bool have_normals = (m->normals.size() == m->positions.size());
    for (int ind : m->indices)
    {
      s_model[pos] = m->positions[3*ind];
      s_model[pos+1] = m->positions[3*ind+1];
      s_model[pos+2] = m->positions[3*ind+2];
      
      if (have_normals)
      {
        s_model[pos+3] = m->normals[3*ind];
        s_model[pos+4] = m->normals[3*ind+1];
        s_model[pos+5] = m->normals[3*ind+2];
      }
      else
      {
        s_model[pos+3] = 1;
        s_model[pos+4] = 0;
        s_model[pos+5] = 0;
      }
      if (m->colors.size()/4 >= m->positions.size()/3)
      {
        s_model[pos+6] = m->colors[4*ind];
        s_model[pos+7] = m->colors[4*ind+1];
      }
      else
      {
        s_model[pos+6] = 0;
        s_model[pos+7] = 0;
      }
      pos+=FLOAT_PER_VERTEX;
    }
  }

  std::vector<float> get_cube()
  {
    Box b = Box(glm::vec3(0,0,0), glm::vec3(1,0,0), glm::vec3(0,1,0), glm::vec3(0,0,1));
    Mesh m;
    visualizer::box_to_model(&b, &m);

    std::vector<float> res;
    model_to_simple_model(&m, res);
    return res;
  }

  inline void add_model(std::vector<dfloat> &dst, const std::vector<dfloat> &src)
  {
    dst.reserve(dst.size()+src.size());
    for (int i=0;i<src.size();i++)
      dst.push_back(src[i]);
  }

  inline void add_model(std::vector<dfloat> &dst, const std::vector<float> &src)
  {
    dst.reserve(dst.size()+src.size());
    for (int i=0;i<src.size();i++)
      dst.push_back(src[i]);
  }

  inline void shift(std::vector<dfloat> &vert, dvec3 shift)
  {
    for (int i=0;i<vert.size()/FLOAT_PER_VERTEX;i++)
    {
      vert[FLOAT_PER_VERTEX*i] += shift[0];
      vert[FLOAT_PER_VERTEX*i+1] += shift[1];
      vert[FLOAT_PER_VERTEX*i+2] += shift[2];
    }
  }

  void transform(std::vector<dfloat> &vert, dmat43 mat, int floats_per_vertex, int pos_start, int norm_start)
  {
    dmat43 norm_mat = transposedInverse3x3(mat);

    if (norm_start >= 0)
    {
      for (int i=0;i<vert.size()/floats_per_vertex;i++)
      {
        mulp(mat, vert[pos_start+floats_per_vertex*i], vert[pos_start+floats_per_vertex*i+1], vert[pos_start+floats_per_vertex*i+2]);
        mulv(norm_mat, vert[norm_start+floats_per_vertex*i], vert[norm_start+floats_per_vertex*i+1], vert[norm_start+floats_per_vertex*i+2]);

        dfloat a = vert[norm_start+floats_per_vertex*i];
        dfloat b = vert[norm_start+floats_per_vertex*i+1];
        dfloat c = vert[norm_start+floats_per_vertex*i+2];
        dfloat len = CppAD::sqrt(a*a + b*b + c*c) + 1e-18;
        vert[norm_start+floats_per_vertex*i] = a/len;
        vert[norm_start+floats_per_vertex*i+1] = b/len;
        vert[norm_start+floats_per_vertex*i+2] = c/len;
      }
    }
    else
    {
      for (int i=0;i<vert.size()/floats_per_vertex;i++)
      {
        mulp(mat, vert[pos_start+floats_per_vertex*i], vert[pos_start+floats_per_vertex*i+1], vert[pos_start+floats_per_vertex*i+2]);
      }
    }
  }

  void transform(std::vector<dvec3> &verts, dmat43 mat)
  {
    for (auto &vert : verts)
    {
      vert = mulp(mat, vert); 
    }
  }

  void test_model(std::vector<dfloat> &vert, std::vector<dfloat> &params)
  {
    dvec3 shift_v{params[0], params[1], params[2]};
    dvec3 scale_v{params[3], params[4], params[5]};
    std::vector<dfloat> tri_model;
    add_model(tri_model, get_cube_expl());

    dvec3 axis{0,1,0};
    dmat43 mat = ident();
    mat = rotate(mat, axis, PI/4);
    mat = translate(mat, shift_v);
    mat = scale(mat, scale_v);
    transform(tri_model, mat);
    for (int i=0;i<4;i++)
    {
      for (int j=0;j<3;j++)
        debug("%.3f ", mat[3*i+j]);
      debugnl();
    }
    add_model(vert, tri_model);
  }

  void print_model(const std::vector<float> &res)
  {
    debug("Model\n");
    for (int i=0;i<res.size()/FLOAT_PER_VERTEX;i++)
    {
      int st = i*FLOAT_PER_VERTEX;
      debug("(%.2f %.2f %.2f) (%.2f %.2f %.2f) (%.2f %.2f)\n",
            res[st], res[st+1], res[st+2],
            res[st+3], res[st+4], res[st+5],
            res[st+6], res[st+7]);
    }
  }

  void print_jackobian(const std::vector<float> &jac, int x_n, int y_n, int lines)
  {
    std::string names[FLOAT_PER_VERTEX] = {" pos_x", " pos_y", " pos_z", 
                                           "norm_x", "norm_y", "norm_z",
                                           "  tc_x", "  tc_y"};
    debug("Jacobian    ");
    for (int j = 0; j < x_n; j++)
      debug("x_%.2d ", j);
    debugnl();
    for (int i = 0; i < MIN(y_n, lines); i++)
    {
      if (i % FLOAT_PER_VERTEX == 0)
      {
        for (int k = 0; k < 13 + 5*x_n; k++)
          debug("-");
        debugnl();
      }
      debug("%s_%.4d ", names[i % FLOAT_PER_VERTEX].c_str(), i/FLOAT_PER_VERTEX);
      for (int j = 0; j < x_n; j++)
        debug("%.2f ", jac[i * x_n + j]);
      debugnl();
    }
  }

  dfloat smoothmax(dfloat a, dfloat b, float alpha)
  {
    return (a*exp(a*alpha) + b*exp(b*alpha))/(exp(a*alpha) + exp(b*alpha));
  }
  dfloat smoothmin(dfloat a, dfloat b, float alpha)
  {
    return (a*exp(-a*alpha) + b*exp(-b*alpha))/(exp(-a*alpha) + exp(-b*alpha));
  }
  dfloat smoothclamp(dfloat a, dfloat from, dfloat to, float alpha)
  {
    return smoothmin(smoothmax(a, from, alpha), to, alpha);
  }

  dfloat default_parameters_reg(const std::vector<dfloat> &params)
  {
    return 0;
  }
  dfloat default_model_reg(const std::vector<dfloat> &params, const std::vector<dfloat> &model)
  {
    return 0;
  }

  dfloat parameters_limits_reg(const std::vector<dfloat> &params, const std::vector<float> &params_min, const std::vector<float> &params_max,
                               float edge_size)
  {
    dfloat res = 0;
    for (int i = 0; i < params.size(); i++)
    {
      res += smoothmax((params_min[i] + edge_size - params[i])/edge_size, 0.0f, 8);
      res += smoothmax((params[i] - (params_max[i] - edge_size))/edge_size, 0.0f, 8);
    }
    res = smoothmax(smoothmax(smoothmax(res, 0, 8), 0, 8), 0, 8);
    return res;
  }

  void transform_by_scene_parameters(std::vector<dgen::dfloat> &params, int offset, std::vector<dgen::dfloat> &model)
  {
    dmat43 rot = rotate(ident(), dvec3{1,0,0}, params[offset]);
    rot = rotate(rot, dvec3{0,1,0}, params[offset+1]);
    rot = rotate(rot, dvec3{0,0,1}, params[offset+2]);
    dmat43 tr = translate(ident(), dvec3{params[offset+3], params[offset+4], params[offset+5]});
    rot = mul(tr, rot);
    transform(model, rot);
  }

  bool check_stability(generator_func func, const std::vector<float> &params, int iterations)
  {
    float eps = 1e-4;
    std::vector<float> model_ref;
    std::vector<float> jac_ref;
    dgen_test_internal(model_ref, func, params, params, &jac_ref, false);
    int x_n = params.size();
    int y_n = model_ref.size();
    debug("Checking stability of differential procedural model\n");
    debug("Model has %d params and %d vertices\n", x_n, y_n/FLOAT_PER_VERTEX);

    std::vector<int> failed_params;
    for (int param_n = 0; param_n < x_n; param_n++)
    {
      int failed_tests = 0;
      for (int i=0;i<iterations;i++)
      {
        std::vector<float> par = params;
        par[param_n] *= urand(0.5, 2);
        std::vector<float> model;
        std::vector<float> jac;
        bool model_created = false;
        try
        {
          dgen_test_internal(model, func, par, params, &jac, false);
          model_created = true;
        }
        catch(const std::exception& e)
        {
          std::cerr << e.what() << '\n';
        }
        if (!model_created)
        {
          debug("Test %d failed. Generator crashed", i);
        }
        else if (model.size() != model_ref.size())
        {
          debug("Test %d failed. Model has wrong number of vertices (%d)\n", i, model.size()/FLOAT_PER_VERTEX);
          failed_tests++;
        }
        else
        {
          int diff_mod = 0;
          for (int j=0;j<model.size();j++)
          {
            if (abs(model[j] - model_ref[j])/(abs(model[j] + model_ref[j]) + eps) > eps)
              diff_mod++;
          }
          int diff_jac = 0;
          for (int j=0;j<jac.size();j++)
          {
            if (abs(jac[j] - jac_ref[j])/(abs(jac[j] + jac_ref[j]) + eps) > eps)
              diff_jac++;
          }
          if (diff_jac > 0 || diff_mod > 0)
            failed_tests++;
          if (diff_mod > 0)
          {
            debug("Test %d failed. Model has difference in %d values with the base one\n", i, diff_mod);
          }
          if (diff_jac > 0)
          {
            debug("Test %d failed. Jacobian has difference in %d values with the base one\n", i, diff_jac);
          }
        }
      }

      if (failed_tests == 0)
      {
        debug("Param %d. Stability check PASSED\n", param_n);
      }
      else
      {
        debug("Param %d. Stability check FAILED (%d/%d fails)\n", param_n, failed_tests, iterations);
        failed_params.push_back(param_n);
      }
    }

    if (failed_params.empty())
    {
      debug("Stability check PASSED\n");
      return true;
    }
    else
    {
      debug("Stability check FAILED\n");
      debug("%d unstable parameters: {", failed_params.size());
      for (int i=0;i<failed_params.size();i++)
      {
        debug("%d", failed_params[i]);
        if (i != failed_params.size() - 1)
          debug(", ");
      }
      debug("}\n");

      return false;
    }
  }

  bool check_robustness(generator_func func, const std::vector<float> &params_min, const std::vector<float> &params_max, int iterations)
  {
    for (int i=0;i<iterations;i++)
    {
      std::vector<float> X0;
      std::vector<float> model;
      for (int j=0;j<params_min.size();j++)
      {
        float rnd = urand(0,1);
        X0.push_back(rnd*params_min[j] + (1-rnd)*params_max[j]);
      }
      dgen_test_internal(model, func, X0, X0, nullptr, false);
    }
    return true;
  }

  void dgen_test(std::string generator_name, std::vector<float> &params, std::vector<float> &model, bool transform_by_scene)
  {
    GeneratorDescription gd = get_generator_by_name(generator_name);
    dgen_test_internal(model, gd.generator, params, params, nullptr, transform_by_scene);
  }
  void dgen_test_internal(std::vector<float> &model, generator_func func, const std::vector<float> &check_params, 
                          const std::vector<float> &params, std::vector<float> *jacobian, bool transform_by_scene)
  {
    assert(check_params.size() > 0);
    assert(check_params.size() == params.size());
    assert(model.empty());

    size_t x_n = check_params.size();
    std::vector<dfloat> X(x_n);
    std::vector<dfloat> Y;
    for (int i=0;i<x_n;i++)
      X[i] = check_params[i];

    // declare independent variables and start recording operation sequence
    CppAD::Independent(X);
    func(X, Y, ModelQuality(false, 1));
    if (transform_by_scene)
    {
      int offset = params.size() - 6;
      dgen::transform_by_scene_parameters(X, offset, Y);
    }
    size_t y_n = Y.size();
    CppAD::ADFun<float> f(X, Y); // store operation sequence in f: X -> Y and stop recording

    // compute derivative using operation sequence stored in f
    std::vector<float> jac(y_n * x_n); // Jacobian of f (m by n matrix)
    std::vector<float> res(y_n); 
    std::vector<float> X0(x_n);        // domain space vector
    for (int i=0;i<x_n;i++)
      X0[i] = params[i];
  
    jac = f.Jacobian(X0); // Jacobian for operation sequence
    res = f.Forward(0, X0);

    //print_model(res);
    //print_jackobian(jac, x_n, y_n);

    model = res;
    if (jacobian)
      *jacobian = jac;
  }

  bool create_model_from_block(Block &bl, ComplexModel &mod)
  {
    GeneratorDescription gd = get_generator_by_name("dishes");
    Model *m = new Model();
    std::vector<float> X0{2.014, 3.424, 3.404, 3.291, 3.276, 3.284, 3.357, 3.383, 3.354,
                          0.781,
                          1,
                          0.05,
                          0.7,
                          0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.4};
    std::vector<float> res;
    dgen::dgen_test("dishes", X0, res);
    visualizer::simple_mesh_to_model_332(res, m);

    mod.models.push_back(m);
    mod.materials.push_back(Material(engine::textureManager->get("porcelain")));
    mod.update();
    return true;
  }
}