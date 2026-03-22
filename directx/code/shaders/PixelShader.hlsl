float4 main() : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}

// compile: fxc /T ps_5_0 /E main /Fo pixel.cso PixelShader.hlsl