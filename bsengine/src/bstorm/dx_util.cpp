#include <bstorm/dx_util.hpp>

namespace bstorm
{
D3DXMATRIXA16 CreateScaleRotTransMatrix(float x, float y, float z, float rx, float ry, float rz, float sx, float sy, float sz)
{
    D3DXMATRIXA16 mat;
    //âÒì]
    D3DXMatrixRotationYawPitchRoll(&mat, D3DXToRadian(ry), D3DXToRadian(rx), D3DXToRadian(rz));

    //ägëÂèkè¨
    mat._11 *= sx;
    mat._12 *= sx;
    mat._13 *= sx;

    mat._21 *= sy;
    mat._22 *= sy;
    mat._23 *= sy;

    mat._31 *= sz;
    mat._32 *= sz;
    mat._33 *= sz;

    //à⁄ìÆ
    mat._41 = x;
    mat._42 = y;
    mat._43 = z;
    return mat;
}
}
