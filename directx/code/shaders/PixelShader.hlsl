cbuffer CBuf
{
    float4 face_colors[6];
};

float4 main(uint tid : SV_PrimitiveID) : SV_Target
{
    return face_colors[tid / 2];
}

// compile: fxc /T ps_5_0 /E main /Fo pixel.cso PixelShader.hlsl
