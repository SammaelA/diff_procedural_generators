#pragma once
#include "../tree.h"
#include "../visualizer.h"
#include "../volumetric_occlusion.h"
#include <vector>
#include <map>
#include "../tinyEngine/save_utils/blk.h"
#include "../hash.h"
#include "default_clustering_params.h"

struct BaseBranchClusteringData
{
    glm::mat4 transform = glm::mat4(1.0f);
    float r_transform = 1;
    bool can_be_center = true;
};
struct BranchClusteringData
{
    int base_cluster_id;
    int id;  
    glm::mat4 transform;
    float r_transform;
    bool can_be_center;
    virtual void clear() {};
    void set_base(BaseBranchClusteringData &base)
    {
        transform = base.transform;
        r_transform = base.r_transform;
        can_be_center = base.can_be_center;
    }
};
struct IntermediateClusteringData
{
    int elements_count;
    std::vector<BranchClusteringData *> branches;
    virtual void clear() {};
};
struct ClusteringContext
{
    LightVoxelsCube *light = nullptr;
    std::vector<TreeTypeData> *types = nullptr;
    ImpostorsData *self_impostors_data = nullptr;
    TextureAtlasRawData *self_impostors_raw_atlas = nullptr;
    virtual void clear() {    
        if (self_impostors_data)
            delete self_impostors_data;
    };
    ~ClusteringContext()
    {
        clear();
    }
};
class ClusteringHelper
{
public:
    virtual BranchClusteringData *convert_branch(Block &settings, Branch *base, ClusteringContext *ctx, BaseBranchClusteringData &data) = 0;
    virtual void clear_branch_data(BranchClusteringData *base, ClusteringContext *ctx) = 0;
    virtual IntermediateClusteringData *prepare_intermediate_data(Block &settings, std::vector<BranchClusteringData *> branches,
                                                                  ClusteringContext *ctx) = 0;
};
class ClusteringBase
{
public:
    struct Transform
    {
        float rot = 0;
    };
    struct ClusterStruct
    {
        int center;
        std::vector<std::pair<int,Transform>> members;
    };
    virtual bool clusterize(Block &settings, IntermediateClusteringData *data, std::vector<ClusterStruct> &result) = 0;
};
struct AdditionalClusterDataArrays
{
    std::vector<float> rotations;
    std::vector<Branch *> originals;
    std::vector<int> ids;
    std::vector<BranchClusteringData *> clustering_data;
};
struct ClusterData
{
    long id = -1;
    int base_pos = 0;
    Branch *base = nullptr;
    InstanceDataArrays IDA;
    AdditionalClusterDataArrays ACDA;
    bool is_valid();
    ClusterData();
};
struct FullClusteringData
{
    IntermediateClusteringData *id;
    std::vector<ClusteringBase::ClusterStruct> clusters;
    ClusteringContext *ctx;
    std::vector<ClusterData> *base_clusters;
    std::vector<ClusterData> *result_clusters;
    Block *settings;
    std::map<int,int> pos_in_table_by_id;
    std::map<int,int> pos_in_table_by_branch_id;
};
DEFINE_ENUM_WITH_STRING_CONVERSIONS(ClusteringStrtegy,(Merge)(Recreate))

class Clusterizer2
{
public:
    void prepare(Block &settings);
    void get_base_clusters(Block &settings, Tree *t, int count, int layer, std::vector<ClusterData> &base_clusters,
                           ClusteringContext *ctx);
    void clusterize(Block &settings, std::vector<ClusterData> &base_clusters, std::vector<ClusterData> &clusters,
                    ClusteringContext *ctx, bool need_save_full_data = false);
    FullClusteringData *get_full_data() { return fcd; }
    ~Clusterizer2();
    
private:
    struct ClusterizationTmpData
    {
        std::map<int,int> pos_in_table_by_id;
        std::map<int,int> pos_in_table_by_branch_id;
    };
    void get_base_clusters(Block &settings, Tree &t, int layer, std::vector<ClusterData> &base_clusters,
                           ClusteringContext *ctx);
    void prepare_branches(Block &settings, std::vector<ClusterData> &base_clusters, std::vector<BranchClusteringData *> &branches);
    void prepare_result(Block &settings, std::vector<ClusterData> &base_clusters, std::vector<ClusterData> &clusters,
                        std::vector<BranchClusteringData *> &branches, ClusteringContext *ctx, 
                        std::vector<ClusteringBase::ClusterStruct> &result);
    BranchClusteringData *convert_branch(Block &settings, Branch *base, ClusteringContext *ctx);
    ClusteringHelper *clusteringHelper = nullptr;
    ClusteringBase *clusteringBase = nullptr;
    ClusterizationTmpData tmpData;
    FullClusteringData *fcd = nullptr;
};