struct Camera
{
    float r;
    float theta;
    float phi;
    float pitch;
    float yaw;
    float roll;
};

void
CameraInit(Camera *c)
{
    c->r     = 20.0f;
    c->theta = 0.0f;
    c->phi   = 0.0f;
    c->pitch = 0.0f;
    c->yaw   = 0.0f;
    c->roll  = 0.0f;
}

DirectX::XMMATRIX
CameraGetMatrix(Camera *c)
{
    namespace dx = DirectX;
    
    const dx::XMVECTOR pos = dx::XMVector3Transform(
                                                    dx::XMVectorSet(0.0f, 0.0f, -c->r, 0.0f),
                                                    dx::XMMatrixRotationRollPitchYaw(c->phi, -c->theta, 0.0f)
                                                    );
    
    return dx::XMMatrixLookAtLH(
                                pos,
                                dx::XMVectorZero(),
                                dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
                                ) * dx::XMMatrixRotationRollPitchYaw(c->pitch, -c->yaw, c->roll);
}

void
CameraReset(Camera *c)
{
    c->r     = 20.0f;
    c->theta = 0.0f;
    c->phi   = 0.0f;
    c->pitch = 0.0f;
    c->yaw   = 0.0f;
    c->roll  = 0.0f;
}

void
CameraSpawnControlWindow(Camera *c)
{
    if(ImGui::Begin("Camera"))
    {
        ImGui::Text("Position");
        ImGui::SliderFloat("R",     &c->r,     0.0f, 80.0f, "%.1f");
        ImGui::SliderAngle("Theta", &c->theta, -180.0f, 180.0f);
        ImGui::SliderAngle("Phi",   &c->phi,   -89.0f,  89.0f);
        ImGui::Text("Orientation");
        ImGui::SliderAngle("Roll",  &c->roll,  -180.0f, 180.0f);
        ImGui::SliderAngle("Pitch", &c->pitch, -180.0f, 180.0f);
        ImGui::SliderAngle("Yaw",   &c->yaw,   -180.0f, 180.0f);
        if(ImGui::Button("Reset"))
            CameraReset(c);
    }
    ImGui::End();
}