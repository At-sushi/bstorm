#include <deque>

#include <bstorm/obj_render.hpp>

#include <bstorm/dnh_const.hpp>
#include <bstorm/math_util.hpp>
#include <bstorm/shader.hpp>
#include <bstorm/texture.hpp>
#include <bstorm/render_target.hpp>
#include <bstorm/obj_shot.hpp>
#include <bstorm/shot_data.hpp>
#include <bstorm/package.hpp>

namespace bstorm
{
ObjRender::ObjRender(const std::shared_ptr<Package>& package) :
    Obj(package),
    visibleFlag_(true),
    priority_(-1),
    x_(0),
    y_(0),
    z_(0),
    angleX_(0),
    angleY_(0),
    angleZ_(0),
    scaleX_(1),
    scaleY_(1),
    scaleZ_(1),
    alpha_(0xff),
    blendType_(BLEND_ALPHA),
    fogEnable_(true),
    zWriteEnable_(true),
    zTestEnable_(true),
    permitCamera_(true)
{
}

ObjRender::~ObjRender()
{
}

void ObjRender::SetColor(int r, int g, int b)
{
    rgb_ = ColorRGB(r, g, b);
}

void ObjRender::SetAlpha(int a)
{
    alpha_ = constrain(a, 0, 0xff);
}

void ObjRender::SetColorHSV(int h, int s, int v)
{
    h %= 360;
    if (h < 0) h += 360;
    s = constrain(s, 0, 255);
    v = constrain(v, 0, 255);
    int r, g, b;
    if (s == 0)
    {
        r = g = b = v;
    } else
    {
        int hi = floor((float)h / 60.0f);
        float f = (float)h / 60.0f - hi;
        int m = (float)v * (1.0f - (float)s / 255.0f);
        int n = (float)v * (1.0f - f * (float)s / 255.0f);
        int k = (float)v * (1.0f - (1.0f - f) * (float)s / 255.0f);
        switch (hi)
        {
            default:
            case 0:
                r = v;
                g = k;
                b = m;
                break;
            case 1:
                r = n;
                g = v;
                b = m;
                break;
            case 2:
                r = m;
                g = v;
                b = k;
                break;
            case 3:
                r = m;
                g = n;
                b = v;
                break;
            case 4:
                r = k;
                g = m;
                b = v;
                break;
            case 5:
                r = v;
                g = m;
                b = n;
                break;
        }
    }
    SetColor(r, g, b);
}

D3DCOLOR ObjRender::GetD3DCOLOR() const
{
    return rgb_.ToD3DCOLOR(alpha_);
}

void ObjRender::SetShader(const std::shared_ptr<Shader>& s)
{
    shader_ = s;
}

void ObjRender::SetShaderO(const std::shared_ptr<ObjRender>& obj)
{
    shader_ = obj->shader_;
}

void ObjRender::ResetShader()
{
    shader_.reset();
}

void ObjRender::SetShaderTechnique(const std::string & name)
{
    if (shader_)
    {
        shader_->getEffect()->SetTechnique(name.c_str());
    }
}

void ObjRender::SetShaderVector(const std::string & name, float x, float y, float z, float w)
{
    if (shader_)
    {
        D3DXVECTOR4 vec4(x, y, z, w);
        shader_->getEffect()->SetVector(name.c_str(), &vec4);
    }
}

void ObjRender::SetShaderFloat(const std::string & name, float f)
{
    if (shader_)
    {
        shader_->getEffect()->SetFloat(name.c_str(), f);
    }
}

void ObjRender::SetShaderFloatArray(const std::string & name, const std::vector<float>& fs)
{
    if (shader_)
    {
        shader_->getEffect()->SetFloatArray(name.c_str(), &fs[0], fs.size());
    }
}

void ObjRender::SetShaderTexture(const std::string & name, const std::shared_ptr<Texture>& texture)
{
    if (shader_ && texture)
    {
        shader_->SetTexture(name, texture->GetTexture());
    }
}

void ObjRender::SetShaderTexture(const std::string & name, const std::shared_ptr<RenderTarget>& renderTarget)
{
    if (shader_ && renderTarget)
    {
        shader_->SetTexture(name, renderTarget->GetTexture());
    }
}

NullableSharedPtr<Shader> ObjRender::GetAppliedShader() const
{
    if (shader_) return shader_;
    if (auto package = GetPackage().lock())
    {
        return package->GetLayerShader(priority_);
    }
    return nullptr;
}

ObjectLayerList::ObjectLayerList() :
    shotRenderPriority_(DEFAULT_SHOT_RENDER_PRIORITY),
    itemRenderPriority_(DEFAULT_ITEM_RENDER_PRIORITY),
    cameraFocusPermitRenderPriority_(DEFAULT_CAMERA_FOCUS_PERMIT_RENDER_PRIORITY),
    stgFrameRenderPriorityMin_(DEFAULT_STG_FRAME_RENDER_PRIORITY_MIN),
    stgFrameRenderPriorityMax_(DEFAULT_STG_FRAME_RENDER_PRIORITY_MAX),
    invalidRenderPriorityMin_(-1),
    invalidRenderPriorityMax_(-1)
{
}

ObjectLayerList::~ObjectLayerList() {}

void ObjectLayerList::Remove(const std::shared_ptr<ObjRender>& obj)
{
    if (obj->priority_ >= 0)
    {
        obj->posInLayer_->reset();
        obj->priority_ = -1;
    }
}

void ObjectLayerList::SetRenderPriority(const std::shared_ptr<ObjRender>& obj, int p)
{
    if (p < 0 || p > MAX_RENDER_PRIORITY) return;
    // 現在のレイヤーから削除
    Remove(obj);
    // 新しいレイヤーに追加
    auto& layer = layers_.at(p);
    obj->posInLayer_ = layer.insert(layer.end(), obj);
    obj->priority_ = p;
}

void ObjectLayerList::RenderLayer(int priority, bool ignoreStgSceneObj, bool checkVisibleFlag, const std::shared_ptr<Renderer>& renderer)
{
    if (priority < 0 || priority > MAX_RENDER_PRIORITY) return;

    auto& layer = layers_.at(priority);

    // バケツソートする
    std::deque<std::shared_ptr<ObjRender>> addShots;
    std::deque<std::shared_ptr<ObjRender>> mulShots;
    std::deque<std::shared_ptr<ObjRender>> subShots;
    std::deque<std::shared_ptr<ObjRender>> invShots;
    std::deque<std::shared_ptr<ObjRender>> alphaShots;
    std::deque<std::shared_ptr<ObjRender>> others;

    auto it = layer.begin();
    while (it != layer.end())
    {
        auto& obj = it->lock();
        if (!obj)
        {
            //解放済み
            it = layer.erase(it);
            continue;
        }
        ++it;

        // 終了状態 or 非表示状態
        if (obj->IsDead() || (checkVisibleFlag && !obj->IsVisible())) { continue; }

        // StgSceneのオブジェクトを描画するかどうか
        if (ignoreStgSceneObj && obj->IsStgSceneObject()) { continue; }

        auto shot = std::dynamic_pointer_cast<ObjShot>(obj);

        // 非Shot
        if (!shot)
        {
            others.push_back(obj);
            continue;
        }

        if (const auto& shotData = shot->GetShotData())
        {
            int blendType;
            if (!shot->IsDelay())
            {
                blendType = shot->GetBlendType();
                if (blendType == BLEND_NONE)
                {
                    blendType = shotData->render;
                }
            } else
            {
                blendType = shot->GetSourceBlendType();
                if (blendType == BLEND_NONE)
                {
                    blendType = shotData->delayRender;
                }
            }
            switch (blendType)
            {
                case BLEND_ALPHA:
                    alphaShots.push_back(shot);
                    break;
                case BLEND_ADD_RGB:
                case BLEND_ADD_ARGB:
                    addShots.push_back(shot);
                    break;
                case BLEND_MULTIPLY:
                    mulShots.push_back(shot);
                    break;
                case BLEND_SUBTRACT:
                    subShots.push_back(shot);
                    break;
                case BLEND_INV_DESTRGB:
                    invShots.push_back(shot);
                    break;
            }
        }
    }
    for (auto shot : addShots) { shot->Render(renderer); }
    for (auto shot : mulShots) { shot->Render(renderer); }
    for (auto shot : subShots) { shot->Render(renderer); }
    for (auto shot : invShots) { shot->Render(renderer); }
    for (auto shot : alphaShots) { shot->Render(renderer); }
    for (auto obj : others) { obj->Render(renderer); }
}

void ObjectLayerList::SetLayerShader(int beginPriority, int endPriority, const std::shared_ptr<Shader>& shader)
{
    beginPriority = std::max(0, beginPriority);
    endPriority = std::min(endPriority, MAX_RENDER_PRIORITY);
    for (int p = beginPriority; p <= endPriority; p++)
    {
        layerShaders_[p] = shader;
    }
}

void ObjectLayerList::ResetLayerShader(int beginPriority, int endPriority)
{
    beginPriority = std::max(0, beginPriority);
    endPriority = std::min(endPriority, MAX_RENDER_PRIORITY);
    for (int p = beginPriority; p <= endPriority; p++)
    {
        layerShaders_[p].reset();
    }
}

NullableSharedPtr<Shader> ObjectLayerList::GetLayerShader(int p) const
{
    if (p < 0 || p > MAX_RENDER_PRIORITY)
    {
        return nullptr;
    }
    return layerShaders_.at(p);
}

int ObjectLayerList::GetShotRenderPriority() const
{
    return shotRenderPriority_;
}

void ObjectLayerList::SetShotRenderPriority(int p)
{
    shotRenderPriority_ = p;
}

int ObjectLayerList::GetItemRenderPriority() const
{
    return itemRenderPriority_;
}

void ObjectLayerList::SetItemRenderPriority(int p)
{
    itemRenderPriority_ = p;
}

int ObjectLayerList::GetCameraFocusPermitRenderPriority() const
{
    return cameraFocusPermitRenderPriority_;
}

int ObjectLayerList::GetStgFrameRenderPriorityMin() const
{
    return stgFrameRenderPriorityMin_;
}

void ObjectLayerList::SetStgFrameRenderPriorityMin(int p)
{
    stgFrameRenderPriorityMin_ = p;
}

int ObjectLayerList::GetStgFrameRenderPriorityMax() const
{
    return stgFrameRenderPriorityMax_;
}

void ObjectLayerList::SetStgFrameRenderPriorityMax(int p)
{
    stgFrameRenderPriorityMax_ = p;
}

bool ObjectLayerList::IsInvalidRenderPriority(int p) const
{
    return p <= invalidRenderPriorityMax_ && invalidRenderPriorityMin_ <= p;
}

void ObjectLayerList::SetInvalidRenderPriority(int min, int max)
{
    invalidRenderPriorityMin_ = min;
    invalidRenderPriorityMax_ = max;
}

void ObjectLayerList::ClearInvalidRenderPriority()
{
    invalidRenderPriorityMin_ = invalidRenderPriorityMax_ = -1;
}

ObjShader::ObjShader(const std::shared_ptr<Package>& package) : ObjRender(package)
{
    SetType(OBJ_SHADER);
}
ObjShader::~ObjShader() {}
}