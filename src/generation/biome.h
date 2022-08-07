#pragma once
#include <vector>
#include "save_utils/blk.h"
#include "common_utils/utility.h"
#include "common_utils/bbox.h"
#include "tinyEngine/texture.h"

class GroveMask;
struct Biome
{
    struct PatchDesc
    {
        float size = 25;
        float size_std_dev = 10;
        float density = 1;
        float density_std_dev = 0.5;
        float coverage_part = 0.0;
        float push = 1;

        std::vector<std::pair<int, float>> types;
    };
    struct VegClass
    {
        float main_density = -1;
        std::vector<std::pair<int, float>> main_types;
        std::vector<PatchDesc> patch_descs;
    };
    
    int id = -1;
    VegClass trees;
    VegClass plants;
    VegClass grass;

    void load_from_blk(Block &b);
};

class BiomeMap
{
public:
    typedef unsigned char biome_type_t; 

    BiomeMap();
    ~BiomeMap();
    void create(AABB2D bbox, float pixel_size);
    int pixels_w() const {return w;}
    int pixels_h() const {return h;}
    AABB2D borders() const {return bbox;}
    int get_default_biome() const {return default_biome_id; }
    int get(int w, int h) const;
    int get(glm::vec2 pos) const;
    int get(glm::vec3 pos) const;
    
    void set_rect(AABB2D box, int id);
    void set_round(glm::vec2 pos, float inner_r, float outer_r, int id);
    void get_stat(std::vector<std::pair<int,int>> &stat, AABB2D bbox) const;
    void set_mask(GroveMask &mask, int biome_id);
    void save_as_image(std::string name = "biome_map_debug") const;
    void set_default_biome(int id);
    Texture save_as_texture_RGBA8() const;
private:

   biome_type_t *data = nullptr;
   int w,h;
   AABB2D bbox;
   float pixel_size;
   int default_biome_id = -1;
};