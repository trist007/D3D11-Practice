/* date = March 29th 2026 10:54 am */

#ifndef SHEET_H
#define SHEET_H
typedef struct Sheet Sheet;

struct Sheet
{
    float r;
    float roll,  pitch,  yaw;
    float droll, dpitch, dyaw;
    float phi,   theta,  chi;
    float dphi,  dtheta, dchi;
    float cr, cg, cb, ca; 
};

void SheetInit(Sheet *s,
               float r,
               float droll, float dpitch, float dyaw,
               float dphi,  float dtheta, float dchi,
               float chi,   float theta,  float phi,
               float cr, float cg, float cb, float ca);

void SheetUpdate(Sheet *s, float dt);

DirectX::XMMATRIX SheetGetTransform(Sheet *s, DirectX::XMMATRIX projection);

void SheetDraw(Renderer *r, Sheet *s,
               Mesh *m, ShaderPipeline *sp,
               ConstantBuffers *cb,
               DirectX::XMMATRIX projection);

#endif //SHEET_H
