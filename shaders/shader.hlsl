struct VSInput
{
    float2 pos : POSITION;
    float2 tex_coord : TEXCOORD;
};

struct PSInput
{
    float4 pos : SV_Position;
    float2 tex_coord : TEXCOORD;
};

Texture2D tex : TEXTURE : register(t0);
SamplerState tex_sampler : SAMPLER : register(s0);

PSInput vs(VSInput input)
{
    PSInput output;
    output.pos = float4(input.pos, 0.0f, 1.0f);
    output.tex_coord = input.tex_coord;
    return output;
}

float4 ps(PSInput input) : SV_Target
{
    return tex.Sample(tex_sampler, input.tex_coord);
}
