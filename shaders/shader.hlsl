struct VSInput
{
    float2 pos : POSITION;
    float4 col : COLOR0;
};

struct PSInput
{
    float4 pos : SV_Position;
    float4 col : COLOR0;
};

PSInput vs(VSInput input)
{
    PSInput output;
    output.pos = float4(input.pos, 0.0f, 1.0f);
    output.col = input.col;
    return output;
}

float4 ps(PSInput input) : SV_Target
{
    return input.col;
}
