#include <bstorm/camera3D.hpp>

#include <d3dx9.h>

namespace bstorm
{
Camera3D::Camera3D() :
    focusX_(0.0f),
    focusY_(0.0f),
    focusZ_(0.0f),
    radius_(500.0f),
    azimuthAngle_(15.0f),
    elevationAngle_(45.0f),
    yaw_(0.0f),
    pitch_(0.0f),
    roll_(0.0f),
    nearClip_(10.0f),
    farClip_(2000.0f)
{
}

float Camera3D::GetX() const
{
    return focusX_ + radius_ * cos(D3DXToRadian(elevationAngle_)) * cos(D3DXToRadian(azimuthAngle_));
}

float Camera3D::GetY() const
{
    return focusY_ + radius_ * sin(D3DXToRadian(elevationAngle_));
}

float Camera3D::GetZ() const
{
    return focusZ_ + radius_ * cos(D3DXToRadian(elevationAngle_)) * sin(D3DXToRadian(azimuthAngle_));
}

void Camera3D::GenerateViewMatrix(D3DXMATRIX* view, D3DXMATRIX* billboard) const
{
    D3DXMATRIXA16 rotYawPitch, rotYawPitchRoll;
    D3DXMatrixRotationYawPitchRoll(&rotYawPitch, D3DXToRadian(yaw_), D3DXToRadian(pitch_), 0);
    D3DXMatrixRotationYawPitchRoll(&rotYawPitchRoll, D3DXToRadian(yaw_), D3DXToRadian(pitch_), D3DXToRadian(roll_));

    // eye: カメラ位置
    const D3DXVECTOR3 eye = D3DXVECTOR3(GetX(), GetY(), GetZ());

    // at: 注視点
    const D3DXVECTOR3 focus = D3DXVECTOR3(focusX_, focusY_, focusZ_);
    D3DXVECTOR3 gaze = focus - eye; // 視線
    D3DXVec3TransformCoord(&gaze, &gaze, &rotYawPitch);
    const D3DXVECTOR3 at = eye + gaze;

    // up: 上方
    D3DXVECTOR3 up = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
    D3DXVec3TransformCoord(&up, &up, &rotYawPitchRoll);

    // ビュー行列
    D3DXMatrixLookAtLH(view, &eye, &at, &up);

    // ビルボード行列行列
    D3DXMatrixLookAtLH(billboard, &D3DXVECTOR3(0.0f, 0.0f, 0.0f), &gaze, &up);
    D3DXMatrixInverse(billboard, NULL, billboard);
}

void Camera3D::GenerateProjMatrix(D3DXMATRIX* proj, float screenWidth, float screenHeight, float cameraScreenX, float cameraScreenY) const
{
    // 射影行列作成 → 移動
    D3DXMatrixPerspectiveFovLH(proj, D3DXToRadian(47.925), screenWidth / screenHeight, nearClip_, farClip_);
    const float tx = (-screenWidth / 2.0f + cameraScreenX) * 2.0f / screenWidth;
    const float ty = (screenHeight / 2.0f - cameraScreenY) * 2.0f / screenHeight;
    proj->_31 = tx; proj->_32 = ty;
}
}