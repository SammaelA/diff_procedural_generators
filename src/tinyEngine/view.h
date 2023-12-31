#pragma once
#include <deque>
#include <unordered_map>
#include <string>
#include "SDL2/SDL.h"
#include <functional>
#include <glm/glm.hpp>
#include "../third_party/imgui/imgui.h"                    //Interface Dependencies
#include "../third_party/imgui/imgui_impl_sdl.h"
#include "../third_party/imgui/imgui_impl_opengl3.h"
#include "tinyEngine/resources.h"    
#include "tinyEngine/audio.h"

class View
{
  public:
    void target(glm::vec3 clearcolor);  //Target main window for drawing
    bool init(std::string windowName, int width, int height);
    void quit();
    void next_frame();
    void handle_input();
    unsigned int get_antialiasing();

    bool enabled = false;
    unsigned int WIDTH, HEIGHT;
    SDL_Window* gWindow;        //Window Pointer
    SDL_GLContext gContext;     //Render Context
    Audio audio;
    ImGuiIO io;
    bool showInterface = true;
    bool fullscreen = false;    //Settings
    bool vsync = true;
    bool ccw = true;
    float lineWidth = 1.0f;
};