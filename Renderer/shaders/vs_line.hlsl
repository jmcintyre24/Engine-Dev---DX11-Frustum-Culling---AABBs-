#include "mvp.hlsli"

struct VSIn
{
    float3 pos : POSITION;
    float4 color : COLOR;
};

struct VSOut
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

VSOut main(VSIn input)
{
    VSOut output;

    float4 posi = { input.pos.x, input.pos.y, input.pos.z, 1.0f };
    
    output.pos = mul(posi, modeling);
    output.pos = mul(output.pos, view);
    output.pos = mul(output.pos, projection);
    
    output.color = input.color;

    return output;
}
	