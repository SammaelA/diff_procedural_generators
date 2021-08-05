#include "tinyEngine/utility.h"
#include "branch_clusterization.h"
#include "texture_manager.h"
#include "visualizer.h"
#include "distribution.h"
#include <math.h>
#include <algorithm>
#include "body.h"
#include <chrono>
#include "tinyEngine/save_utils/saver.h"
#include "impostor.h"
#include "terrain.h"
#include "field_2d.h"
#include "grove_generation_utils.h"
#include "synthetic_trees_generator.h"
#include "grove_packer.h"


void transform_according_to_root(ClusterData &cluster)
{
    InstanceDataArrays IDA = cluster.IDA;
    ::Branch *base = cluster.base;
    if (cluster.ACDA.originals.size() != IDA.transforms.size())
    {
        logerr("Transform according to root failed: corrupted clusters data.");
        return;
    }
    for (int i = 0; i< cluster.ACDA.originals.size(); i++)
    {
        if (base->joints.empty() || cluster.ACDA.originals[i]->joints.empty())
            continue;
        std::vector<glm::vec3> replaced_joints;
        for (::Joint &j : base->joints)
            replaced_joints.push_back(glm::vec3(IDA.transforms[i]*glm::vec4(j.pos,1)));
        for (::Joint &j : cluster.ACDA.originals[i]->joints)
        {
            float d_min = 1e9;
            glm::vec3 n_min = glm::vec3(0);
            for (glm::vec3 &np : replaced_joints)
            {
                float d = glm::length(np - j.pos);
                if (d<d_min)
                {
                    d_min = d;
                    n_min = np;
                }
            }
            glm::vec3 sh = n_min - j.pos;
            for (::Branch *ch_b : j.childBranches)
            {
                glm::mat4 tr = glm::translate(glm::mat4(1.0f),sh);
                ch_b->transform(tr);
            }
        }
    }
}
void pack_branch_recursively(::Branch *b, GrovePacked &grove, std::vector<unsigned> &ids, BranchStructure &b_struct, int lvl_from, int lvl_to);
void pack_cluster(ClusterData &cluster, GrovePacked &grove, std::vector<BranchStructure> &instanced_structures, int lvl_from, int lvl_to)
{
    instanced_structures.push_back(BranchStructure());
    grove.instancedBranches.push_back(InstancedBranch());
    grove.instancedBranches.back().IDA = cluster.IDA;
    std::vector<unsigned> &ids = grove.instancedBranches.back().branches;
    ::Branch *base = cluster.base;
    pack_branch_recursively(base, grove, ids,instanced_structures.back(), lvl_from, lvl_to);
    if (instanced_structures.back().childBranches.size() == 1)
    {
        BranchStructure bs = instanced_structures.back().childBranches[0];
        instanced_structures.back() = bs;
    }
    for (int i=0;i<cluster.ACDA.originals.size();i++)//leave marks on branch to construct tree structure in future
    {
        cluster.ACDA.originals[i]->mark_A = instanced_structures.size() - 1;//cluster id
        cluster.ACDA.originals[i]->mark_B = - i - 100;//i is number in cluster
    }
    grove.instancedBranches.back().bbox = BillboardCloudRaw::get_minimal_bbox(base);
}
void pack_tree(::Tree &t, GrovePacked &grove, int up_to_level)
{
    for (int i = 0; i <= up_to_level; i++)
    {
        for (::Branch &branch : t.branchHeaps[i]->branches)
        {
            PackedBranch b;
            branch.pack(b);
            branch.mark_B = grove.uniqueCatalogue.add(b, branch.level);
        }
    }
}

void pack_structure(::Branch *rt, GrovePacked &grove, BranchStructure &str, std::vector<BranchStructure> &instanced_structures)
{
    str.pos = rt->mark_B;
    for (::Joint &j : rt->joints)
    {
        for (::Branch *br : j.childBranches)
        {
            if (br->mark_B < 0)//it is a mark made by pack_cluster
            {
                unsigned transform_n = - (br->mark_B + 100);
                unsigned instance_n = br->mark_A;
                BranchStructure bs = instanced_structures[instance_n];
                glm::mat4 tr = grove.instancedBranches[instance_n].IDA.transforms[transform_n];
                str.childBranchesInstanced.push_back(std::pair<glm::mat4,BranchStructure>(tr,bs));
            }
            else
            {
                BranchStructure bs;
                pack_structure(br,grove,bs,instanced_structures);
                str.childBranches.push_back(bs);
            }
            
        }
    }
}
void pack_branch_recursively(::Branch *b, GrovePacked &grove, std::vector<unsigned> &ids, BranchStructure &b_struct, int lvl_from, int lvl_to)
{
    if (b->level > lvl_to)
        return;
    if (b->level >= lvl_from)
    {
        PackedBranch pb;
        b->pack(pb);
        ids.push_back(grove.instancedCatalogue.add(pb, b->level));
        b_struct.childBranches.push_back(BranchStructure(ids.back()));
    }
    for (::Joint &j : b->joints)
    {
        for (::Branch *br : j.childBranches)
            pack_branch_recursively(br, grove, ids, b_struct.childBranches.back(), lvl_from, lvl_to);
    }
}
void add_occluder(LightVoxelsCube *voxels, Branch *b)
{
    for (Joint &j : b->joints)
    {
        voxels->set_occluder(j.pos,j.leaf ? 2.5 : 1);
        for (Branch *chb : j.childBranches)
            add_occluder(voxels,chb);
    }
}
void add_occluder(LightVoxelsCube *voxels, Tree *trees, int count)
{
    for (int i=0;i<count;i++)
    {
        if (trees[i].valid && trees[i].root)
            add_occluder(voxels,trees[i].root);
    }
}
bool is_valid_tree(::Tree &t)
{
    if (!t.valid)
    {
        logerr("tree %u was marked as not valid by a generator. Grove packer will ignore it",t.id);
        return false;
    }   
    if (!(t.valid && t.leaves && t.branchHeaps.size() >= 2))
    {
        logerr("tree %u do not have some of essential data structures. %d Grove packer will ignore it",t.id,
        t.branchHeaps.size());
        return false;
    }
    if (t.branchHeaps[0]->branches.empty())
    {
        logerr("tree %u do not have trunk. Grove packer will ignore it",t.id);
        return false;
    }
    if (t.branchHeaps[1]->branches.empty())
    {
        logerr("tree %u do not have branches instead of trunk. Grove packer will ignore it",t.id);
        return false;
    }
    return true;
}
void GrovePacker::pack_grove(GroveGenerationData ggd, GrovePacked &grove, DebugVisualizer &debug, 
                ::Tree *trees_external, Heightmap *h, bool visualize_voxels)
{
    grove.center = glm::vec3(0,0,0);
    grove.ggd_name = ggd.name;
    int synts = ggd.synts_count;
    int count = ggd.trees_count;
    LightVoxelsCube *post_voxels = nullptr;
    Seeder *post_seeder = nullptr;
    GroveGenerationData &curGgd = ggd;
    
    int valid_trees_cnt = 0;
    for (int i=0;i<count;i++)
    {
        bool valid = is_valid_tree(trees_external[i]);
        trees_external[i].valid = valid;
        valid_trees_cnt += valid;
    }
    if (count == 0)
    {
        logerr("Grove %s is empty. ",ggd.name.c_str());
        return;
    }
    if (valid_trees_cnt == 0)
    {
        logerr("Grove %s has %d tree(s) but none of them are valid.",ggd.name.c_str(), count);
        return;
    }

    debugl(1,"Packing grove %s with %d/%d valid trees",ggd.name.c_str(),valid_trees_cnt, count);

    {
        float r = sqrt(count);
        glm::vec3 vox_center = glm::vec3(0, 100, 0) + curGgd.pos;
        glm::vec3 vox_size = curGgd.size;
        auto &type = ggd.types[0];
        TreeStructureParameters base_params;
        ParameterSetWrapper params = ParameterSetWrapper(base_params,base_params.max_depth() + 1);
        params.set_state(params().max_depth() - 1);
        float single_voxel_size = params().seg_len_mult()/ params().light_precision();
        
        post_voxels = new LightVoxelsCube(vox_center, vox_size, params().seg_len_mult(), params().light_precision());
        post_seeder = new Seeder(ggd,10,h);

        post_voxels->add_heightmap(*h);
        for (int i=0;i<curGgd.obstacles.size();i++)
        {
            post_voxels->add_body(curGgd.obstacles[i]);
            post_seeder->add_body(curGgd.obstacles[i]);
        }
        post_seeder->recalcuate_shadows(trees_external,count);
        add_occluder(post_voxels,trees_external,count);
    }
    std::vector<ClusterData> trunks_clusters, branches_clusters, full_tree_clusters;
    
    
    std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
    if (ggd.task & (GenerationTask::SYNTS|GenerationTask::CLUSTERIZE))
    {
        Clusterizer tr_cl;
        ClusterizationParams tr_cp;
        tr_cp.weights = std::vector<float>{1,0,0,0.0,0.0};
        tr_cp.ignore_structure_level = 1;
        tr_cp.delta = 0.1;
        tr_cp.light_importance = 0;
        tr_cp.different_types_tolerance = true;
        tr_cp.r_weights = std::vector<float>{0.4,0,0,0.0,0.0}; 
        tr_cp.max_individual_dist = 0.4;
        tr_cp.bwd_rotations = 4;
        tr_cl.set_clusterization_params(tr_cp);
        tr_cl.set_branches(trees_external, count, 0, post_voxels);
        tr_cl.clusterize(trunks_clusters);
        for (ClusterData &cd : trunks_clusters)
        {
            transform_according_to_root(cd);
        }
    }

    std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
    
    if (ggd.task & (GenerationTask::SYNTS|GenerationTask::CLUSTERIZE|GenerationTask::BILLBOARDS))
    {
        Clusterizer cl;
        ClusterizationParams cp;
        cp.weights = std::vector<float>{5000,800,40,0.0,0.0};
        cp.ignore_structure_level = 2;
        cp.delta = 0.3;
        cp.max_individual_dist = ggd.clustering_max_individual_distance;
        cp.bwd_rotations = 4;

        cl.set_clusterization_params(cp);
        cl.set_branches(trees_external, count, 1, post_voxels);
        cl.clusterize(branches_clusters);

        for (int i = 0;i<branches_clusters.size();i++)
        {
            for (::Branch *br : branches_clusters[i].ACDA.originals)
            {
                br->mark_A = i;
            }
        }
    }

    std::chrono::steady_clock::time_point t3 = std::chrono::steady_clock::now();

    if (ggd.task & (GenerationTask::SYNTS))
    {
        SyntheticTreeGenerator stg = SyntheticTreeGenerator(*post_seeder, trunks_clusters, branches_clusters, curGgd);
        stg.generate(trees_external + count, synts, post_voxels);
    }

    std::chrono::steady_clock::time_point t4 = std::chrono::steady_clock::now();
    
    if (ggd.task & (GenerationTask::IMPOSTORS|GenerationTask::IMPOSTOR_FULL_GROVE))
    {
        Clusterizer cl2;
        ClusterizationParams cp;
        cp.weights = std::vector<float>{5000,800,40,0.0,0.0};
        cp.ignore_structure_level = 1;
        cp.delta = 0.3;
        cp.max_individual_dist = ggd.clustering_max_individual_distance;
        cp.bwd_rotations = 4;
        cp.light_importance = 0.8;
        cp.different_types_tolerance = false;
        cl2.set_clusterization_params(cp);
        cl2.set_branches(trees_external, count + synts, 0, post_voxels);
        cl2.clusterize(full_tree_clusters);
    }

    std::chrono::steady_clock::time_point t5 = std::chrono::steady_clock::now();

    grove.impostors.push_back(ImpostorsData());
    if (ggd.task & (GenerationTask::IMPOSTOR_FULL_GROVE))
    {
        ImpostorBaker *ib = new ImpostorBaker(2048, 2048, curGgd.types);
        
        ib->prepare_all_grove(ggd, 0, full_tree_clusters, &grove.impostors.back());

        delete(ib);
    }
    
    grove.impostors.push_back(ImpostorsData());
    if (ggd.task & (GenerationTask::IMPOSTORS))
    {
        ImpostorBaker *ib2 = new ImpostorBaker(ggd.impostor_quality,0,full_tree_clusters,curGgd.types,&grove.impostors.back());
        delete(ib2);
    }
    
    grove.clouds.push_back(BillboardCloudData());//empty 'zero' data refers to full-grove-impostor LOD
    grove.clouds.push_back(BillboardCloudData());//empty 'zero' data refers to impostors LOD
    grove.clouds.push_back(BillboardCloudData());//main cloud 1
    if (ggd.task & (GenerationTask::BILLBOARDS))
    {
        BillboardCloudRaw *cloud1 = new BillboardCloudRaw(ggd.bill_1_quality, 1,
                                                        branches_clusters,curGgd.types,&grove.clouds.back());
        delete(cloud1);
    }
    grove.clouds.push_back(BillboardCloudData());//main cloud 2
    if (ggd.task & (GenerationTask::BILLBOARDS))
    {    
        BillboardCloudRaw *cloud2 = new BillboardCloudRaw(ggd.bill_2_quality, 2,
                                                        branches_clusters,curGgd.types,&grove.clouds.back());
        delete(cloud2);
    }
    
    
    if (ggd.task & (GenerationTask::CLUSTERIZE))
    {
        std::vector<BranchStructure> instanced_structures;

        for (ClusterData &cd : trunks_clusters)
        {
            pack_cluster(cd, grove, instanced_structures, 0, 0);
        }
        for (ClusterData &cd : branches_clusters)
        {
            pack_cluster(cd, grove, instanced_structures, 1, 1000);
        }
        for (int i = 0; i < count; i++)
        {
            if (trees_external[i].root)
            {
                grove.roots.push_back(BranchStructure());
                pack_structure(trees_external[i].root,grove,grove.roots.back(),instanced_structures);
            }
        }
    }
    delete(post_voxels);
    delete(post_seeder);

    std::chrono::steady_clock::time_point t6 = std::chrono::steady_clock::now();
    /*std::cerr << "Generation took " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << "[ms]" << std::endl;
    std::cerr << "Main clusterization took " << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count() << "[ms]" << std::endl;
    std::cerr << "Syntetic trees generation took " << std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count() << "[ms]" << std::endl;
    std::cerr << "Secondary clusterization took " << std::chrono::duration_cast<std::chrono::milliseconds>(t5 - t4).count() << "[ms]" << std::endl;
    std::cerr << "Finishing took " << std::chrono::duration_cast<std::chrono::milliseconds>(t6 - t5).count() << "[ms]" << std::endl;
    */
}