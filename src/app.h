#pragma once
#include <functional>
#include <chrono>
#include "core/grove.h"
#include "render/grove_renderer.h"
#include "tinyEngine/camera.h"
#include "core/tree.h"
#include "common_utils/sun.h"
#include "tinyEngine/event.h"
class FpsCounter
{
    float average_fps;
    uint64_t frame = 0;
    float mu = 0.99;
    std::chrono::steady_clock::time_point t1, t_prev;
public:
    FpsCounter();
    void tick();
    float get_average_fps() {return average_fps;}
    int get_frame_n() {return frame;}
};
enum RenderMode
{
    StartingScreen,
    Generating,
    Rendering
};
struct AppContext
{
    const int WIDTH = 1000;
    const int HEIGHT = 1000;
    RenderMode renderMode = StartingScreen;
    const int DEBUG_RENDER_MODE = -2;
    const int ARRAY_TEX_DEBUG_RENDER_MODE = -3;
    const int MAX_RENDER_MODE = 2;
    const float fov = glm::radians(90.0f);
    int forced_LOD = 4;
    int render_mode = 0;
    int debug_tex = 0;
    int debug_layer = 0;
    int benchmark_grove_needed = -1;
    int benchmark_grove_current = -1;
    glm::vec2 mousePos = glm::vec2(-1, -1);
    glm::vec4 mouseWorldPosType = glm::vec4(0,0,0,-1);//-1 means that mouse is not on scene geometry

    glm::mat4 projection = glm::perspective(fov, (float)WIDTH / HEIGHT, 1.0f, 3000.0f);

    GroveRendererDebugParams groveRendererDebugParams;
    FpsCounter fpsCounter;
    Camera camera;
    DirectedLight light;

    bool regeneration_needed = false;
    bool add_generation_needed = false;
    bool save_to_hydra = false;
    bool free_camera = false;
};

extern std::function<void(AppContext &, Event &)> eventHandler;