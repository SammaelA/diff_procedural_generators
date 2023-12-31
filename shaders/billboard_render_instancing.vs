#version 460

struct TypeData
{
    uint offset;
    uint pad1,pad2,pad3;
};
layout(std140, binding=1) readonly buffer _TypeData
{
    TypeData typeData[];
};
struct InstanceData
{
    vec4 center_self;
    vec4 center_par;
    mat4 projection_camera;
};
layout(std140, binding=3) readonly buffer _InstanceData
{
    InstanceData instances[];
};
struct ModelData
{
    uint LOD;
    uint type;
    uint vertexes;
    uint first_index;
    uvec2 interval;
    uint culling;
    uint pad;
    vec4 x_s;
    vec4 y_s;
    vec4 z_s;
};
layout(std140, binding=4) readonly buffer _ModelData
{
    ModelData modelData[];
};
struct currentInstancesData
{
    uint index;
    uint pad;
    float mn;
    float mx;
};
layout(std140, binding=5) readonly buffer _curInsts
{
    currentInstancesData curInsts[];
};

layout(std140, binding=6) readonly buffer _curModels
{
    uvec4 curModels[];
};

in vec3 in_Position;
in vec3 in_Normal;
in vec4 in_Tex;

uniform mat4 projection;
uniform mat4 view;
uniform vec3 camera_pos;
uniform uint type_id;
uniform mat4 lightSpaceMatrix;

out vec3 ex_Tex;
out vec3 ex_Normal;
out vec3 ex_FragPos;
out vec4 ex_FragPosView;
out vec2 a_mult;
flat out uint model_id;
out vec4 FragPosLightSpace;
out mat4 normalTr;

void main(void) {

	uint id = typeData[type_id].offset + gl_DrawID;
	uint offset = curModels[id].x + gl_InstanceID;
	mat4 inst_mat = instances[curInsts[offset].index].projection_camera;
    normalTr = transpose(inverse(inst_mat));
	a_mult = vec2(curInsts[offset].mn, curInsts[offset].mx);

	ex_FragPos = (inst_mat * vec4(in_Position, 1.0f)).xyz;
	ex_Normal = (normalTr*vec4(in_Normal,0)).xyz;
	ex_FragPosView = view * vec4(ex_FragPos, 1.0f);
    gl_Position = projection * ex_FragPosView;
	ex_Tex = in_Tex.xyz;
	model_id = curModels[id].y;
    FragPosLightSpace = lightSpaceMatrix * vec4(ex_FragPos, 1.0);
}