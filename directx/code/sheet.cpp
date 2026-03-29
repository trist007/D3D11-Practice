#include "sheet.h"

void
SheetInit(Sheet *s,
          float r,
          float droll, float dpitch, float dyaw,
          float dphi,  float dtheta, float dchi,
          float chi,   float theta,  float phi,
          float cr, float cg, float cb, float ca)
{
    s->r      = r;
    s->roll   = 0.0f; s->pitch  = 0.0f; s->yaw   = 0.0f;
    s->droll  = droll; s->dpitch = dpitch; s->dyaw  = dyaw;
    s->dphi   = dphi;  s->dtheta = dtheta; s->dchi  = dchi;
    s->chi    = chi;   s->theta  = theta;  s->phi   = phi;
    s->cr = cr;
    s->cg = cg;
    s->cb = cb;
    s->ca = ca;
}

void
SheetUpdate(Sheet *s, float dt)
{
    s->roll  += s->droll  * dt;
    s->pitch += s->dpitch * dt;
    s->yaw   += s->dyaw   * dt;
    s->theta += s->dtheta * dt;
    s->phi   += s->dphi   * dt;
    s->chi   += s->dchi   * dt;
}

DirectX::XMMATRIX
SheetGetTransform(Sheet *s)
{
    return (
            DirectX::XMMatrixRotationRollPitchYaw(s->pitch, s->yaw, s->roll) *
            DirectX::XMMatrixTranslation(s->r, 0.0f, 0.0f) *
            DirectX::XMMatrixRotationRollPitchYaw(s->theta, s->phi, s->chi) *
            DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f));
}

void
SheetDraw(Renderer *r, Sheet *s,
          Mesh *m, ShaderPipeline *sp,
          ConstantBuffers *cb,
          UINT width, UINT height)
{
    ConstantBuffersUpdateTransform(r, cb, SheetGetTransform(s));
    
    float color[4] = { s->cr, s->cg, s->cb, s->ca };
    ConstantBuffersUpdateColor(r, cb, color);
    
    r->context->VSSetShader(sp->vs, 0, 0u);
    r->context->PSSetShader(sp->ps, 0, 0u);
    r->context->IASetInputLayout(sp->input_layout);
    
    UINT stride = sizeof(Vertex);
    UINT offset = 0u;
    r->context->IASetVertexBuffers(0u, 1u, &m->vertex_buffer, &stride, &offset);
    r->context->IASetIndexBuffer(m->index_buffer, DXGI_FORMAT_R16_UINT, 0u);
    
    r->context->VSSetConstantBuffers(0u, 1u, &cb->transform);
    r->context->PSSetConstantBuffers(0u, 1u, &cb->face_colors);
    
    r->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    D3D11_VIEWPORT vp = {};
    vp.Width    = (float)width; vp.Height   = (float)height;
    vp.MinDepth = 0;   vp.MaxDepth = 1;
    r->context->RSSetViewports(1u, &vp);
    
    r->context->DrawIndexed(m->index_count, 0u, 0u);
}