struct VSOut
{
	float4 pos : SV_POSITION;
	float4 normal : NORMAL;
	float4 color : COLOR;
};

struct PS_OUTPUT 
{
	float4 color : SV_TARGET;
};

PS_OUTPUT main(VSOut input)
{
	PS_OUTPUT output;

	output.color = input.color;

	return output;
}