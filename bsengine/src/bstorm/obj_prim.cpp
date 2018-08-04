#include <bstorm/obj_prim.hpp>

#include <bstorm/dnh_const.hpp>
#include <bstorm/math_util.hpp>
#include <bstorm/dx_util.hpp>
#include <bstorm/texture.hpp>
#include <bstorm/render_target.hpp>
#include <bstorm/renderer.hpp>
#include <bstorm/package.hpp>

#include <d3dx9.h>

static bool isValidIndex(int i, const std::vector<bstorm::Vertex>& v)
{
    return i >= 0 && i < v.size();
}

namespace bstorm
{
ObjPrim::ObjPrim(const std::shared_ptr<Package>& state) :
    ObjRender(state),
    primType_(D3DPT_TRIANGLELIST)
{
}

ObjPrim::~ObjPrim()
{
}

void ObjPrim::Update()
{
}

int ObjPrim::GetPrimitiveType() const
{
    return static_cast<int>(primType_);
}

void ObjPrim::SetPrimitiveType(int t)
{
    primType_ = static_cast<_D3DPRIMITIVETYPE>(t);
}

void ObjPrim::SetVertexCount(int cnt, bool doClear)
{
    if (doClear) vertices_.clear();
    vertices_.resize(cnt);
}

int ObjPrim::GetVertexCount() const
{
    return vertices_.size();
}

void ObjPrim::SetTexture(const std::shared_ptr<Texture> &tex)
{
    renderTarget_.reset();
    texture_ = tex;
}

void ObjPrim::SetRenderTarget(const std::shared_ptr<RenderTarget>& target)
{
    texture_.reset();
    renderTarget_ = target;
}

void ObjPrim::SetVertexPosition(int vIdx, float x, float y, float z)
{
    if (isValidIndex(vIdx, vertices_))
    {
        vertices_[vIdx].x = x;
        vertices_[vIdx].y = y;
        vertices_[vIdx].z = z;
    }
}

float ObjPrim::GetVertexPositionX(int vIdx) const
{
    if (isValidIndex(vIdx, vertices_))
    {
        return vertices_[vIdx].x;
    }
    return 0;
}

float ObjPrim::GetVertexPositionY(int vIdx) const
{
    if (isValidIndex(vIdx, vertices_))
    {
        return vertices_[vIdx].y;
    }
    return 0;
}

float ObjPrim::GetVertexPositionZ(int vIdx) const
{
    if (isValidIndex(vIdx, vertices_))
    {
        return vertices_[vIdx].z;
    }
    return 0;
}

void ObjPrim::SetVertexUV(int vIdx, float u, float v)
{
    if (isValidIndex(vIdx, vertices_))
    {
        vertices_[vIdx].u = u;
        vertices_[vIdx].v = v;
    }
}

void ObjPrim::SetVertexUVT(int vIdx, float u, float v)
{
    auto d3DTex = GetD3DTexture();
    if (d3DTex)
    {
        SetVertexUV(vIdx, u / GetD3DTextureWidth(d3DTex), v / GetD3DTextureHeight(d3DTex));
    }
}

void ObjPrim::SetVertexColor(int vIdx, int r, int g, int b)
{
    if (isValidIndex(vIdx, vertices_))
    {
        ColorRGB rgb(r, g, b);
        vertices_[vIdx].color = rgb.ToD3DCOLOR(vertices_[vIdx].color >> 24);
    }
}

void ObjPrim::SetVertexAlpha(int vIdx, int a)
{
    if (isValidIndex(vIdx, vertices_))
    {
        a = constrain(a, 0, 0xff);
        vertices_[vIdx].color &= D3DCOLOR_RGBA(0xff, 0xff, 0xff, 0);
        vertices_[vIdx].color |= D3DCOLOR_RGBA(0, 0, 0, a);
    }
}

const std::shared_ptr<Texture>& ObjPrim::GetTexture() const
{
    return texture_;
}

const std::shared_ptr<RenderTarget>& ObjPrim::GetRenderTarget() const
{
    return renderTarget_;
}

const std::vector<Vertex>& ObjPrim::GetVertices() const
{
    return vertices_;
}

IDirect3DTexture9 * ObjPrim::GetD3DTexture() const
{
    if (texture_)
    {
        return texture_->GetTexture();
    } else if (renderTarget_)
    {
        return renderTarget_->GetTexture();
    }
    return NULL;
}

_D3DPRIMITIVETYPE ObjPrim::GetD3DPrimitiveType() const
{
    return primType_;
}

ObjPrim2D::ObjPrim2D(const std::shared_ptr<Package>& state) :
    ObjPrim(state)
{
    SetType(OBJ_PRIMITIVE_2D);
}

void ObjPrim2D::Render(const std::shared_ptr<Renderer>& renderer)
{
    D3DXMATRIXA16 world = CreateScaleRotTransMatrix(GetX(), GetY(), GetZ(), GetAngleX(), GetAngleY(), GetAngleZ(), GetScaleX(), GetScaleY(), GetScaleZ());
    renderer->RenderPrim2D(GetD3DPrimitiveType(), vertices_.size(), vertices_.data(), GetD3DTexture(), GetBlendType(), world, GetAppliedShader(), IsPermitCamera(), true);
}

ObjSprite2D::ObjSprite2D(const std::shared_ptr<Package>& state) :
    ObjPrim2D(state)
{
    SetType(OBJ_SPRITE_2D);
    SetPrimitiveType(PRIMITIVE_TRIANGLESTRIP);
    SetVertexCount(4);
}

void ObjSprite2D::SetSourceRect(float left, float top, float right, float bottom)
{
    SetVertexUVT(0, left, top);
    SetVertexUVT(1, right, top);
    SetVertexUVT(2, left, bottom);
    SetVertexUVT(3, right, bottom);
}

void ObjSprite2D::SetDestRect(float left, float top, float right, float bottom)
{
    SetVertexPosition(0, left, top, 0);
    SetVertexPosition(1, right, top, 0);
    SetVertexPosition(2, left, bottom, 0);
    SetVertexPosition(3, right, bottom, 0);
}

void ObjSprite2D::SetDestCenter()
{
    if (GetVertexCount() < 4) return;
    auto texture = GetD3DTexture();
    if (texture)
    {
        float hw = GetD3DTextureWidth(texture) / 2.0 * (vertices_[3].u - vertices_[0].u);
        float hh = GetD3DTextureHeight(texture) / 2.0 * (vertices_[3].v - vertices_[0].v);
        SetDestRect(-hw, -hh, hw, hh);
    }
}

ObjSpriteList2D::ObjSpriteList2D(const std::shared_ptr<Package>& state) :
    ObjPrim2D(state),
    isVertexClosed_(false),
    srcRectLeft_(0),
    srcRectTop_(0),
    srcRectRight_(0),
    srcRectBottom_(0),
    dstRectLeft_(0),
    dstRectTop_(0),
    dstRectRight_(0),
    dstRectBottom_(0)
{
    SetType(OBJ_SPRITE_LIST_2D);
    SetPrimitiveType(D3DPT_TRIANGLELIST);
}

void ObjSpriteList2D::Render(const std::shared_ptr<Renderer>& renderer)
{
    if (isVertexClosed_)
    {
        ObjPrim2D::Render(renderer);
    } else
    {
        // 保存
        float prevX = GetX();
        float prevY = GetY();
        float prevZ = GetZ();
        float prevAngleX = GetAngleX();
        float prevAngleY = GetAngleY();
        float prevAngleZ = GetAngleZ();
        float prevScaleX = GetScaleX();
        float prevScaleY = GetScaleY();
        float prevScaleZ = GetScaleZ();
        // パラメータをリセットして描画
        SetPosition(0, 0, 0);
        SetAngleXYZ(0, 0, 0);
        SetScaleXYZ(1, 1, 1);
        ObjPrim2D::Render(renderer);
        // 復元
        SetPosition(prevX, prevY, prevZ);
        SetAngleXYZ(prevAngleX, prevAngleY, prevAngleZ);
        SetScaleXYZ(prevScaleX, prevScaleY, prevScaleZ);
    }
}

void ObjSpriteList2D::SetSourceRect(float left, float top, float right, float bottom)
{
    srcRectLeft_ = left;
    srcRectTop_ = top;
    srcRectRight_ = right;
    srcRectBottom_ = bottom;
}

void ObjSpriteList2D::SetDestRect(float left, float top, float right, float bottom)
{
    dstRectLeft_ = left;
    dstRectTop_ = top;
    dstRectRight_ = right;
    dstRectBottom_ = bottom;
}

void ObjSpriteList2D::SetDestCenter()
{
    /* テクスチャが存在しなくても実行できるが、弾幕風の実装に合わせる */
    if (GetD3DTexture())
    {
        float hw = (srcRectRight_ - srcRectLeft_) / 2.0;
        float hh = (srcRectBottom_ - srcRectTop_) / 2.0;
        SetDestRect(-hw, -hh, hw, hh);
    }
}

void ObjSpriteList2D::AddVertex()
{
    // 頂点を6つ生成する
    // 並びは
    // 0-2 3
    // |/ /|
    // 1 4-5

    // 座標変換
    D3DXMATRIX mat = CreateScaleRotTransMatrix(GetX(), GetY(), GetZ(), GetAngleX(), GetAngleY(), GetAngleZ(), GetScaleX(), GetScaleY(), GetScaleZ());
    D3DXMATRIX vecs;
    // 1行目:左上
    // 2行目:左下
    // 3行目:右上
    // 4行目:右下
    vecs._11 = vecs._21 = dstRectLeft_;
    vecs._12 = vecs._32 = dstRectTop_;
    vecs._31 = vecs._41 = dstRectRight_;
    vecs._22 = vecs._42 = dstRectBottom_;
    vecs._13 = vecs._23 = vecs._33 = vecs._43 = 0; // z = 0
    vecs._14 = vecs._24 = vecs._34 = vecs._44 = 1; // w = 1
    auto result = vecs * mat;
    float ul = 0;
    float vt = 0;
    float ur = 0;
    float vb = 0;
    auto texture = GetD3DTexture();
    if (texture)
    {
        int texWidth = GetD3DTextureWidth(texture);
        int texHeight = GetD3DTextureHeight(texture);
        ul = srcRectLeft_ / texWidth;
        vt = srcRectTop_ / texHeight;
        ur = srcRectRight_ / texWidth;
        vb = srcRectBottom_ / texHeight;
    }
    D3DCOLOR color = GetD3DCOLOR();
    Vertex v0(result._11, result._12, 0, color, ul, vt);
    Vertex v1(result._21, result._22, 0, color, ul, vb);
    Vertex v2(result._31, result._32, 0, color, ur, vt);
    Vertex& v3 = v2;
    Vertex& v4 = v1;
    Vertex v5(result._41, result._42, 0, color, ur, vb);
    vertices_.push_back(v0);
    vertices_.push_back(v1);
    vertices_.push_back(v2);
    vertices_.push_back(v3);
    vertices_.push_back(v4);
    vertices_.push_back(v5);
}

void ObjSpriteList2D::CloseVertex()
{
    SetPosition(0, 0, 0);
    SetAngleXYZ(0, 0, 0);
    SetScaleXYZ(1, 1, 1);
    isVertexClosed_ = true;
}

void ObjSpriteList2D::ClearVerexCount()
{
    SetVertexCount(0);
    isVertexClosed_ = false;
}

ObjPrim3D::ObjPrim3D(const std::shared_ptr<Package>& state) :
    ObjPrim(state),
    billboardEnable_(false)
{
    SetType(OBJ_PRIMITIVE_3D);
}

void ObjPrim3D::Render(const std::shared_ptr<Renderer>& renderer)
{
    D3DXMATRIX world = CreateScaleRotTransMatrix(GetX(), GetY(), GetZ(), GetAngleX(), GetAngleY(), GetAngleZ(), GetScaleX(), GetScaleY(), GetScaleZ());
    renderer->RenderPrim3D(GetD3DPrimitiveType(), vertices_.size(), vertices_.data(), GetD3DTexture(), GetBlendType(), world, GetAppliedShader(), IsZWriteEnabled(), IsZTestEnabled(), IsFogEnabled(), IsBillboardEnabled());
}

bool ObjPrim3D::IsBillboardEnabled() const
{
    return billboardEnable_;
}

ObjSprite3D::ObjSprite3D(const std::shared_ptr<Package>& state) :
    ObjPrim3D(state)
{
    SetType(OBJ_SPRITE_3D);
    SetPrimitiveType(PRIMITIVE_TRIANGLESTRIP);
    SetVertexCount(4);
}

void ObjSprite3D::SetSourceRect(float left, float top, float right, float bottom)
{
    // NOTE : Sprite2Dとは頂点の順番が違う
    SetVertexUVT(0, left, top);
    SetVertexUVT(1, left, bottom);
    SetVertexUVT(2, right, top);
    SetVertexUVT(3, right, bottom);
}

void ObjSprite3D::SetDestRect(float left, float top, float right, float bottom)
{
    // 弾幕風の仕様で-0.5
    left -= 0.5f;
    top -= 0.5f;
    right -= 0.5f;
    bottom -= 0.5f;
    // NOTE : Sprite2Dとは頂点の順番が違う
    SetVertexPosition(0, left, top, 0);
    SetVertexPosition(1, left, bottom, 0);
    SetVertexPosition(2, right, top, 0);
    SetVertexPosition(3, right, bottom, 0);
}

void ObjSprite3D::SetSourceDestRect(float left, float top, float right, float bottom)
{
    SetSourceRect(left, top, right, bottom);
    float hw = (right - left) / 2;
    float hh = (bottom - top) / 2;
    SetDestRect(-hw, -hh, hw, hh);
}

void ObjSprite3D::SetBillboard(bool enable)
{
    billboardEnable_ = enable;
}
}