struct VSOut
{
    float3 color: Color;
    float4 pos: SV_Position;
};

// the Constant Buffer has the matrix to rotate the mesh a little each frame
cbuffer CBuf
{
    // NOTE(trist007): using row_major to Transpose matrix otherwise mesh gets deformed
    // HLSL stores matrices in column_major order by default but C code files the constant
    // buffer in row_major order, so when HLSL reads this as column_major the data is
    // interpreted differently so row 0 gets read as column 0 which transposes the matrix
    // can also use DirectXMath function XMMatrixTranspose on the C side before uploading it
    // this also changes the direction of rotation
    row_major matrix transform;
};

VSOut main(float2 pos : Position,float3 color : Color)
{
    VSOut vso;
    vso.pos = mul(float4(pos.x, pos.y, 0.0f, 1.0f),transform);
    vso.color = color;
    return vso;
}

// compile: fxc /T vs_5_0 /E main /Fo vertex.cso VertexShader.hlsl
