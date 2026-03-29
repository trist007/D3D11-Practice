cbuffer CBFaceColors : register(b0)
{
    float4 face_colors[6];
};

float4 main(float2 tc : TexCoord) : SV_Target
{
    return face_colors[0];
}
// compile: fxc /T ps_5_0 /E main /Fo sheet_pixel.cso SheetPixelShader.hlsl