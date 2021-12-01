#include "scene_generator.h"
#include "graphics_utils/volumetric_occlusion.h"
#include "grove_packer.h"
#include "core/tree.h"
#include "save_utils/blk.h"
#include "tree_generators/GE_generator.h"
#include "tree_generators/python_tree_gen.h"
#include "tree_generators/weber_penn_parameters.h"
#include "tree_generators/simple_generator.h"
#include "tree_generators/proctree.h"
#include "tree_generators/generated_tree.h"
#include "grass_generator.h"
#include "graphics_utils/debug_transfer.h"
#include <algorithm>

LightVoxelsCube *SceneGenerator::create_grove_voxels(GrovePrototype &prototype, std::vector<TreeTypeData> &types,
                                                     AABB &influence_box)
{
  float min_scale_factor = 1000;
  for (auto &p : prototype.possible_types)
  {
    auto &type = types[p.first];
    min_scale_factor = MIN(min_scale_factor,type.params->get_scale_factor());
  }
  float vox_scale = 0.5/0.8;
  glm::vec3 voxel_sz = vox_scale*(influence_box.max_pos - influence_box.min_pos);
  glm::vec3 voxel_center = influence_box.min_pos + voxel_sz;
  auto *v = new LightVoxelsCube(voxel_center, voxel_sz, vox_scale*min_scale_factor, 1.0f);
  AABB &box = influence_box;
  float Mvoxels = 1e-6*v->get_size_cnt();
  debugl(1, "created voxels array [%.1f %.1f %.1f] - [%.1f %.1f %.1f] for patch [%.1f %.1f] - [%.1f %.1f] with %.2f Mvoxels\n",
  box.min_pos.x,box.min_pos.y,
  box.min_pos.z, box.max_pos.x,box.max_pos.y,box.max_pos.z,prototype.pos.x - prototype.size.x,
  prototype.pos.y - prototype.size.y, prototype.pos.x + prototype.size.x, prototype.pos.y + prototype.size.y,
  Mvoxels);
  return v;
}

LightVoxelsCube *SceneGenerator::create_cell_small_voxels(Cell &c)
{
  float vox_scale = 4;
  glm::vec3 voxel_sz = vox_scale*(c.influence_bbox.max_pos - c.influence_bbox.min_pos);
  glm::vec3 voxel_center = c.influence_bbox.min_pos + voxel_sz;
  
  auto *voxels = new LightVoxelsCube(voxel_center, voxel_sz, vox_scale, 1.0f);
  for (auto *b : ctx.global_ggd.obstacles)
  {
    voxels->add_body(b,10000);
  }

  return voxels;
}

AABB SceneGenerator::get_influence_AABB(GrovePrototype &prototype, std::vector<TreeTypeData> &types,
                                        Heightmap &h)
{
  glm::vec3 max_tree_size = glm::vec3(0,0,0);
  for (auto &p : prototype.possible_types)
  {
    auto &type = types[p.first];
    max_tree_size = max(max_tree_size,type.params->get_tree_max_size());
  }
  
  float min_hmap = 0, max_hmap = 0;
  h.get_min_max_imprecise(prototype.pos - prototype.size, prototype.pos + prototype.size, &min_hmap, &max_hmap);
  float br = 5;
  float min_y = min_hmap - br;
  float max_y = max_hmap + max_tree_size.y;
  float y_center = (min_y + max_y)/2;
  float y_sz = (max_y - min_y)/2;
  glm::vec3 voxel_sz = glm::vec3(prototype.size.x + max_tree_size.x, y_sz, prototype.size.y + max_tree_size.z);
  glm::vec3 voxel_center = glm::vec3(prototype.pos.x, y_center, prototype.pos.y);
  return AABB(voxel_center - voxel_sz, voxel_center + voxel_sz);

}
AbstractTreeGenerator *get_gen(std::string &generator_name)
{
  AbstractTreeGenerator *gen;
  if (generator_name == "proctree")
    gen = new Proctree::ProctreeGenerator();
  else if (generator_name == "simple")
    gen = new SimpleTreeGenerator();
  else if (generator_name == "load_from_file")
    gen = new TreeLoaderBlk();
  else if (generator_name == "python_tree_gen")
    gen = new PythonTreeGen();
  else if (generator_name == "ge_gen")
    gen = new GETreeGenerator();
  else
    gen = new mygen::TreeGenerator();
  
  return gen;
}

void SceneGenerator::create_scene_auto()
{
  generate_grove();
}
void SceneGenerator::generate_grove()
{
  GrovePacker packer;

  int max_tc = ctx.settings.get_int("max_trees_per_patch", 1);
  int fixed_patches_count = ctx.settings.get_int("fixed_patches_count", 0);
  float patches_density = ctx.settings.get_double("patches_density", 1);
  glm::vec2 heightmap_size = ctx.settings.get_vec2("heightmap_size", glm::vec2(1000,1000));
  glm::vec2 full_size = ctx.settings.get_vec2("scene_size", glm::vec2(100,100));
  glm::vec2 grass_field_size = ctx.settings.get_vec2("grass_field_size", glm::vec2(500,500));
  grass_field_size = max(min(grass_field_size, heightmap_size), full_size);
  glm::vec2 center = ctx.settings.get_vec2("scene_center", glm::vec2(0,0));
  glm::vec2 cell_size = ctx.settings.get_vec2("cell_size", glm::vec2(50,50));
  glm::vec2 mask_pos = center;
  glm::vec3 center3 = glm::vec3(center.x,0,center.y);
  float hmap_cell_size = ctx.settings.get_double("heightmap_cell_size", 10.0f);

  Block *grass_blk = ctx.settings.get_block("grass");
  bool grass_needed = (grass_blk != nullptr);


  ctx.scene->heightmap = new Heightmap(center3, heightmap_size,hmap_cell_size);
  ctx.scene->heightmap->random_generate(0,0,100);
  ctx.scene->grove.center = center3;
  ctx.scene->grove.ggd_name = "blank";
  
  std::vector<TreeTypeData> types;
  Block *types_bl = ctx.settings.get_block("types");
  if (!types_bl)
  {
    logerr("error: scene generation settings should have \"types\" block");
    return;
  }  
  else
  {
    for (int i=0;i<types_bl->size();i++)
    {
      std::string type_name = types_bl->get_name(i);
      auto it = ctx.tree_types.find(type_name);
      if (it == ctx.tree_types.end())
      {
        logerr("error: using unknown tree type %s",type_name.c_str());
      }
      else
      {
        types.push_back(it->second);
      }
    }
    if (types.empty())
    {
      logerr("error: \"types\" block should contain at least one valid tree type");
      return;
    }
  }
  
  GroveGenerationData ggd;
  ggd.types = types; 


  int cells_x = ceil(grass_field_size.x/cell_size.x);
  int cells_y = ceil(grass_field_size.y/cell_size.y);
  glm::vec2 start_pos = center - 0.5f*cell_size*glm::vec2(cells_x, cells_y);
  std::vector<Cell> cells = std::vector<Cell>(cells_x*cells_y,Cell(Cell::CellStatus::EMPTY));
  
  GrassGenerator grassGenerator;
  glm::vec2 mask_size = full_size;
  if (cells_x <= 2 || cells_y <= 2)
    mask_size *= 2.0f;
  GroveMask mask = GroveMask(glm::vec3(mask_pos.x,0,mask_pos.y), mask_size, 3);
  mask.set_square(mask_size.x, mask_size.y);

  std::list<int> waiting_cells;
  std::list<int> border_cells;

  for (int i=0;i<cells_x;i++)
  {
    for (int j=0;j<cells_y;j++)
    {
      int id = i*cells_y + j;
      cells[id].id = id;
      cells[id].bbox = AABB2D(start_pos + cell_size*glm::vec2(i,j), start_pos + cell_size*glm::vec2(i+1,j+1));
    }
  }

  if (fixed_patches_count > 0)
  {
    std::vector<int> cells_n = {};
    for (int i=0;i<cells.size();i++)
    {
      glm::vec2 ct = 0.5f*(cells[i].bbox.min_pos + cells[i].bbox.max_pos);
      if (mask.get_bilinear(glm::vec3(ct.x,0,ct.y)) > 0.1)
        cells_n.push_back(i);
    }
    std::random_shuffle(cells_n.begin(), cells_n.end());
    for (int i=0;i<MIN(fixed_patches_count, cells_n.size());i++)
    {
      cells[cells_n[i]].status = Cell::CellStatus::WAITING;
    }
  }

  for (int i=0;i<cells_x;i++)
  {
    for (int j=0;j<cells_y;j++)
    {
      int id = i*cells_y + j;

      int trees_count = 0;
      if (urand() <= patches_density || (cells[id].status == Cell::CellStatus::WAITING))
        trees_count = MAX(urand(0.4,0.6)*max_tc,1);
      if (grass_needed)
        cells[id].status = Cell::CellStatus::WAITING;
      if (cells[id].status == Cell::CellStatus::WAITING)
      {
        glm::vec2 center = start_pos + cell_size*glm::vec2(i+0.5,j+0.5);
        cells[id].prototype.pos = center;
        cells[id].prototype.size = 0.5f*cell_size;
        cells[id].prototype.possible_types = {};
        for (int i=0;i<types.size();i++)
        {
          cells[id].prototype.possible_types.push_back(std::pair<int, float>(i,1.0f/types.size()));
        }
        cells[id].prototype.trees_count = trees_count;
        cells[id].influence_bbox = get_influence_AABB(cells[id].prototype, ggd.types, *(ctx.scene->heightmap));
        waiting_cells.push_back(id);
      }
    }
  }

  if (grass_needed)
  {
    grassGenerator.set_grass_types(ctx.grass_types, *grass_blk);
    grassGenerator.prepare_grass_patches(cells, cells_x, cells_y);
  }

  for (int c_id : waiting_cells)
  {
    //find dependencies
    int j0 = c_id % cells_y;
    int i0 = c_id / cells_y;
    bool search = true;
    int d = 3;
    int d_prev = 0;
    while (search)
    {
      search = false;
      auto func = [&](int i1, int j1)
      {
          int i = i0 + i1;
          int j = j0 + j1;

          int ncid = i*cells_y + j;
          if (i >= 0 && j >= 0 && i < cells_x && j < cells_y && ncid > c_id)
          {
            auto &c = cells[ncid];
            if (c.status == Cell::CellStatus::WAITING && c.influence_bbox.intersects(cells[c_id].influence_bbox))
            {
              cells[c_id].depends.push_back(ncid);
              c.depends_from.push_back(c_id);
              search = true;
            }
          }
      };
      for (int i1=-d;i1<=d;i1++)
      {
        for (int j1=-d;j1<=d;j1++)
        {
          if (i1 <= -d_prev || j1 <= -d_prev || i1 >= d_prev || j1 >= d_prev)
            func(i1,j1);
        }
      }
      d++;
      d_prev = d;
    }
    /*
    debug("depends of cell %d: ",c_id);
    for (auto &d : cells[c_id].depends)
      debug("%d ",d);
    debugnl();
    */
  }

  for (int c_id : waiting_cells)
  {
    //generation trees and grass
    auto &c = cells[c_id];
    bool short_lived_voxels = false;
    if (c.prototype.trees_count > 0)
    {
      //temp stuff
      ggd.pos.x = c.prototype.pos.x;
      ggd.pos.z = c.prototype.pos.y;
      ggd.pos.y = ctx.scene->heightmap->get_height(ggd.pos);
      ggd.size.x = c.prototype.size.x;
      ggd.size.z = c.prototype.size.y;
      ggd.trees_count = c.prototype.trees_count;

      ::Tree *trees = new ::Tree[ggd.trees_count];
      GroveGenerator grove_gen;
      GrovePrototype &prototype = c.prototype;
      LightVoxelsCube *voxels = create_grove_voxels(prototype, ggd.types, c.influence_bbox);
      
      for (auto *b : ctx.global_ggd.obstacles)
      {
        voxels->add_body(b,10000, true, 7.5, 3.5);
      }

      for (auto &dep_cid : c.depends_from)
      {
        voxels->add_voxels_cube(cells[dep_cid].voxels_small);
      }
      voxels->add_heightmap(*(ctx.scene->heightmap));
      grove_gen.prepare_patch(prototype, ggd.types, *(ctx.scene->heightmap), mask, *voxels, trees);
      debugl(1, "creating patch with %d trees\n", ggd.trees_count);
      packer.add_trees_to_grove(ggd, ctx.scene->grove, trees, ctx.scene->heightmap);
    
      if (!c.depends.empty())
      {
        c.status = Cell::CellStatus::BORDER;
        c.voxels_small = new LightVoxelsCube(voxels,glm::ivec3(0,0,0), voxels->get_vox_sizes(), 4,
                                            glm::vec2(0,1e8));
        border_cells.push_back(c_id);
      }
      else
      {
        c.status = Cell::CellStatus::FINISHED_PLANTS;
      }
      delete voxels;
      delete[] trees;
    } 
    else 
    {
      if (!c.depends.empty())
      {
        c.status = Cell::CellStatus::BORDER;
        c.voxels_small = create_cell_small_voxels(c);
        border_cells.push_back(c_id);
      }
      else
      {
        c.status = Cell::CellStatus::FINISHED_PLANTS;
      }
    }

    //TODO: can be done not every iteration
    auto it = border_cells.begin();
    while (it != border_cells.end())
    {
      bool have_deps = false;
      for (int &dep : cells[*it].depends)
      {
        if (cells[dep].status == Cell::CellStatus::WAITING)
        {
          have_deps = true;
          break;
        }
      }
      if (!have_deps)
      {
        //logerr("removed dependency %d",*it);
        if (grass_needed && cells[*it].voxels_small)
        {
          glm::vec3 occ_center = cells[*it].voxels_small->get_center();
          AABB bbox = cells[*it].voxels_small->get_bbox();
          glm::vec2 bord = glm::vec2(bbox.max_pos.x - bbox.min_pos.x, bbox.max_pos.z - bbox.min_pos.z);
          float vox_size = cells[*it].voxels_small->get_voxel_size();
          cells[*it].planar_occlusion = new Field_2d(occ_center, 0.5f*bord, vox_size);
          std::function<float(glm::vec2 &)> func = [&](glm::vec2 &p) ->float
          {
            return cells[*it].voxels_small->get_occlusion_projection(glm::vec3(p.x,0,p.y));
          };
          cells[*it].planar_occlusion->fill_func(func);
        }

        cells[*it].status = Cell::CellStatus::FINISHED_PLANTS;

        if (debugTransferSettings.save_small_voxels_count > debugTransferData.debug_voxels.size())
          debugTransferData.debug_voxels.push_back(cells[*it].voxels_small);
        else
          delete cells[*it].voxels_small;
        
        cells[*it].voxels_small = nullptr;
        it = border_cells.erase(it);
      }
      else
      {
        it++;
      }   
    }
  }
  
  if (grass_needed)
  {
    for (auto &c : cells)
    {
      if (c.status == Cell::CellStatus::FINISHED_PLANTS && !c.grass_patches.empty())
        grassGenerator.generate_grass_in_cell(c, c.planar_occlusion);
    }
    grassGenerator.pack_all_grass(ctx.scene->grass, *(ctx.scene->heightmap));
  }

  for (auto &c : cells)
  {
    if (c.planar_occlusion)
      delete c.planar_occlusion;
    if (c.voxels_small)
    {
      logerr("missed dependencies. Cell %d", c.id);
      delete c.voxels_small;
    }
  }
}
