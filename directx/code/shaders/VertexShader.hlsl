// the Constant Buffer has the matrix to rotate the mesh a little each frame
cbuffer CBuf
{
    // NOTE(trist007): using row_major to Transpose matrix otherwise mesh gets deformed
    // HLSL stores matrices in column_major order by default but C code files the constant
    // buffer in row_major order, so when HLSL reads this as column_major the data is
    // interpreted differently so row 0 gets read as column 0 which transposes the matrix
    // can also use DirectXMath function XMMatrixTranspose on the C side before uploading it
    // this also changes the direction of rotation
    // NOTE(trist007): doing row_major is slower so i will remove row_major from matrix transform
    // just be sure to run dx::XMMatrixTranspose on the matrixrotationz and matrixscaling 
    matrix transform;
};

float4 main(float3 pos : Position) : SV_Position
{
    return mul(float4(pos, 1.0f), transform);
}

// compile: fxc /T vs_5_0 /E main /Fo vertex.cso VertexShader.hlsl
