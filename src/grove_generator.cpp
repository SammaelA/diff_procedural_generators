#include "grove_generator.h"
#include "generators/GE_generator.h"
#include "generators/python_tree_gen.h"
#include "generators/weber_penn_parameters.h"
#include "generators/simple_generator.h"
#include "generators/proctree.h"
#include "generators/generated_tree.h"
#include "grove_generation_utils.h"
#include "planter.h"
#include <map>

AbstractTreeGenerator *GroveGenerator::get_generator(std::string &generator_name)
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

void GroveGenerator::prepare_patch(GrovePrototype &prototype, 
                                   std::vector<TreeTypeData> &treeTypesCatalogue,
                                   Heightmap &hmap,
                                   GroveMask &mask,
                                   LightVoxelsCube &voxels,
                                   Tree *trees)
{
    std::map<std::string,AbstractTreeGenerator *> generators;
    std::map<std::string,int> t_counts;
    float w = 0;
    for (auto &p : prototype.possible_types)
    {
        std::string g_name = treeTypesCatalogue[p.first].generator_name;
        logerr("using generator %s to create grove",g_name.c_str());
        generators.emplace(g_name,get_generator(g_name));
        t_counts.emplace(g_name,0);
        w += p.second;
    }

    int trees_planted = 0;
    bool generating = true;
    //Seeder seeder = Seeder(glm::vec3(prototype.pos.x,0,prototype.pos.y), 
    //                       glm::vec3(prototype.size.x,0, prototype.size.y), 3.0f, &hmap);
    Planter planter = Planter(&voxels, &hmap, &mask,
                              glm::vec3(prototype.pos.x,0,prototype.pos.y), prototype.size,
                              1,prototype.trees_count,7.5);
    while (trees_planted < prototype.trees_count || generating)
    {
        if (trees_planted < prototype.trees_count)
        {
            std::vector<glm::vec3> seeds = planter.get_saplings();
            //seeder.choose_places_for_seeds(1,seeds);
            if (!seeds.empty())
            {
                for (auto &seed : seeds)
                {
                    float f = urand(0,w);
                    int type = 0;
                    for (auto &p : prototype.possible_types)
                    {
                        if (f < p.second)
                        {
                            type = p.first;
                            break;
                        }
                        else
                            f -= p.second;
                    }

                    AbstractTreeGenerator *gen = generators.at(treeTypesCatalogue[type].generator_name);
                    //glm::vec3 pos = glm::vec3(seeds[0].pos.x,0,seeds[0].pos.y);
                    //pos.y = hmap.get_height(pos);
                    logerr("olant tree %d",prototype.trees_count);
                    gen->plant_tree(seed, &(treeTypesCatalogue[type]));
                    t_counts.at(treeTypesCatalogue[type].generator_name)++;
                }
            }
            trees_planted++;
        }

        generating = false;
        for (auto &p : generators)
        {
            generating = generating || p.second->iterate(voxels);
        }
    }

    int t_count = 0;
    for (auto &p : generators)
    {
        int t_cnt = t_counts.at(p.first);
        p.second->finalize_generation(trees + t_count,voxels);
        t_count += t_cnt;
    }
}