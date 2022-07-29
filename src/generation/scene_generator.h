#pragma once
#include "grove_generator.h"
#include "core/scene.h"
#include "biome.h"
#include <mutex>
#include <atomic>

struct Cell;
class SceneGenerator
{
public:
    struct Patch
    {
      Sphere2D border;
      float density;
      std::vector<std::pair<int, float>> types;
      int patch_type_id;
      int biome_id;
      Patch() {};
      Patch(glm::vec2 pos, Biome::PatchDesc &patchDesc, int biome_id, int patch_type_id);
    };
    struct SceneGenerationContext
    {
      SceneGenerationContext(): objects_bvh(false) {};
        Scene *scene = nullptr;//only a pointer, should not be deleted
        Block settings;
        BVH objects_bvh;
        BiomeMap biome_map;
        GroveMask global_mask;
        std::vector<Cell> cells;
        std::vector<Patch> trees_patches;
        std::vector<Patch> grass_patches;
        int cells_x, cells_y;
        float hmap_pixel_size, biome_map_pixel_size;
        glm::vec2 heightmap_size, grass_field_size, full_size, cell_size, center, start_pos;
        glm::vec3 center3;

        int base_biome_id = 0;
        bool inited = false;
    };
    
    SceneGenerator(SceneGenerationContext &_ctx);
    void create_scene_auto();
    void init_scene(Block &settings);
    void create_heightmap_simple_auto();
    uint64_t add_object_blk(Block &b);
    bool remove_object(uint64_t id);
    void set_default_biome(std::string biome_name);
    void set_biome_round(glm::vec2 pos, float r, std::string biome_name);
    void plant_tree(glm::vec2 pos, int type);
private:
    void generate_grove();
    void prepare_patches(int type);
    void create_global_grove_mask(GroveMask &mask);
    SceneGenerationContext &ctx;
};

struct Cell
{
  enum CellStatus
  {
    EMPTY,//nothing to do with it or task is not set
    WAITING,//task is set but not performed. voxels_small == nullptr, planar_occlusion == nullptr
    BORDER,//plants are generated. voxels_small!= nullptr, planar_occlusion == nullptr
    FINISHED_PLANTS, //plants are generated. voxels_small == nullptr, planar_occlusion != nullptr
    FINISHED_ALL //plants and grass are generated. voxels_small == nullptr, planar_occlusion == nullptr
  };
  std::mutex cell_lock;
  std::vector<GrovePrototype> prototypes;

  LightVoxelsCube *voxels_small = nullptr;//used for plants gen is dependant cells
  Field_2d *planar_occlusion = nullptr;//used for grass gen in this cell
  int id = -1;
  AABB2D bbox;
  std::atomic<CellStatus> status;
  std::vector<int> depends;
  std::vector<int> grass_patches;//set and used by grass generator
  std::vector<int> trees_patches;
  std::vector<std::pair<int,int>> biome_stat;//<biome_id, number_of_pixels> 
  AABB influence_bbox;
  explicit Cell(CellStatus _status = CellStatus::EMPTY) {status = _status;}
  Cell(const Cell &cell)
  {
    id = cell.id;
    prototypes = cell.prototypes;
    bbox = cell.bbox;
    status.store(cell.status);
    influence_bbox = cell.influence_bbox;
    depends = cell.depends;

    if (voxels_small || planar_occlusion || !grass_patches.empty())
    {
      logerr("only copying of empty cells is allowed");
    }
  };
};