@echo off

pushd directx\code\shaders

fxc /T vs_5_0 /E main /Fo vertex.cso VertexShader.hlsl
fxc /T ps_5_0 /E main /Fo pixel.cso PixelShader.hlsl

popd
