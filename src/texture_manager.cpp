#include "texture_manager.h"
#include "tinyEngine/helpers/image.h"
#include <exception>

Texture TextureManager::get(std::string name)
{
    auto t_it = textures.find(name);
    if (t_it == textures.end())
        return textures.at("texture not found");
    else
        return textures.at(name);
}
Texture TextureManager::get(int n)
{
    n = n % textures.size();
    int i =0;
    for (auto it = textures.begin(); it != textures.end(); it++)
    {
        if (i == n)
            return it->second;
        i++;
    }
}
Texture TextureManager::get_arr(int n)
{
    n = n % unnamed_array_textures.size();
    int i =0;
    for (auto it = unnamed_array_textures.begin(); it != unnamed_array_textures.end(); it++)
    {
        if (i == n)
            return it->second;
        i++;
    }
}
TextureManager::TextureManager()
{
    Texture t(false);
    textures.emplace("empty",t);
}
TextureManager::TextureManager(std::string base_path)
{
    image::base_img_path = base_path;
    std::vector<std::string> names = {"leaf1","leaf2","leaf","wood","wood2","wood3","texture not found"};
    std::vector<std::string> paths = {"leaf1.png","leaf2.png","leaf4.png","wood1.jpg","wood2.jpg","wood3.jpg","texture_not_found.png"};
    for (int i=0;i<paths.size();i++)
    {
        try
        {
            auto ptr = image::load(image::base_img_path + paths[i]);
            if (!ptr)
                continue;
            Texture t(ptr);
            textures.emplace(names[i],t);
        }
        catch(const std::exception& e)
        {
            logerr("texture not found %s",paths[i].c_str());
        }
    }
    debug("textures loaded %d\n",textures.size());
}
Texture TextureManager::create_unnamed(int w, int h, bool shadow)
{
    Texture t(w,h,shadow);
    unnamed_textures.emplace(t.texture,t);
    return t;
}
Texture TextureManager::create_unnamed_array(int w, int h, bool shadow, int layers)
{
    Texture t(w,h,shadow,layers);
    unnamed_array_textures.emplace(t.texture,t);
    return t;
}
Texture TextureManager::create_unnamed(SDL_Surface *s)
{
    Texture t(s);
    unnamed_textures.emplace(t.texture,t);
    return t;
}
Texture TextureManager::empty()
{
    return get("empty");
}
void TextureManager::clear_unnamed()
{
    for (auto const &p : unnamed_textures)
        glDeleteTextures(1, &(p.second.texture));
}