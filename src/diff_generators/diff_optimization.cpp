#include "diff_optimization.h"
#include "differentiable_generators.h"
#include "mitsuba_python_interaction.h"
#include "common_utils/distribution.h"
#include <cppad/cppad.hpp>
#include "common_utils/utility.h"
#include <functional>
#include <chrono>
#include <algorithm>
#include "common_utils/blk.h"
#include "tinyEngine/engine.h"
#include "graphics_utils/silhouette.h"
#include "save_utils/csv.h"
#include "graphics_utils/model_texture_creator.h"
#include "graphics_utils/modeling.h"
#include "common_utils/optimization/optimization.h"
#include "diff_generators/depth_extract_compare.h"

namespace dopt
{
  class UShortVecComparator
  {
  public:
    bool operator()(const std::vector<unsigned short> &v1, const std::vector<unsigned short> &v2) const
    {
      for (int i = 0; i < MIN(v1.size(), v2.size()); i++)
      {
        if (v1[i] < v2[i])
          return true;
        else if (v1[i] > v2[i])
          return false;
      }
      return false;
    }
  };

  class DiffFunctionEvaluator
  {
  public:
    ~DiffFunctionEvaluator()
    {
      for (auto f : functions)
      {
        if (f)
          delete f;
      }
    }
    void init(dgen::generator_func _model_creator, std::vector<unsigned short> &variant_positions)
    {
      model_creator = _model_creator;
      variant_params_positions = variant_positions;
    }
    std::vector<float> get(const std::vector<float> &params, dgen::ModelQuality mq = dgen::ModelQuality())
    {
      return functions[find_or_add(params, mq)]->Forward(0, params); 
    }
    std::vector<float> get_jac(const std::vector<float> &params, dgen::ModelQuality mq = dgen::ModelQuality())
    {
      return functions[find_or_add(params, mq)]->Jacobian(params); 
    }
    std::vector<float> get_transformed(const std::vector<float> &params, const std::vector<float> &camera_params, 
                                       dgen::ModelQuality mq = dgen::ModelQuality())
    {
      std::vector<float> f_model = functions[find_or_add(params, mq)]->Forward(0, params); 
      std::vector<dgen::dfloat> model(f_model.size());
      for (int i=0;i<f_model.size();i++)
        model[i] = f_model[i];

      dgen::dmat43 rot = dgen::rotate(dgen::ident(), dgen::dvec3{1,0,0}, camera_params[3]);
      rot = dgen::rotate(rot, dgen::dvec3{0,1,0}, camera_params[4]);
      rot = dgen::rotate(rot, dgen::dvec3{0,0,1}, camera_params[5]);
      dgen::dmat43 tr = dgen::translate(dgen::ident(), dgen::dvec3{camera_params[0], camera_params[1], camera_params[2]});
      rot = dgen::mul(tr, rot);
      dgen::transform(model, rot);

      for (int i=0;i<f_model.size();i++)
        f_model[i] = CppAD::Value(model[i]);
      
      return f_model;
    }
  private:
    int find_or_add(const std::vector<float> &params, dgen::ModelQuality mq)
    {
      auto vs = get_variant_set(params);
      vs.push_back((unsigned short)mq.create_only_position);
      vs.push_back((unsigned short)mq.quality_level);
      auto it = variant_set_to_function_pos.find(vs);
      if (it == variant_set_to_function_pos.end())
      {
        debug("added new function {");
        for (auto &v : vs)
          debug("%d ", (int)v);
        debug("}\n");
        std::vector<dgen::dfloat> X(params.size());
        for (int i=0;i<params.size();i++)
          X[i] = params[i];
        std::vector<dgen::dfloat> Y;
        CppAD::Independent(X);
        model_creator(X, Y, mq);
        CppAD::ADFun<float> *f = new CppAD::ADFun<float>(X, Y); 
        functions.push_back(f);
        output_sizes.push_back(Y.size());
        int f_pos = functions.size()-1;
        variant_set_to_function_pos.emplace(vs, f_pos);

        return f_pos;
      }
      return it->second;
    }

    std::vector<unsigned short> get_variant_set(const std::vector<float> &params)
    {
      std::vector<unsigned short> vs;
      for (auto &pos : variant_params_positions)
      {
        vs.push_back((unsigned short)round(params[pos]));
      }
      return vs;
    }
    dgen::generator_func model_creator;
    std::map<std::vector<unsigned short>, int, UShortVecComparator> variant_set_to_function_pos;
    std::vector<CppAD::ADFun<float> *> functions;
    std::vector<int> output_sizes;//same size as functions vector
    std::vector<unsigned short> variant_params_positions;
  };

  struct OptimizationResult
  {
    std::vector<float> best_params;
    float best_err;
    int total_iters;
  };

  float parameters_error(const std::vector<float> &params, const std::vector<float> &ref_params,
                         const std::vector<float> &params_min, const std::vector<float> &params_max)
  {
    float err = 0;
    for (int i=0;i<params.size();i++)
    {
      err += abs(params[i] - ref_params[i]) / (params_max[i] - params_min[i]);
    }
    return err/params.size();
  }

  void load_presets_from_blk(Block &presets_blk, Block &gen_params, const std::vector<float> &init_params, 
                             std::vector<std::vector<float>> &parameter_presets /*output*/)
  {
    // presets are manually created valid sets of generator parmeters, representing different types of objects,
    // that generator can create. They do not include scene parameters

    int gen_params_cnt = gen_params.size();
    std::map<std::string, int> param_n_by_name;
    for (int i = 0; i < gen_params.size(); i++)
    {
      Block *pb = gen_params.get_block(i);
      if (pb)
      {
        param_n_by_name.emplace(gen_params.get_name(i), i);
      }
    }
    for (int i = 0; i < presets_blk.size(); i++)
    {
      Block *preset_block = presets_blk.get_block(i);
      if (preset_block)
      {
        std::vector<float> preset = init_params;
        if (preset_block->has_tag("compact"))
        {
          std::vector<float> params;
          preset_block->get_arr("params", params);
          if (params.size() == gen_params_cnt)
          {
            for (int k = 0; k < gen_params_cnt; k++)
              preset[k] = params[k];
          }
          else
          {
            logerr("Error: compact preset %s has %d parameters, while the generator requests %d",
                   presets_blk.get_name(i).c_str(), params.size(), gen_params_cnt);
          }
        }
        else
        {
          for (int j = 0; j < preset_block->size(); j++)
          {
            auto it = param_n_by_name.find(preset_block->get_name(j));
            if (it != param_n_by_name.end())
            {
              float val = 0;
              if (preset_block->get_type(i) == Block::ValueType::DOUBLE)
                val = preset_block->get_double(i);
              else if (preset_block->get_type(i) == Block::ValueType::INT)
                val = preset_block->get_int(i);
              else if (preset_block->get_type(i) == Block::ValueType::BOOL)
                val = (int)(preset_block->get_bool(i));
              else
                logerr("parameter %s of preset %s has unknown parameter type. It should be double, int or bool",
                       preset_block->get_name(j).c_str(), presets_blk.get_name(i).c_str());
              preset[it->second] = val;
            }
            else
            {
              logerr("Unknown parameter name %s in preset %s",
                     preset_block->get_name(j).c_str(), presets_blk.get_name(i).c_str());
            }
          }
        }
        parameter_presets.push_back(preset);
        debug("read preset %s \n", presets_blk.get_name(i).c_str());
        for (int k = 0; k < gen_params_cnt; k++)
          debug("%f ", preset[k]);
        debugnl();
      }
    }
  }

  float image_based_optimization(Block &settings_blk, MitsubaInterface &mi)
  {
    Block gen_params, scene_params, presets_blk;
    std::vector<float> params_min, params_max, init_params;
    std::vector<unsigned short> init_bins_count;
    std::vector<unsigned short> init_bins_positions;
    std::vector<unsigned short> variant_count;
    std::vector<unsigned short> variant_positions;
    std::vector<std::vector<float>> parameter_presets;
    dgen::GeneratorDescription generator = dgen::get_generator_by_name(settings_blk.get_string("procedural_generator"));
    load_block_from_file(generator.parameters_description_blk_path, gen_params);
    load_block_from_file(settings_blk.get_string("scene_description"), scene_params);
    load_block_from_file(generator.presets_blk_path, presets_blk);
    int gen_params_cnt = gen_params.size();
    int scene_params_cnt = scene_params.size();

    int verbose_level = settings_blk.get_int("verbose_level", 1);
    int ref_image_size = settings_blk.get_int("reference_image_size", 512);
    int sel_image_size = settings_blk.get_int("selection_image_size", 196);
    bool by_reference = settings_blk.get_bool("synthetic_reference", true);
    bool texture_extraction = settings_blk.get_bool("texture_extraction", false);
    std::string search_algorithm = settings_blk.get_string("search_algorithm", "simple_search");
    std::string save_stat_path = settings_blk.get_string("save_stat_path", "");
    std::string saved_result_path = settings_blk.get_string("saved_result_path", "saves/selected_final.png");
    std::string saved_textured_path = settings_blk.get_string("saved_textured_path", "saves/selected_textured.png");
    std::string saved_initial_path = settings_blk.get_string("saved_initial_path", "");
    std::string reference_path = settings_blk.get_string("reference_path", "");

    auto get_gen_params = [&](const std::vector<float> &params) -> std::vector<float>
    {
      std::vector<float> gp = std::vector<float>(params.begin(), params.begin() + gen_params_cnt);
      return gp;
    };
    auto get_camera_params = [&](const std::vector<float> &params) -> std::vector<float>
    {
      std::vector<float> gp = std::vector<float>(params.begin() + gen_params_cnt, params.end());
      return gp;
    };

    auto process_blk = [&](Block &blk){
      for (int i=0;i<blk.size();i++)
      {
        Block *pb = blk.get_block(i);
        if (!pb && pb->size()>0)
        {
          logerr("invalid parameter description\"%s\". It should be non-empty block", blk.get_name(i));
        }
        else
        {
          glm::vec2 min_max = pb->get_vec2("values", glm::vec2(1e9,-1e9));
          if (min_max.x > min_max.y)
            logerr("invalid parameter description\"%s\". It should have values:p2 with min and max values", blk.get_name(i));
          params_min.push_back(min_max.x);
          params_max.push_back(min_max.y);
          init_params.push_back(0.5*(min_max.x + min_max.y));

          int bins_cnt = pb->get_int("init_bins_count", 0);
          bool is_variant = pb->get_bool("is_variant", false);
          if (is_variant)
          {
            if (bins_cnt > 0)
            {
              logerr("invalid parameter description\"%s\". Variant parameter should not have explicit init_bins_count", blk.get_name(i));
            }
            int imin = round(min_max.x);
            int imax = round(min_max.y);
            if (abs((float)imin - min_max.x) > 1e-3 || abs((float)imax - min_max.y) > 1e-3)
            {
              logerr("invalid parameter description\"%s\". Variant parameter should have integer min max values", blk.get_name(i));
            }
            bins_cnt = imax - imin + 1;
            variant_count.push_back(bins_cnt);
            variant_positions.push_back(params_min.size()-1);
          }
          if (bins_cnt<0 || bins_cnt>512)
          {
            bins_cnt = 0;
            logerr("invalid parameter description\"%s\". Bin count should be in [0, 512] interval", blk.get_name(i));
          }

          if (bins_cnt>0)
          {
            init_bins_count.push_back((unsigned short)bins_cnt);
            init_bins_positions.push_back(params_min.size()-1);
          }
        }
      }
    };

    debug("Starting image-based optimization. Target function has %d parameters (%d for generator, %d for scene). %d SP %d var\n", 
          gen_params_cnt + scene_params_cnt, gen_params_cnt, scene_params_cnt, init_bins_count.size(), variant_count.size());

    DiffFunctionEvaluator func;
    func.init(generator.generator, variant_positions);

    std::vector<Texture> reference_tex, reference_mask, reference_depth;
    int cameras_count = 1;

    CameraSettings camera;
    camera.origin = glm::vec3(0, 0.5, 1.5);
    camera.target = glm::vec3(0, 0.5, 0);
    camera.up = glm::vec3(0, 1, 0);

    DepthLossCalculator dlc;

    if (by_reference)
    {
      std::vector<float> reference_params;

      settings_blk.get_arr("reference_params", reference_params);
      if (reference_params.size() != gen_params_cnt)
      {
        logerr("DOpt Error: reference_params has %d values, it should have %d", reference_params.size(), gen_params_cnt);
        return 1.0;
      }

      Block *reference_cameras_blk = settings_blk.get_block("reference_cameras");
      if (!reference_cameras_blk || reference_cameras_blk->size() == 0)
      {
        logerr("DOpt Error: reference_cameras block not found");
        return 1.0;
      }
      cameras_count = reference_cameras_blk->size();
      for (int i=0;i<reference_cameras_blk->size();i++)
      {
        std::vector<float> reference_camera_params;
        reference_cameras_blk->get_arr(i, reference_camera_params);
        std::vector<float> reference = func.get_transformed(reference_params, reference_camera_params);
        mi.init_scene_and_settings(MitsubaInterface::RenderSettings(ref_image_size, ref_image_size, 256, MitsubaInterface::LLVM, MitsubaInterface::MONOCHROME));
        mi.render_model_to_file(reference, "saves/reference.png", dgen::ModelLayout(), camera);
        reference_tex.push_back(engine::textureManager->load_unnamed_tex("saves/reference.png"));

        Model *m = new Model();
        visualizer::simple_mesh_to_model_332(reference, m);
        m->update();
        reference_depth.push_back(dlc.get_depth(*m, camera, 128, 128));
        delete m;
      }
    }
    else
    {
      cameras_count = 1;
      reference_tex.push_back(engine::textureManager->load_unnamed_tex(reference_path));
      reference_depth.push_back(engine::textureManager->create_texture(128, 128));//TODO: estimate depth
    }

    SilhouetteExtractor se = SilhouetteExtractor(1.0f, 0.075, 0.225);
    std::vector<std::string> reference_images_dir;
    for (int i=0;i<cameras_count;i++)
    {
      reference_mask.push_back(se.get_silhouette(reference_tex[i], sel_image_size, sel_image_size));
      reference_images_dir.push_back("saves/reference_" + std::to_string(i) + ".png");
      engine::textureManager->save_png_directly(reference_mask[i], reference_images_dir.back());
    }

    process_blk(gen_params);
    for (int i=0;i<cameras_count;i++)
      process_blk(scene_params);//we dublicate scene parameters for each camera (currently scene == camera only)

    load_presets_from_blk(presets_blk, gen_params, init_params, parameter_presets);

    CppAD::ADFun<float> f_reg;
    {
      std::vector<dgen::dfloat> X(init_params.size());
      for (int i = 0; i < init_params.size(); i++)
        X[i] = init_params[i];
      std::vector<dgen::dfloat> Y;
      CppAD::Independent(X);
      Y.resize(1);
      Y[0] = generator.params_regularizer(X);
      f_reg = CppAD::ADFun<float>(X, Y);
    }


    mi.init_optimization(reference_images_dir, MitsubaInterface::LOSS_MIXED, 1 << 16, dgen::ModelLayout(0, 3, 3, 3, 8), 
                         MitsubaInterface::RenderSettings(sel_image_size, sel_image_size, 1, MitsubaInterface::LLVM, MitsubaInterface::SILHOUETTE),
                         cameras_count, settings_blk.get_bool("save_intermediate_images", false));

    OptimizationResult opt_result{init_params, 1000, 0};

    Block *opt_settings = settings_blk.get_block(search_algorithm);
    if (!opt_settings)
    {
      logerr("Optimizer algorithm %s does not have settings block", search_algorithm.c_str());
      return 1.0;
    }
    else
    {
      int iters = 0;
      double total_time_ms = 0;

      int cnt = 0;
      std::vector<double> grad_stat;

      opt::opt_func_with_grad F_silhouette = [&](std::vector<float> &params) -> std::pair<float,std::vector<float>>
      {
        std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
        for (int i=0;i<params.size();i++)
          params[i] = CLAMP(params[i], params_min[i], params_max[i]);
        bool verbose = verbose_level > 1;
        if (verbose)
        {
          debug("params [");
          for (int j=0;j<params.size();j++)
          {
            debug("%.3f, ", params[j]);
          }
          debug("]\n");
        }
        std::vector<float> jac = func.get_jac(get_gen_params(params), dgen::ModelQuality(true, 0));
        std::vector<float> res = func.get(get_gen_params(params), dgen::ModelQuality(true, 0)); 
        std::vector<float> final_grad = std::vector<float>(params.size(), 0);

        float loss = mi.render_and_compare(res, camera, get_camera_params(params));
        mi.compute_final_grad(jac, gen_params_cnt, res.size()/FLOAT_PER_VERTEX, final_grad);
        float reg_q = 0.01;
        std::vector<float> reg_res = f_reg.Forward(0, params);
        std::vector<float> reg_jac = f_reg.Jacobian(params);
        //logerr("reg_res[0] = %f",reg_res[0]);
        loss += reg_q*MAX(0, reg_res[0]);
        for (int i=0;i<MIN(final_grad.size(), reg_jac.size());i++)
          final_grad[i] += reg_q*reg_jac[i];

        if (verbose)
        {
          debug("iter [%d] loss = %.3f\n", opt_result.total_iters, loss);
          debug("grad {");
          for (int j=0;j<final_grad.size();j++)
          {
            debug("%.3f ", final_grad[j]);
          }
          debug("}\n");
        }
        std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
        total_time_ms += 1e-3 * std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        iters++;
        if (cnt == 0)
        {
          grad_stat = std::vector<double>(final_grad.size(), 0);
        }
        for (int j = 0; j < final_grad.size(); j++)
          grad_stat[j] += abs(final_grad[j]);
        cnt++;
        if (cnt % 100 == 0)
        {
          debug("grad stat [");
          for (int j = 0; j < final_grad.size(); j++)
            debug("%.2f ", (float)(1000*grad_stat[j]/cnt));
          debug("]\n");
        }
        return std::pair<float,std::vector<float>>(loss, final_grad);
      };

      opt::opt_func F_depth_reg = [&](std::vector<float> &params) -> float
      {
        std::vector<float> res = func.get_transformed(get_gen_params(params), get_camera_params(params), dgen::ModelQuality(true, 1)); 
        Model *m = new Model();
        visualizer::simple_mesh_to_model_332(res, m);
        m->update();
        if (reference_depth.size() > 1)
        {
          logerr("Warning: you are using F_depth_reg function with more than one camera estimation. It's useless");
        }
        float val = dlc.get_loss(*m, reference_depth[0], camera);
        delete m;
        return val;
      };

      opt::Optimizer *opt = nullptr;
      if (search_algorithm == "adam")
        opt = new opt::Adam();
      else if (search_algorithm == "DE")
        opt = new opt::DifferentialEvolutionOptimizer();
      else if (search_algorithm == "memetic_classic")
        opt = new opt::MemeticClassic();
      else if (search_algorithm == "grid_search_adam")
      {
        opt_settings->add_arr("init_bins_count", init_bins_count);
        opt_settings->add_arr("init_bins_positions", init_bins_positions);
        opt = new opt::GridSearchAdam();
      }
      else
      {
        logerr("Unknown optimizer algorithm %s", search_algorithm.c_str());
        return 1.0;
      }
      
      for (float sz = 0.01; sz < 0.2; sz += 0.01)
      {
        for (int i=0; i<100; i++)
        {

        }
      }
      /*
      for (int id = 0; id < params_max.size(); id++)
      {
        std::vector<float> ref_params = {3, 3.3, 3.33, 3.36, 3.4, 3.43, 3.45, 3.47, 3.5, 0.781, 1, 0.05, 0.7, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.4,
                                        0.1, 0, 0.004, 0.3, 0.0, -0.250};
        
        debug("data_param_%d = [", id);
        for (int i=0;i<100;i++)
        {
          ref_params[id] = (i/100.0)*(params_max[id] - params_min[id]) + params_min[id];
          auto res = F_silhouette(ref_params);
          debug("%.4f", res.first);
          if (i + 1 < 100)
            debug(", ");
        }
        debug("]\n");
      }
      return 1;
      */
      std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
      opt->optimize(F_silhouette, params_min, params_max, *opt_settings, F_depth_reg);
      std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();

      double opt_time_ms = 1e-3 * std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
      opt_result.best_params = opt->get_best_result(&(opt_result.best_err));
      opt_result.total_iters = iters;
      
      debug("Optimization stat\n");
      debug("%.1f s total \n", 1e-3 * opt_time_ms);
      debug("%.1f s target function calc (%.1f ms/iter)\n", 1e-3 * total_time_ms, total_time_ms/iters);
    }

    std::vector<float> best_model = func.get_transformed(get_gen_params(opt_result.best_params), get_camera_params(opt_result.best_params),
                                                         dgen::ModelQuality(false, 3));
    mi.init_scene_and_settings(MitsubaInterface::RenderSettings(ref_image_size, ref_image_size, 256, MitsubaInterface::LLVM, MitsubaInterface::MONOCHROME));
    mi.render_model_to_file(best_model, saved_result_path, dgen::ModelLayout(), camera);
    if (saved_initial_path != "")
    {
      std::vector<float> initial_model = func.get_transformed(get_gen_params(init_params), get_camera_params(init_params));
      mi.render_model_to_file(initial_model, saved_initial_path, dgen::ModelLayout(), camera);
    }
    if (texture_extraction)
    {
      //Texture estimation only by 1 camera by now

      ModelTex mt;
      Model *m = new Model();
      visualizer::simple_mesh_to_model_332(best_model, m);
      m->update();
      Texture res_tex = mt.getTexbyUV(reference_mask[0], *m, reference_tex[0], 3, camera);
      engine::textureManager->save_png(res_tex, "reconstructed_tex");

      mi.init_scene_and_settings(MitsubaInterface::RenderSettings(512, 512, 256, MitsubaInterface::LLVM, MitsubaInterface::TEXTURED_CONST, "../../saves/reconstructed_tex.png"));
      mi.render_model_to_file(best_model, saved_textured_path, dgen::ModelLayout(), camera);
    }
    debug("Model optimization finished. %d iterations total. Best result saved to \"%s\"\n", opt_result.total_iters, saved_result_path.c_str());
    debug("Best error: %f\n", opt_result.best_err);
    debug("Best params: [");
    for (int j = 0; j < opt_result.best_params.size(); j++)
    {
      debug("%.3f, ", opt_result.best_params[j]);
    }
    debug("]\n");
    mi.finish();

    return opt_result.best_err;
  }
}