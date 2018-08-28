#include <bstorm/camera2D.hpp>

#include <algorithm>
#include <d3dx9.h>

namespace bstorm
{
Camera2D::Camera2D()
{
    Reset(0.0f, 0.0f);
}

float Camera2D::GetRatio() const { return std::max(GetRatioX(), GetRatioY()); }

void Camera2D::Reset(float focusX, float focusY)
{
    SetFocusX(focusX);
    SetFocusY(focusY);
    SetAngleZ(0.0f);
    SetRatio(1.0f);
}

void Camera2D::GenerateViewMatrix(D3DXMATRIX* view) const
{
    // カメラ中心に変換してから拡大して回転
    D3DXMATRIXA16 rot;
    D3DXMatrixLookAtLH(view, &D3DXVECTOR3(focusX_, focusY_, 1.0f), &D3DXVECTOR3(focusX_, focusY_, 0.0f), &D3DXVECTOR3(0.0f, -1.0f, 0.0f));
    D3DXMatrixRotationZ(&rot, D3DXToRadian(-angleZ_));
    rot._11 *= ratioX_; rot._12 *= ratioX_;
    rot._21 *= ratioY_; rot._22 *= ratioY_;
    *view = (*view) * rot;
}

void Camera2D::GenerateProjMatrix(D3DXMATRIX* proj, float screenWidth, float screenHeight, float cameraScreenX, float cameraScreenY) const
{
    // 移動してから拡大
    const float tx = -screenWidth / 2.0f + cameraScreenX;
    const float ty = screenHeight / 2.0f - cameraScreenY;
    const float sx = 2.0f / screenWidth;
    const float sy = 2.0f / screenHeight;
    D3DXMatrixScaling(proj, sx, sy, 1.0f);
    proj->_41 = sx * tx;
    proj->_42 = sy * ty;
}
}
