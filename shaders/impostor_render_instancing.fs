#version 430

in vec3 tc_a;
in vec3 tc_b;
in vec3 tc_t;
in vec3 q_abt;
in vec3 ex_Normal;
in vec3 ex_FragPos;
in vec4 ex_FragPosView;
in vec2 a_mult;
in mat4 rot_m;
in mat4 normalTr;
in vec4 FragPosLightSpace;
flat in uint model_id;

uniform sampler2DArray color_tex;
uniform sampler2DArray normal_tex;
uniform sampler2D noise;
uniform vec4 screen_size;
uniform int debug_model_id;

layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 fragNormal;
layout (location = 2) out vec4 fragViewPos;
layout (location = 3) out vec4 fragWorldPos;

float gradientNoise(float x, float y)
{
  float f = 0.06711056f * x + 0.00583715f * y;
  return fract(52.9829189f * fract(f));
}
void main(void) 
{
  vec2 noise_pos = vec2(fract(25*gl_FragCoord.x*screen_size.z), fract(25*gl_FragCoord.y*screen_size.w));
  float ns_imp = texture(noise,noise_pos).x;
  float ns = gradientNoise(float(gl_FragCoord.x),float(gl_FragCoord.y));
  vec3 tc = ns_imp <= q_abt.x ? tc_a : (ns_imp <= q_abt.x + q_abt.y ? tc_b : tc_t);
  fragColor = texture(color_tex,tc);
  if (fragColor.a<0.33)
    discard;

  if ((a_mult.y > 0.1 && ns > a_mult.x) || (a_mult.y < -0.1 && ns <  1  - a_mult.x))
    discard;
  fragColor.rgb /= fragColor.a;
  if (int(model_id) == debug_model_id)
    fragColor = vec4(1,0,1,1);

  fragNormal = vec4(ex_Normal.xyz,2);
  fragViewPos = vec4(ex_FragPosView.xyz,1);
  fragWorldPos = vec4(ex_FragPos,1);

}
