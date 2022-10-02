#pragma once
#include "tinyEngine/texture.h"
#include "tinyEngine/postfx.h"
#include "canny.h"
#include <vector>

class SilhouetteExtractor
{
public:
  SilhouetteExtractor(float blur_sigma = 1.0, float low_thr = 0.125, float high_thr = 0.375);
  Texture get_silhouette(Texture &input);
private:
  Canny canny;
  PostFx fill_edges, detect_object, fill_silhouette;
};