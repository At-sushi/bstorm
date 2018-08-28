#include <bstorm/shot_data.hpp>

#include <bstorm/dnh_const.hpp>
#include <bstorm/parser.hpp>
#include <bstorm/file_util.hpp>
#include <bstorm/texture.hpp>
#include <bstorm/logger.hpp>

#include <algorithm>

namespace bstorm
{
ShotData::ShotData() :
    id(ID_INVALID),
    rect(0, 0, 0, 0),
    render(BLEND_ALPHA),
    alpha(0xff),
    delayRect(0, 0, 0, 0),
    useDelayRect(false),
    delayColor(0, 0, 0),
    useDelayColor(false),
    delayRender(BLEND_ADD_ARGB),
    angularVelocity(0),
    useAngularVelocityRand(false),
    angularVelocityRandMin(0),
    angularVelocityRandMax(0),
    fixedAngle(false)
{
}

ShotDataTable::ShotDataTable(Type type, const std::shared_ptr<TextureStore>& textureStore, const std::shared_ptr<FileLoader>& fileLoader) :
    type_(type),
    textureStore_(textureStore),
    fileLoader_(fileLoader)
{
}

ShotDataTable::~ShotDataTable()
{
}

void ShotDataTable::Add(const std::shared_ptr<ShotData>& data)
{
    if (data->texture)
    {
        table_.emplace(data->id, data);
    }
}

const char * ShotDataTable::GetTypeName(Type type)
{
    if (type == Type::PLAYER) return "player";
    if (type == Type::ENEMY) return "enemy";
    return "unknown";
}

static Log::Param::Tag getElemTag(ShotDataTable::Type type)
{
    if (type == ShotDataTable::Type::PLAYER) return Log::Param::Tag::PLAYER_SHOT_DATA;
    if (type == ShotDataTable::Type::ENEMY) return Log::Param::Tag::ENEMY_SHOT_DATA;
    return Log::Param::Tag::TEXT;
}

void ShotDataTable::Load(const std::wstring & path, const std::shared_ptr<SourcePos>& srcPos)
{
    if (IsLoaded(path))
    {
        Logger::WriteLog(std::move(
            Log(Log::Level::LV_WARN)
            .SetMessage(std::string(GetTypeName(type_)) + " shot data already loaded.")
            .SetParam(Log::Param(getElemTag(type_), path))
            .AddSourcePos(srcPos)));
    } else
    {
        Reload(path, srcPos);
    }
}

void ShotDataTable::Reload(const std::wstring & path, const std::shared_ptr<SourcePos>& srcPos)
{
    std::wstring uniqPath = GetCanonicalPath(path);
    auto userShotData = ParseUserShotData(uniqPath, fileLoader_);
    auto& texture = textureStore_->Load(userShotData->imagePath);
    for (auto& entry : userShotData->dataMap)
    {
        auto& data = entry.second;
        if (!data.useDelayRect)
        {
            data.delayRect = userShotData->delayRect;
        }

        if (!data.useDelayColor)
        {
            data.delayColor = userShotData->delayColor;
        }
        if (data.collisions.empty())
        {
            float width = std::abs(data.rect.right - data.rect.left);
            float height = std::abs(data.rect.bottom - data.rect.top);
            float r = std::max(std::max(width, height) / 1.5f - 1.5f, 3.0f) / 4.0f;
            data.collisions.push_back({ r, 0.0f, 0.0f });
        }
        data.texture = texture;
        table_.emplace(data.id, std::make_shared<ShotData>(data));
    }
    alreadyLoadedPaths_.insert(uniqPath);
    Logger::WriteLog(std::move(
        Log(Log::Level::LV_INFO)
        .SetMessage("load " + std::string(GetTypeName(type_)) + " shot data.")
        .SetParam(Log::Param(getElemTag(type_), path))
        .AddSourcePos(srcPos)));
}

bool ShotDataTable::IsLoaded(const std::wstring & path) const
{
    return alreadyLoadedPaths_.count(GetCanonicalPath(path)) != 0;
}

NullableSharedPtr<ShotData> ShotDataTable::Get(int id) const
{
    auto it = table_.find(id);
    if (it != table_.end())
    {
        return it->second;
    }
    return nullptr;
}

ShotDataTable::Type ShotDataTable::GetType() const
{
    return type_;
}
}