/////////////
// GLOBALS //
/////////////
cbuffer cbMatrix : register(b0)
{
    matrix  g_mViewProjection;
    matrix  g_mWorld;
    float3  g_viewPoint;
    float   g_time;
    float2 g_renderTargetSize;
}


//////////////////////
// CONSTANT BUFFERS //
//////////////////////
cbuffer cbMatrix2 : register(b1)
{
    matrix g_lightProjView;
    int mode;
}

//////////////
// TYPEDEFS //
//////////////
struct VertexInputType
{
    int    Sequence : SEQ;
    float3 Position     : POSITION;
    float3 Color        : COLOR;
    float3 Direction    : DIR;
    float3 Reference    : REF;
};
struct VSInput
{
    float4 Position     : POSITION;
    float3 Color        : COLOR;
};


struct  VSOutput
{
    float4 Position     : SV_POSITION; 
    float4 Color        : COLOR0;
};

VSOutput VS(VertexInputType input)
{
    VSOutput Output;
    Output.Position = mul(float4(input.Position, 1.0), g_mViewProjection);
    Output.Color = float4(input.Color, 1.0);
    return Output;
}

float4 PS(VSOutput input) : SV_TARGET
{
    return input.Color;
}
