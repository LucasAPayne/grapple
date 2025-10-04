struct VSInput
{
    float2 pos   : POSITION;
    float2 uv    : TEXCOORD;
    float4 color : COLOR;
};

struct PSInput
{
    float4 pos   : SV_Position;
    float2 uv    : TEXCOORD;
    float4 color : COLOR;
};

cbuffer ProjectionBuffer : register(b0)
{
    float4x4 proj;
};

Texture2D tex : TEXTURE : register(t0);
SamplerState tex_sampler : SAMPLER : register(s0);

PSInput vs(VSInput input)
{
    PSInput output;
    float4 position = float4(input.pos, 0.0f, 1.0f);
    output.pos = mul(position, proj);
    output.uv = input.uv;
    output.color = input.color;
    return output;
}

float4 ps(PSInput input) : SV_Target
{
    float4 tex_color = tex.Sample(tex_sampler, input.uv);
    return tex_color * input.color;
}
