#include <bstorm/replay_data.hpp>

#include <bstorm/dnh_const.hpp>
#include <bstorm/dnh_value.hpp>
#include <bstorm/time_point.hpp>
#include <bstorm/file_util.hpp>
#include <bstorm/string_util.hpp>
#include <bstorm/logger.hpp>
#include <bstorm/log_error.hpp>

#include <fstream>
#include <ctime>

namespace
{
const char REPLAY_HEADER[] = "BSTORM_REPLAY";
}

namespace bstorm
{

ReplayData::ReplayData()
{
    // リプレイ情報の初期値を設定
    data_.ClearAllCommonDataArea();
    data_.CreateCommonDataArea(ReplayInfoAreaName);
    data_.SetAreaCommonData(ReplayInfoAreaName, FilePathInfoKey, std::make_unique<DnhArray>(L""));
    data_.SetAreaCommonData(ReplayInfoAreaName, DateTimeInfoKey, std::make_unique<DnhArray>(L""));
    data_.SetAreaCommonData(ReplayInfoAreaName, UserNameInfoKey, std::make_unique<DnhArray>(L""));
    data_.SetAreaCommonData(ReplayInfoAreaName, TotalScoreInfoKey, std::make_unique<DnhReal>(0.0));
    data_.SetAreaCommonData(ReplayInfoAreaName, FpsAverageInfoKey, std::make_unique<DnhReal>(0.0));
    data_.SetAreaCommonData(ReplayInfoAreaName, PlayerNameInfoKey, std::make_unique<DnhArray>(L""));
    data_.SetAreaCommonData(ReplayInfoAreaName, StageIndexListInfoKey, std::make_unique<DnhArray>());
    data_.SetAreaCommonData(ReplayInfoAreaName, CommentInfoKey, std::make_unique<DnhArray>(L""));
}

ReplayData::ReplayData(const std::wstring & filePath)
{
    std::fstream fstream;
    fstream.open(filePath, std::ios::in | std::ios::binary);
    if (!fstream.is_open())
    {
        throw cant_open_replay_file(filePath);
    }

    // ヘッダの検査
    constexpr size_t headerSize = sizeof(REPLAY_HEADER) - 1;
    std::string header(headerSize, '\0');
    fstream.read(&header[0], headerSize);
    if (header != std::string(REPLAY_HEADER))
    {
        throw illegal_replay_format(filePath);
    }

    try
    {
        data_.ClearAllCommonDataArea();
        // リプレイ情報エリアをロード
        data_.LoadCommonDataArea(ReplayInfoAreaName, fstream);
        const auto& areaNameList = data_.GetAreaCommonData(ReplayInfoAreaName, AreaNameListInfoKey, DnhValue::Nil());
        if (auto areaNameListArr = dynamic_cast<DnhArray*>(areaNameList.get()))
        {
            // リプレイ情報エリア以外のエリアをロード
            for (int i = 0; i < areaNameListArr->GetSize(); ++i)
            {
                const auto areaName = areaNameListArr->Index(i)->ToString();
                if (areaName != ReplayInfoAreaName)
                {
                    data_.LoadCommonDataArea(areaName, fstream);
                }
            }
        }
    } catch (const Log&)
    {
        throw illegal_replay_format(filePath);
    }
}

const std::unique_ptr<DnhValue>& ReplayData::GetReplayInfo(const CommonDataDB::DataKey & infoKey) const
{
    return data_.GetAreaCommonData(ReplayInfoAreaName, infoKey, DnhValue::Nil());
}

const std::unique_ptr<DnhValue>& ReplayData::GetStageInfo(StageIndex stageIdx, const std::wstring & infoKey) const
{
    return data_.GetAreaCommonData(StageInfoAreaName(stageIdx), infoKey, DnhValue::Nil());
}

float ReplayData::GetFps(StageIndex stageIdx, int stageElapsedFrame) const
{
    const auto stageInfoAreaName = StageInfoAreaName(stageIdx);
    const auto& fpsList = data_.GetAreaCommonData(stageInfoAreaName, StageFpsListInfoKey, DnhValue::Nil());
    if (auto fpsListArr = dynamic_cast<DnhRealArray*>(fpsList.get()))
    {
        return (float)fpsListArr->Index(stageElapsedFrame);
    }
    return 0.0f;
}

std::unordered_map<VirtualKey, KeyState> ReplayData::GetVirtualKeyStates(StageIndex stageIdx, int stageElapsedFrame) const
{
    std::unordered_map<VirtualKey, KeyState> ret;
    const auto stageInfoAreaName = StageInfoAreaName(stageIdx);
    {
        // 全フレームの入力情報
        const auto& vkStateList = data_.GetAreaCommonData(stageInfoAreaName, StageVirtualKeyStateListInfoKey, DnhValue::Nil());
        if (const auto vkStateListArr = dynamic_cast<DnhArray*>(vkStateList.get()))
        {
            // そのフレームの入力情報
            const auto& vkStates = vkStateListArr->Index(stageElapsedFrame);
            if (const auto vkStatesArr = dynamic_cast<DnhUInt16Array*>(vkStates.get()))
            {
                for (int i = 0; i < vkStatesArr->GetSize(); i += 2)
                {
                    VirtualKey vk = vkStatesArr->Index(i);
                    KeyState s = vkStatesArr->Index(i + 1);
                    ret[vk] = s;
                }
            }
        }
    }
    return ret;
}

void ReplayData::Save(const std::wstring & filePath, const std::wstring & userName, PlayerScore totalScore, const std::wstring & playerName) noexcept(false)
{
    const auto uniqPath = GetCanonicalPath(filePath);

    // 各種リプレイ情報保存
    data_.SetAreaCommonData(ReplayInfoAreaName, FilePathInfoKey, std::make_unique<DnhArray>(uniqPath));
    {
        // 保存日時を設定
        time_t now = std::time(nullptr);
        struct tm* local = std::localtime(&now);
        std::string buf(16, '\0');
        sprintf(&buf[0], "%04d/%02d/%02d %02d:%02d", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min);
        data_.SetAreaCommonData(ReplayInfoAreaName, DateTimeInfoKey, std::make_unique<DnhArray>(ToUnicode(buf)));
    }
    data_.SetAreaCommonData(ReplayInfoAreaName, UserNameInfoKey, std::make_unique<DnhArray>(userName));
    data_.SetAreaCommonData(ReplayInfoAreaName, TotalScoreInfoKey, std::make_unique<DnhReal>((double)totalScore));
    {
        // fps平均
        if (const auto& indexList = GetReplayInfo(StageIndexListInfoKey))
        {
            if (const auto indexListArr = dynamic_cast<DnhArray*>(indexList.get()))
            {
                int fpsCnt = 0;
                double fpsSum = 0.0;
                for (int i = 0; i < indexListArr->GetSize(); ++i)
                {
                    StageIndex stageIdx = indexListArr->Index(i)->ToInt();
                    const auto stageInfoAreaName = StageInfoAreaName(stageIdx);
                    if (const auto fpsListArr = dynamic_cast<DnhRealArray*>(data_.GetAreaCommonData(stageInfoAreaName, StageFpsListInfoKey, DnhValue::Nil()).get()))
                    {
                        fpsSum += data_.GetAreaCommonData(stageInfoAreaName, StageFpsSumInfoKey, DnhValue::Nil())->ToNum();
                        fpsCnt += fpsListArr->GetSize();
                    }
                }
                double fpsAverage = fpsSum / fpsCnt;
                data_.SetAreaCommonData(ReplayInfoAreaName, FpsAverageInfoKey, std::make_unique<DnhReal>(fpsAverage));
            }
        }
    }
    data_.SetAreaCommonData(ReplayInfoAreaName, PlayerNameInfoKey, std::make_unique<DnhArray>(playerName));

    // エリア一覧保存
    {
        auto areaNameList = data_.GetCommonDataAreaKeyList();
        auto arr = std::make_unique<DnhArray>(areaNameList.size() - 1);
        for (const auto& areaName : areaNameList)
        {
            if (areaName != ReplayInfoAreaName)
            {
                arr->PushBack(std::make_unique<DnhArray>(areaName));
            }
        }
        data_.SetAreaCommonData(ReplayInfoAreaName, AreaNameListInfoKey, std::move(arr));
    }

    {
        // ファイルに保存

        // ディレクトリがなかったら作成
        MakeDirectoryP(GetParentPath(uniqPath));
        std::ofstream fstream;
        fstream.open(uniqPath, std::ios::out | std::ios::binary);
        if (!fstream.good())
        {
            throw failed_to_save_replay_file(filePath);
        }

        // ヘッダを設定
        fstream.write(REPLAY_HEADER, sizeof(REPLAY_HEADER) - 1);

        try
        {
            // リプレイ情報エリアを保存
            data_.SaveCommonDataArea(ReplayInfoAreaName, fstream);
            // リプレイ情報エリア以外を保存
            for (const auto& areaName : data_.GetCommonDataAreaKeyList())
            {
                if (areaName != ReplayInfoAreaName)
                {
                    data_.SaveCommonDataArea(areaName, fstream);
                }
            }
        } catch (const Log&)
        {
            throw failed_to_save_replay_file(filePath);
        }
    }
}

void ReplayData::StartRecordingStageInfo(StageIndex stageIdx, PlayerScore stageStartScore, RandSeed randSeed, const std::shared_ptr<TimePoint>& startTime)
{
    const auto stageInfoAreaName = StageInfoAreaName(stageIdx);

    // ステージ情報用の共通データエリア作成
    data_.CreateCommonDataArea(stageInfoAreaName);

    // ステージ開始時スコアをステージ情報に追加
    data_.SetAreaCommonData(stageInfoAreaName, StageStartScoreInfoKey, std::make_unique<DnhReal>((double)stageStartScore));

    // 乱数のシード値を保存
    data_.SetAreaCommonData(stageInfoAreaName, StageStartRandSeedInfoKey, std::make_unique<DnhReal>((double)randSeed));

    // ステージ開始時間を保存
    data_.SetAreaCommonData(stageInfoAreaName, StageStartTimeInfoKey, std::make_unique<DnhReal>(startTime->GetTimeMilliSec()));

    // その他初期値を設定
    data_.SetAreaCommonData(stageInfoAreaName, StageLastScoreInfoKey, std::make_unique<DnhReal>(0.0));
    data_.SetAreaCommonData(stageInfoAreaName, StageFpsSumInfoKey, std::make_unique<DnhReal>(0.0));
    data_.SetAreaCommonData(stageInfoAreaName, StageFpsListInfoKey, std::make_unique<DnhRealArray>());
    data_.SetAreaCommonData(stageInfoAreaName, StageVirtualKeyStateListInfoKey, std::make_unique<DnhArray>());
    data_.SetAreaCommonData(stageInfoAreaName, StageEndTimeInfoKey, std::make_unique<DnhReal>(0.0));
    data_.SetAreaCommonData(stageInfoAreaName, StagePauseCountInfoKey, std::make_unique<DnhReal>(0.0));
}

void ReplayData::EndRecordingStageInfo(StageIndex stageIdx, PlayerScore stageLastScore, const std::shared_ptr<TimePoint>& endTime) noexcept(true)
{
    const auto stageInfoAreaName = StageInfoAreaName(stageIdx);
    {
        // リプレイ情報のステージインデックスリストに追加
        const auto& indexList = GetReplayInfo(StageIndexListInfoKey);
        if (auto indexListArr = dynamic_cast<DnhArray*>(indexList.get()))
        {
            indexListArr->PushBack(std::make_unique<DnhReal>((double)stageIdx));
        }
    }

    // ステージ終了時スコアをステージ情報に追加
    data_.SetAreaCommonData(stageInfoAreaName, StageLastScoreInfoKey, std::make_unique<DnhReal>((double)stageLastScore));

    // ステージ終了時間を保存
    data_.SetAreaCommonData(stageInfoAreaName, StageEndTimeInfoKey, std::make_unique<DnhReal>(endTime->GetTimeMilliSec()));
}

void ReplayData::RecordFrameStageInfo(StageIndex stageIdx, float fps, const std::unordered_map<VirtualKey, KeyState>& keyStates)
{
    const auto stageInfoAreaName = StageInfoAreaName(stageIdx);
    // キー情報を記録
    const auto& vkStateList = data_.GetAreaCommonData(stageInfoAreaName, StageVirtualKeyStateListInfoKey, DnhValue::Nil());
    if (const auto vkStateListArr = dynamic_cast<DnhArray*>(vkStateList.get()))
    {
        auto vkStates = std::make_unique<DnhUInt16Array>();
        for (const auto& entry : keyStates)
        {
            auto vk = entry.first;
            auto state = entry.second;
            // KEY_FREEの場合は保存しない(容量削減)
            if (state != KEY_FREE)
            {
                vkStates->PushBack(entry.first);
                vkStates->PushBack(entry.second);
            }
        }
        vkStateListArr->PushBack(std::move(vkStates));
    }

    // FPSを記録
    {
        const auto& fpsList = data_.GetAreaCommonData(stageInfoAreaName, StageFpsListInfoKey, DnhValue::Nil());
        if (auto fpsListArr = dynamic_cast<DnhRealArray*>(fpsList.get()))
        {
            fpsListArr->PushBack((double)fps);
        }
    }

    // FPSの和に加算
    double fpsSum = data_.GetAreaCommonData(stageInfoAreaName, StageFpsSumInfoKey, DnhValue::Nil())->ToNum();
    data_.SetAreaCommonData(stageInfoAreaName, StageFpsSumInfoKey, std::make_unique<DnhReal>(fpsSum + fps));
}

void ReplayData::RecordPause(StageIndex stageIdx)
{
    const auto stageInfoAreaName = StageInfoAreaName(stageIdx);
    int pauseCnt = data_.GetAreaCommonData(stageInfoAreaName, StagePauseCountInfoKey, DnhValue::Nil())->ToInt();
    data_.SetAreaCommonData(stageInfoAreaName, StagePauseCountInfoKey, std::make_unique<DnhReal>((double)(pauseCnt + 1)));
}

bool ReplayData::SaveCommonDataArea(StageIndex stageIdx, const CommonDataDB::DataAreaName & areaName, const CommonDataDB & src)
{
    return data_.CopyCommonDataAreaFromOtherDB(StageCommonDataAreaName(stageIdx, areaName), areaName, src);
}

void ReplayData::SetReplayInfo(const CommonDataDB::DataKey & key, std::unique_ptr<DnhValue>&& value)
{
    data_.SetAreaCommonData(ReplayInfoAreaName, key, std::move(value));
}

bool ReplayData::LoadCommonDataArea(StageIndex stageIdx, const CommonDataDB::DataAreaName & areaName, CommonDataDB & dst) const
{
    const auto stageCommonDataAreaName = StageCommonDataAreaName(stageIdx, areaName);
    return dst.CopyCommonDataAreaFromOtherDB(areaName, stageCommonDataAreaName, data_);
}

std::wstring ReplayData::StageInfoAreaName(StageIndex stageIdx)
{
    return L"stage_" + std::to_wstring(stageIdx);
}

std::wstring ReplayData::StageCommonDataAreaName(StageIndex stageIdx, const CommonDataDB::DataAreaName& areaName)
{
    return StageInfoAreaName(stageIdx) + L"_" + areaName;
}
}
