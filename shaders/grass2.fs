#version 330

in vec3 ex_Tex;
in vec3 ex_Normal;
in vec3 ex_FragPos;
in vec4 ex_FragPosView;

uniform sampler2DArray tex;

layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 fragNormal;
layout (location = 2) out vec4 fragViewPos;
layout (location = 3) out vec4 fragWorldPos;

void main(void) 
{
  fragColor = texture(tex,ex_Tex.xyz);
  if (fragColor.a<0.5)
    discard;
  fragColor.rgb/fragColor.a;

  fragNormal = vec4(ex_Normal.xyz,3);
  fragViewPos = vec4(ex_FragPosView.xyz,1);
  fragWorldPos = vec4(ex_FragPos,1);

}