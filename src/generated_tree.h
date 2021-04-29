#pragma once
#include <vector>
#include <list>
#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include <unordered_set>
#include "volumetric_occlusion.h"
#include "tinyEngine/utility/model.h"
#include "billboard_cloud.h"
#include "tree.h"
#include "grove.h"
#include "grove_generation_utils.h"
class DebugVisualizer;
class Heightmap;

#define MAX_TREES 1500
namespace mygen
{
    struct Segment;
    struct Joint;
    struct Leaf;
    struct Branch;
    struct Tree;

    struct Segment
{
    glm::vec3 begin;
    glm::vec3 end;
    float rel_r_begin;
    float rel_r_end;
    std::vector<float> mults;
};
struct Joint
{
    enum JointType
    {
        NONE,
        END,
        MIDDLE,
        FORK,
        LEAF
    };
    JointType type;
    Leaf *leaf = nullptr;
    glm::vec3 pos;
    std::list<Branch *> childBranches;
    short max_branching = 0;
    short iters_to_next_branch_try = 0;
    short tries_from_last_grown_branch = 0;
    float light;
};
struct Branch
{
    int id = 0;
    ushort type_id = 0;
    short level;
    short base_seg_n;
    short max_seg_count;
    short iters_to_next_segment_try = 0;
    short tries_from_last_grown_segment = 0;
    std::list<Segment> segments;
    std::list<Joint> joints;
    float light;
    float size;
    float base_r;
    bool dead = false;
    glm::vec4 plane_coef;//plane ax+by+cz+d = 0 len(a,b,c) = 1
    glm::vec3 center_par;
    glm::vec3 center_self;
    
};
struct Leaf
{
    glm::vec3 pos;
    std::vector<glm::vec3> edges;
    bool dead = false;
};
struct LeafHeap
{
    std::list<Leaf> leaves;
    Leaf *new_leaf()
    {
        leaves.push_back(Leaf());
        return &leaves.back();
    }
    void clear_removed();
    ~LeafHeap()
    {
        leaves.clear();
    }
    LeafHeap(){};

};
struct BranchHeap
{
    std::list<Branch> branches;
    Branch *new_branch()
    {
        branches.push_back(Branch());
        return &branches.back();
    }
    void clear_removed();
};

struct Tree
{
    std::vector<BranchHeap *> branchHeaps;
    LeafHeap *leaves = nullptr;
    glm::vec3 pos;
    ParameterSetWrapper params;
    Branch *root= nullptr;
    int iter = 0;
    uint id = 0;
    uint type_id;
    LightVoxelsCube *voxels= nullptr;
    Texture wood;
    Texture leaf;
    std::vector<BillboardCloudRaw *> billboardClouds;
    std::vector<Model *> models;
    void render(Shader &defaultShader, int cloudnum, glm::mat4 projcam);
    Tree();
    ~Tree();
};
class TreeGenerator
{
public:
    TreeGenerator(Tree &t) : curTree(t), curParams(TreeStructureParameters(),1){};
    void create_grove(GroveGenerationData ggd, ::Tree *trees_external, Heightmap *h);

    bool tree_to_model(::Tree &t, bool leaves, DebugVisualizer &debug);
    Tree &curTree;
    Branch *root;
    Branch *test;
    ParameterSetWrapper curParams;
    LightVoxelsCube *voxels;
    Heightmap *heightmap;
    GroveGenerationData curGgd;
    Seeder *seeder;

    void grow_branch(Branch *b, float feed);
    void new_joint(Branch *b, Joint &j);
    void try_new_branch(Branch *base, Joint &j, Segment &s, bool from_end);
    void new_branch(Branch *base, Joint &j, Segment &s, glm::vec3 &M, bool from_end);
    void try_new_segment(Branch *base);
    void new_segment(Branch *base, glm::vec3 &M);
    void new_segment2(Branch *base, glm::vec3 &dir, glm::vec3 &pos);
    void set_seg_r(Branch *base, Segment &s);
    float calc_light(Joint &j);
    float calc_light(Branch *b);
    float calc_size(Joint &j);
    float calc_size(Branch *b);
    void distribute_feed(Branch *b);
    void remove_branch(Branch *b);
    void recalculate_thickness(Branch *b);
    LightVoxelsCube *create_light_voxels_cube(ParameterSetWrapper params, glm::vec3 pos);
    glm::vec3 rand_dir();
    bool is_branch_productive(Branch *b);
    glm::vec3 get_optimal_branch_growth_direction(float &quality, Branch *base, Joint &j, Segment &s, bool from_end);
    glm::vec3 get_optimal_segment_growth_direction(float &quality, Branch *base);
    void calc_quality_field(LightVoxelsCube *&field, glm::vec3 pos, glm::vec3 sizes, glm::vec3 prev_dir, glm::vec4 plane,
                            float dir_cons, float plane_cons, float rnd, float spread, float up, float to_light);
    void post_process(GroveGenerationData ggd, Tree &t);
    void set_branches_centers(GroveGenerationData ggd, Tree &t, int up_to_level);
    void deform_root(Branch *b);
    private:
    bool random_tree(Tree &t);
    void plant_tree(Tree &t, ParameterSetWrapper params);
    void grow_tree(Tree &t);
    void create_tree(Tree &t, ParameterSetWrapper params, DebugVisualizer &debug);
    void create_grove(Tree *trees, int count);
    void convert(mygen::Tree *src, ::Tree *dst, int count);
    void convert(mygen::Tree &src, ::Tree &dst);
    void convert(mygen::Tree &src_tree, ::Tree &dst_tree, mygen::Branch &src, ::Branch &dst);
    void convert(mygen::Tree &src_tree, ::Tree &dst_tree, mygen::Joint &src, ::Joint &dst);
    void convert(mygen::Tree &src_tree, ::Tree &dst_tree, mygen::Segment &src, ::Segment &dst);
    void convert(mygen::Tree &src_tree, ::Tree &dst_tree, mygen::Leaf &src, ::Leaf &dst);
};
}