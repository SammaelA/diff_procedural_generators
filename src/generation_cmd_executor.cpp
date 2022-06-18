#include "cmd_executors.h"
#include "common_utils/utility.h"
#include "generation/scene_generator_helper.h"
#include "generation/metainfo_manager.h"
#include "generation/scene_generator_plants.h"

namespace scene_gen
{
  void align(float &from, float by)
  {
    from = ceil(from / by) * by;
  }
  void align(glm::vec2 &from, glm::vec2 by)
  {
    from.x = ceil(from.x / by.x) * by.x;
    from.y = ceil(from.y / by.y) * by.y;
  }
  void align(glm::vec2 &from, float by)
  {
    from.x = ceil(from.x / by) * by;
    from.y = ceil(from.y / by) * by;
  }

  void init_scene(Block &_settings, SceneGenerator::SceneGenerationContext &ctx)
  {
    metainfoManager.reload_all();
    ctx.settings = _settings;

    ctx.heightmap_size = ctx.settings.get_vec2("heightmap_size", glm::vec2(1000, 1000));
    ctx.hmap_pixel_size = ctx.settings.get_double("heightmap_cell_size", 10.0f);
    ctx.biome_map_pixel_size = ctx.settings.get_double("biome_map_pixel_size", 1.0f);
    ctx.full_size = ctx.settings.get_vec2("scene_size", glm::vec2(100, 100));
    ctx.grass_field_size = ctx.settings.get_vec2("grass_field_size", glm::vec2(1750, 1750));
    ctx.grass_field_size = max(min(ctx.grass_field_size, ctx.heightmap_size), ctx.full_size);
    ctx.center = ctx.settings.get_vec2("scene_center", glm::vec2(0, 0));
    ctx.cell_size = ctx.settings.get_vec2("cell_size", glm::vec2(150, 150));
    ctx.center3 = glm::vec3(ctx.center.x, 0, ctx.center.y);

    align(ctx.hmap_pixel_size, ctx.biome_map_pixel_size);
    align(ctx.heightmap_size, ctx.biome_map_pixel_size);
    align(ctx.grass_field_size, ctx.biome_map_pixel_size);
    align(ctx.full_size, ctx.biome_map_pixel_size);
    align(ctx.cell_size, ctx.biome_map_pixel_size);
    align(ctx.grass_field_size, ctx.cell_size);

    ctx.cells_x = ceil(ctx.grass_field_size.x / ctx.cell_size.x);
    ctx.cells_y = ceil(ctx.grass_field_size.y / ctx.cell_size.y);
    ctx.start_pos = ctx.center - 0.5f * ctx.cell_size * glm::vec2(ctx.cells_x, ctx.cells_y);

    ctx.cells = std::vector<Cell>(ctx.cells_x * ctx.cells_y, Cell(Cell::CellStatus::EMPTY));
    for (int i = 0; i < ctx.cells_x; i++)
    {
      for (int j = 0; j < ctx.cells_y; j++)
      {
        int id = i * ctx.cells_y + j;
        ctx.cells[id].id = id;
        ctx.cells[id].bbox = AABB2D(ctx.start_pos + ctx.cell_size * glm::vec2(i, j), ctx.start_pos + ctx.cell_size * glm::vec2(i + 1, j + 1));
      }
    }

    ctx.biome_map.create(AABB2D(ctx.center - 0.5f * ctx.grass_field_size, ctx.center + 0.5f * ctx.grass_field_size), 1);

    debug("Initialized scene\n");
    debug("Heightmap size %.1fx%.1f\n", ctx.heightmap_size.x, ctx.heightmap_size.y);
    debug("Grass field size %.1fx%.1f\n", ctx.grass_field_size.x, ctx.grass_field_size.y);
    debug("Vegetation scene size %.1fx%.1f\n", ctx.full_size.x, ctx.full_size.y);
    debug("created %dx%d cells with %.1fx%.1f size each\n", ctx.cells_x, ctx.cells_y, ctx.cell_size.x, ctx.cell_size.y);
    debug("created biome map %dx%d pixels\n", ctx.biome_map.pixels_w(), ctx.biome_map.pixels_h());

    ctx.scene->grove.center = ctx.center3;
    ctx.scene->grove.ggd_name = "blank";
  }

  void create_heightmap(Block &settings, SceneGenerator::SceneGenerationContext &ctx)
  {
    std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
    ctx.scene->heightmap = new Heightmap(ctx.center3, ctx.heightmap_size, ctx.hmap_pixel_size);
    bool load_from_image = settings.get_bool("load_from_image", true);
    float base = settings.get_double("base_height", 0);
    float mn = settings.get_double("min_height", 0);
    float mx = settings.get_double("max_height", 100);
    if (load_from_image)
    {
      std::string hmap_tex_name = settings.get_string("tex_name", "heightmap1.jpg");
      ctx.scene->heightmap->load_from_image(base, mn, mx, hmap_tex_name);
    }
    else
    {
      ctx.scene->heightmap->random_generate(base, mn, mx);
    }
    std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
    float ms = 1e-4 * std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    debug("created heightmap. Took %.2f ms\n", ms);
  }

  uint64_t add_object_blk(Block &b, SceneGenerator::SceneGenerationContext &ctx)
  {
    std::string name = b.get_string("name", "debug_box");
    glm::mat4 transform = b.get_mat4("transform");

    bool place_on_terrain = b.get_bool("on_terrain", false);

    if (place_on_terrain)
    {
      glm::vec4 from = transform * glm::vec4(0, 0, 0, 1);
      glm::vec3 pos = glm::vec3(from);
      float min_h, float_max_h;
      pos.y = ctx.scene->heightmap->get_height(pos);
      transform[3][1] += (pos.y - from.y);
    }
    bool new_model = true;
    unsigned model_num = 0;
    unsigned pos = 0;
    for (auto &im : ctx.scene->instanced_models)
    {
      if (im.name == name)
      {
        im.instances.push_back(transform);
        new_model = false;
        pos = im.instances.size() - 1;
        break;
      }
      model_num++;
    }

    if (new_model)
    {
      ModelLoader loader;
      ctx.scene->instanced_models.emplace_back();
      ctx.scene->instanced_models.back().model = loader.create_model_from_block(b, ctx.scene->instanced_models.back().tex);
      ctx.scene->instanced_models.back().model->update();
      ctx.scene->instanced_models.back().instances.push_back(transform);
      ctx.scene->instanced_models.back().name = name;
      pos = 0;
    }

    // logerr("model num %d %d", model_num, ctx.scene->instanced_models.size());
    auto &im = ctx.scene->instanced_models[model_num];
    std::vector<AABB> boxes;
    SceneGenHelper::get_AABB_list_from_instance(im.model, transform, boxes, 4 * ctx.biome_map_pixel_size, 1.1);
    uint64_t id = SceneGenHelper::pack_id(0, (int)Scene::SIMPLE_OBJECT, model_num, pos);
    ctx.objects_bvh.add_bboxes(boxes, id);
    return id;
  }

  void plant_tree(glm::vec2 pos, int type, SceneGenerator::SceneGenerationContext &ctx)
  {
    if (type < 0 || type >= metainfoManager.get_all_tree_types().size())
    {
      logerr("Failed to place tree manually. Invalid type = %d", type);
      return;
    }
    glm::ivec2 c_ij = (pos - ctx.start_pos) / ctx.cell_size;
    if (c_ij.x >= 0 && c_ij.x < ctx.cells_x && c_ij.y >= 0 && c_ij.y < ctx.cells_y)
    {
      glm::vec3 p = glm::vec3(pos.x, 0, pos.y);
      p.y = ctx.scene->heightmap->get_bilinear(p);
      Cell &c = ctx.cells[c_ij.x * ctx.cells_y + c_ij.y];
      if (c.prototypes.empty())
      {
        c.prototypes.emplace_back();
        c.prototypes.back().pos = 0.5f * (c.bbox.max_pos + c.bbox.min_pos);
        c.prototypes.back().size = 0.5f * (c.bbox.max_pos - c.bbox.min_pos);
        c.prototypes.back().trees_count = 0;
      }
      bool have_type = false;
      for (auto &tp : c.prototypes.back().possible_types)
      {
        if (tp.first == type)
        {
          have_type = true;
          break;
        }
      }
      if (!have_type)
      {
        c.prototypes.back().possible_types.push_back(std::pair<int, float>(type, 0));
      }
      c.prototypes.back().preplanted_trees.push_back(std::pair<int, glm::vec3>(type, p));
      c.prototypes.back().trees_count++;
    }
  }
}
void GenerationCmdExecutor::execute(int max_cmd_count)
{
  int cmd_left = max_cmd_count;
  while (!genCmdBuffer.empty() && cmd_left != 0)
  {
    auto &cmd = genCmdBuffer.front();
    switch (cmd.type)
    {
    case GC_GEN_HMAP:
      if (genCtx.scene->heightmap)
        delete genCtx.scene->heightmap;
      scene_gen::create_heightmap(cmd.args, genCtx);
      break;
    case GC_ADD_OBJECT:
      if (genCtx.scene->heightmap)
      {
        uint64_t id = scene_gen::add_object_blk(cmd.args, genCtx);
        debug("added object id = %lu \n", id);
      }
      break;
    case GC_CLEAR_SCENE:
      if (genCtx.inited)
      {
        if (genCtx.scene)
          delete genCtx.scene;
        genCtx = SceneGenerator::SceneGenerationContext();
        genCtx.scene = new Scene();
        genCtx.inited = false;
      }
      break;
    case GC_INIT_SCENE:
      if (!genCtx.inited)
      {
        scene_gen::init_scene(cmd.args, genCtx);
        genCtx.inited = true;
      }
      break;
    case GC_REMOVE_BY_ID:
      for (int i = 0; i < cmd.args.size(); i++)
      {
        glm::ivec4 unpacked_id = cmd.args.get_ivec4(i, glm::ivec4(-1, 0, 0, 0));
        logerr("remove by mask %d %d %d %d %d", unpacked_id.x, unpacked_id.y, unpacked_id.z, unpacked_id.w, cmd.args.size());
        if (unpacked_id.x < 0)
          continue;
        else if (unpacked_id.y < 0)
        {
          // remove everything
          genCtx.scene->instanced_models.clear();
          genCtx.objects_bvh.clear();
        }
        else if (unpacked_id.z < 0)
        {
          // remove everything in this category
          if (unpacked_id.z == Scene::ObjCategories::SIMPLE_OBJECT)
          {
            genCtx.scene->instanced_models.clear();
            genCtx.objects_bvh.clear();
          }
        }
        else if (unpacked_id.w < 0)
        {
          // TODO
        }
        else
        {
          uint64_t id = SceneGenHelper::pack_id(unpacked_id.x, unpacked_id.y, unpacked_id.z, unpacked_id.w);
          logerr("remove by mask %d %d %d %d %lu", unpacked_id.x, unpacked_id.y, unpacked_id.z, unpacked_id.w, id);
          genCtx.objects_bvh.remove_bboxes(id);
          if (unpacked_id.y == Scene::ObjCategories::SIMPLE_OBJECT)
          {
            int m_num = unpacked_id.z;
            int inst_num = unpacked_id.w;
            if (m_num >= 0 && m_num < genCtx.scene->instanced_models.size())
            {
              auto &im = genCtx.scene->instanced_models[m_num];
              if (inst_num >= 0 && inst_num < im.instances.size())
              {
                if (im.instances.size() > 1)
                  im.instances.erase(im.instances.begin() + inst_num);
                else
                  genCtx.scene->instanced_models.erase(genCtx.scene->instanced_models.begin() + m_num);
              }
            }
          }
        }
      }
      break;
    case GC_PLANT_TREE:
      {
      std::string type_name = cmd.args.get_string("type_name");
      int type_id = metainfoManager.get_tree_type_id_by_name(type_name);
      if (type_id >= 0)
      {
        scene_gen::plant_tree(cmd.args.get_vec2("pos"), type_id, genCtx);
      }
      else
      {
        logerr("trying to create tree with unknown type %s", type_name.c_str());
      }
      }
      break;
    case GC_GEN_TREES_CELL:
    {
      std::vector<int> ids = {};
      scene_gen::generate_plants_cells(genCtx, ids);
    }
      break;
    default:
      logerr("GenerationCmdExecutor: command %d is not implemented yet", (int)(cmd.type));
      break;
    }
    genCmdBuffer.pop();
    cmd_left--;
  }
}