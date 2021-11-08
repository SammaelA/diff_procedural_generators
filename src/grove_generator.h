#pragma once
#include "generators/abstract_generator.h"
#include "grove_generation_utils.h"

struct GrovePrototype
{
    int trees_count;
    glm::vec2 pos;//pos of center
    glm::vec2 size;//size from center, BBox is [pos - (size.x,0,size.z)] - [pos + size]
    std::vector<std::pair<int,float>> possible_types;//type id and chance to create tree of that type
};
class GroveGenerator
{
public:
    void prepare_patch(GrovePrototype &prototype, 
                       std::vector<TreeTypeData> &treeTypesCatalogue,
                       Heightmap &hmap,
                       GroveMask &mask,
                       LightVoxelsCube &voxels,
                       Tree *trees);
private:
    static AbstractTreeGenerator *get_generator(std::string &name);
};