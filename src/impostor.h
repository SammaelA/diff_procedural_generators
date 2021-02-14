#pragma once
#include "billboard_cloud.h"
#include "tree.h"


class ImpostorBaker : public BillboardCloudRaw
{
public:
    ImpostorBaker(int tex_w, int tex_h, std::vector<TreeTypeData> &ttd) : BillboardCloudRaw(tex_w,tex_h,ttd) {};
    void prepare(Tree &t, int branch_level, std::vector<Clusterizer::Cluster> &clusters, std::list<int> &numbers,
                 ImpostorsData *data = nullptr);
    void make_impostor(Branch &b, Impostor &imp, int slices_n = 8);
private:

};
class ImpostorRenderer
{
    public:
    ImpostorRenderer(ImpostorsData *data = nullptr);
    ~ImpostorRenderer();
    void render(glm::mat4 &projectionCamera, glm::vec3 camera_pos = glm::vec3(0,0,0), glm::vec2 LOD_min_max = glm::vec2(0,0),
                glm::vec4 screen_size = glm::vec4(800,600,1/800,1/600));
    private:
    GLuint slicesBuffer = 0;
    ImpostorsData *data= nullptr;

    Shader impostorRenderer;
    //Shader impostorRendererInstancing;

    std::vector<Model *> models;
    std::vector<uint> offsets;
};