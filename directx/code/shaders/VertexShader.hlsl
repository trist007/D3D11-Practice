struct VSOut
{
    float3 color: Color;
    float4 pos: SV_Position;
};

// the Constant Buffer has the matrix to rotate the mesh a little each frame
cbuffer CBuf
{
    matrix transform;
};

VSOut main(float2 pos : Position,float3 color : Color)
{
    VSOut vso;
    vso.pos = mul(float4(pos.x, pos.y, 0.0f, 1.0f),transform);
    vso.color = color;
    return vso;
}

// compile: fxc /T vs_5_0 /E main /Fo vertex.cso VertexShader.hlsl
