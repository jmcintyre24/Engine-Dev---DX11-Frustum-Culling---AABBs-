#include "mvp.hlsli"

struct VSIn
{
	uint vertexId : SV_VertexID;
};

struct VSOut
{
	float4 pos : SV_POSITION;
	float4 normal : NORMAL;
	float4 color : COLOR;
};

static const float4 cube_n[6] =
{
	float4(-1.0f, 0.0f, 0.0f, 0.0f),
	float4(1.0f, 0.0f, 0.0f, 0.0f),
	float4(0.0f, -1.0f, 0.0f, 0.0f),
	float4(0.0f, 1.0f, 0.0f, 0.0f),
	float4(0.0f, 0.0f, -1.0f, 0.0f),
	float4(0.0f, 0.0f, 1.0f, 0.0f),
};

static const float4 cube_v[8] =
{
	float4(-1.0f,-1.0f,-1.0f, 1.0f),
	float4(-1.0f,-1.0f, 1.0f, 1.0f),
	float4(-1.0f, 1.0f,-1.0f, 1.0f),
	float4(-1.0f, 1.0f, 1.0f, 1.0f),

	float4(1.0f,-1.0f,-1.0f, 1.0f),
	float4(1.0f,-1.0f, 1.0f, 1.0f),
	float4(1.0f, 1.0f,-1.0f, 1.0f),
	float4(1.0f, 1.0f, 1.0f, 1.0f),
};

static const uint cube_i[36] =
{
	0,1,2, // -x
	1,3,2,

	4,6,5, // +x
	5,6,7,

	0,5,1, // -y
	0,4,5,

	2,7,6, // +y
	2,3,7,

	0,6,4, // -z
	0,2,6,

	1,7,3, // +z
	1,5,7
};

VSOut main(VSIn input)
{
	VSOut output;

	uint vert_index = cube_i[input.vertexId];

	float4 vert_pos = cube_v[vert_index];
	float4 vert_norm = cube_n[input.vertexId / 6];

	output.pos = mul( vert_pos, modeling);
	output.pos = mul(output.pos, view);
	output.pos = mul(output.pos, projection);

	output.color = float4((vert_norm.xyz + 1.0f) * 0.5f, 1.0f);

	output.normal = mul(vert_norm, modeling);

	return output;
}
	