#pragma once

#include <bstorm/non_copyable.hpp>

#include <d3dx9.h>
#include <array>
#include <memory>

namespace bstorm
{
struct Vertex;
class Shader;
class Mesh;
class Renderer : private NonCopyable
{
public:
    Renderer(IDirect3DDevice9*);
    ~Renderer();
    // NOTE : initRenderState : 描画デバイスの初期化を行う
    // デバイスロストやライブラリによってデバイスの状態が書き換えられた場合も呼ぶ必要がある
    // 一連の描画の前に1回呼べばよい
    void InitRenderState();
    void RenderPrim2D(D3DPRIMITIVETYPE primType, int vertexCount, const Vertex* vertices, IDirect3DTexture9* texture, int blendType, const D3DXMATRIX& worldMatrix, const std::shared_ptr<Shader>& pixelShader, bool permitCamera, bool insertHalfPixelOffset);
    void RenderPrim3D(D3DPRIMITIVETYPE primType, int vertexCount, const Vertex* vertices, IDirect3DTexture9* texture, int blendType, const D3DXMATRIX& worldMatrix, const std::shared_ptr<Shader>& pixelShader, bool zWriteEnable, bool zTestEnable, bool useFog, bool billboardEnable_);
    void RenderMesh(const std::shared_ptr<Mesh>& mesh, const D3DCOLORVALUE& col, int blendType, const D3DXMATRIX& worldMatrix, const std::shared_ptr<Shader>& pixelShader, bool zWriteEnable, bool zTestEnable, bool useFog);
    void SetViewProjMatrix2D(const D3DXMATRIX& view, const D3DXMATRIX& proj);
    void SetForbidCameraViewProjMatrix2D(int screenWidth, int screenHeight);
    void SetViewProjMatrix3D(const D3DXMATRIX& view, const D3DXMATRIX& proj);
    void SetViewProjMatrix3D(const D3DXMATRIX& view, const D3DXMATRIX& proj, const D3DXMATRIX& billboardMatrix);
    void SetBlendType(int type);
    // NOTE : enable/disableScissorTest : デバイスロストすると設定値は消える
    void EnableScissorTest(const RECT& rect);
    void DisableScissorTest();
    void SetFogEnable(bool enable);
    void SetFogParam(float fogStart, float fogEnd, int r, int g, int b);
private:
    IDirect3DDevice9 * d3DDevice_;
    IDirect3DVertexShader9* prim2DVertexShader_;
    IDirect3DVertexShader9* prim3DVertexShader_;
    IDirect3DVertexShader9* meshVertexShader_;
    float screenWidth_;
    float screenHeight_;
    int currentBlendType_;
    D3DXMATRIXA16 viewProjMatrix2D_;
    D3DXMATRIXA16 viewProjMatrix3D_;
    D3DXMATRIXA16 billboardViewProjMatrix3D_;
    D3DXMATRIXA16 forbidCameraViewProjMatrix2D_;
    D3DXMATRIXA16 halfPixelOffsetMatrix_;
    bool fogEnable_;
    float fogStart_;
    float fogEnd_;
    D3DCOLOR fogColor_;
};
}