struct VSOut
{
    float3 color : Color;
    float4 pos : SV_Position;
};

float4 main(VSOut vso) : SV_Target
{
    return float4(vso.color, 1.0f);
}

// compile: fxc /T ps_5_0 /E main /Fo pixel.cso PixelShader.hlsl
