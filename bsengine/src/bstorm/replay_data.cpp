#include <bstorm/replay_data.hpp>

#include <bstorm/dnh_value.hpp>
#include <bstorm/fps_counter.hpp>
#include <bstorm/util.hpp>
#include <bstorm/logger.hpp>
#include <bstorm/log_error.hpp>

#include <fstream>

namespace
{
const char REPLAY_HEADER[] = "BSTORM_REPLAY";
}

namespace bstorm
{

ReplayData::ReplayData()
{
    // ���v���C���̏����l��ݒ�
    data_.CreateCommonDataArea(ReplayInfoAreaName);
    data_.SetAreaCommonData(ReplayInfoAreaName, FilePathInfoKey, std::make_unique<DnhArray>());
    data_.SetAreaCommonData(ReplayInfoAreaName, DateTimeInfoKey, std::make_unique<DnhArray>());
    data_.SetAreaCommonData(ReplayInfoAreaName, UserNameInfoKey, std::make_unique<DnhArray>());
    data_.SetAreaCommonData(ReplayInfoAreaName, TotalScoreInfoKey, std::make_unique<DnhReal>(0.0));
    data_.SetAreaCommonData(ReplayInfoAreaName, FpsAverageInfoKey, std::make_unique<DnhReal>(0.0));
    data_.SetAreaCommonData(ReplayInfoAreaName, PlayerNameInfoKey, std::make_unique<DnhArray>());
    data_.SetAreaCommonData(ReplayInfoAreaName, StageIndexListInfoKey, std::make_unique<DnhArray>());
    data_.SetAreaCommonData(ReplayInfoAreaName, CommentInfoKey, std::make_unique<DnhArray>());
}

void ReplayData::Load(const std::wstring & filePath) noexcept(false)
{
    std::fstream fstream;
    fstream.open(filePath, std::ios::in | std::ios::binary);
    if (!fstream.is_open())
    {
        throw cant_open_replay_file(filePath);
    }

    // �w�b�_�̌���
    constexpr size_t headerSize = sizeof(REPLAY_HEADER) - 1;
    std::string header(headerSize, '\0');
    fstream.read(&header[0], headerSize);
    if (header != std::string(REPLAY_HEADER))
    {
        throw illegal_replay_format(filePath);
    }

    CommonDataDB tmp;
    try
    {
        // ���v���C���G���A�����[�h
        tmp.LoadCommonDataArea(ReplayInfoAreaName, fstream);
        const auto& areaNameList = tmp.GetAreaCommonData(ReplayInfoAreaName, AreaNameListInfoKey, DnhValue::Nil());
        if (auto areaNameListArr = dynamic_cast<DnhArray*>(areaNameList.get()))
        {
            // ���v���C���G���A�ȊO�̃G���A�����[�h
            for (int i = 0; i < areaNameListArr->GetSize(); ++i)
            {
                const auto areaName = areaNameListArr->Index(i)->ToString();
                if (areaName != ReplayInfoAreaName)
                {
                    tmp.LoadCommonDataArea(areaName, fstream);
                }
            }
        }
    } catch (const Log&)
    {
        throw illegal_replay_format(filePath);
    }
    data_ = std::move(tmp);
}

const std::unique_ptr<DnhValue>& ReplayData::GetReplayInfo(const CommonDataDB::DataKey & infoKey) const
{
    return data_.GetAreaCommonData(ReplayInfoAreaName, infoKey, DnhValue::Nil());
}

std::unique_ptr<DnhValue> ReplayData::GetStageInfoList(const CommonDataDB::DataKey & infoKey) const
{
    std::unique_ptr<DnhArray> stageInfoList = std::make_unique<DnhArray>();
    const auto& indexList = GetReplayInfo(StageIndexListInfoKey);
    if (const auto indexListArr = dynamic_cast<DnhArray*>(indexList.get()))
    {
        stageInfoList->Reserve(indexListArr->GetSize());
        for (int i = 0; i < indexListArr->GetSize(); ++i)
        {
            StageIndex stageIndex = indexListArr->Index(i)->ToInt();
            const auto stageInfoAreaName = StageInfoAreaName(stageIndex);
            stageInfoList->PushBack(data_.GetAreaCommonData(stageInfoAreaName, infoKey, DnhValue::Nil())->Clone());
        }
    }
    return std::move(stageInfoList);
}

float ReplayData::GetFps(StageIndex stageIdx, FrameCount stageElapsedFrame) const
{
    const auto stageInfoAreaName = StageInfoAreaName(stageIdx);
    const auto& fpsList = data_.GetAreaCommonData(stageInfoAreaName, StageFpsListInfoKey, DnhValue::Nil());
    if (auto fpsListArr = dynamic_cast<DnhArray*>(fpsList.get()))
    {
        return (float)fpsListArr->Index(stageElapsedFrame)->ToNum();
    }
    return 0.0f;
}

std::unordered_map<VirtualKey, KeyState> ReplayData::GetVirtualKeyStates(StageIndex stageIdx, FrameCount stageElapsedFrame) const
{
    std::unordered_map<VirtualKey, KeyState> ret;
    const auto stageInfoAreaName = StageInfoAreaName(stageIdx);
    {
        // �S�t���[���̓��͏��
        const auto& vkStateList = data_.GetAreaCommonData(stageInfoAreaName, StageVirtualKeyStateListInfoKey, DnhValue::Nil());
        if (const auto vkStateListArr = dynamic_cast<DnhArray*>(vkStateList.get()))
        {
            // ���̃t���[���̓��͏��
            const auto& vkStates = vkStateListArr->Index(stageElapsedFrame);
            if (const auto vkStatesArr = dynamic_cast<DnhArray*>(vkStates.get()))
            {
                for (int i = 0; i < vkStatesArr->GetSize(); i += 2)
                {
                    VirtualKey vk = vkStatesArr->Index(i)->ToInt();
                    KeyState s = vkStatesArr->Index(i + 1)->ToInt();
                    ret[vk] = s;
                }
            }
        }
    }
    return ret;
}

void ReplayData::Save(const std::wstring & filePath, const std::wstring & userName, GameScore totalScore, const std::wstring & playerName) noexcept(false)
{
    const auto uniqPath = GetCanonicalPath(filePath);

    // �e�탊�v���C���ۑ�
    data_.SetAreaCommonData(ReplayInfoAreaName, FilePathInfoKey, std::make_unique<DnhArray>(uniqPath));
    data_.SetAreaCommonData(ReplayInfoAreaName, DateTimeInfoKey, std::make_unique<DnhArray>());
    data_.SetAreaCommonData(ReplayInfoAreaName, UserNameInfoKey, std::make_unique<DnhArray>(userName));
    data_.SetAreaCommonData(ReplayInfoAreaName, TotalScoreInfoKey, std::make_unique<DnhReal>((double)totalScore));
    {
        // fps����
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
                    if (const auto fpsListArr = dynamic_cast<DnhArray*>(data_.GetAreaCommonData(stageInfoAreaName, StageFpsListInfoKey, DnhValue::Nil()).get()))
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

    // �G���A�ꗗ�ۑ�
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
        // �t�@�C���ɕۑ�

        // �f�B���N�g�����Ȃ�������쐬
        MakeDirectoryP(GetParentPath(uniqPath));
        std::ofstream fstream;
        fstream.open(uniqPath, std::ios::out | std::ios::binary);
        if (!fstream.good())
        {
            throw failed_to_save_replay_file(filePath);
        }

        // �w�b�_��ݒ�
        fstream.write(REPLAY_HEADER, sizeof(REPLAY_HEADER) - 1);

        try
        {
            // ���v���C���G���A��ۑ�
            data_.SaveCommonDataArea(ReplayInfoAreaName, fstream);
            // ���v���C���G���A�ȊO��ۑ�
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

void ReplayData::StartRecordingStageInfo(StageIndex stageIdx, GameScore stageStartScore, RandValue randSeed, const std::shared_ptr<TimePoint>& startTime)
{
    const auto stageInfoAreaName = StageInfoAreaName(stageIdx);

    // �X�e�[�W���p�̋��ʃf�[�^�G���A�쐬
    data_.CreateCommonDataArea(stageInfoAreaName);

    // �X�e�[�W�J�n���X�R�A���X�e�[�W���ɒǉ�
    data_.SetAreaCommonData(stageInfoAreaName, StageStartScoreInfoKey, std::make_unique<DnhReal>((double)stageStartScore));

    // �����̃V�[�h�l��ۑ�
    data_.SetAreaCommonData(stageInfoAreaName, StageStartRandSeedInfoKey, std::make_unique<DnhReal>((double)randSeed));

    // �X�e�[�W�J�n���Ԃ�ۑ�
    data_.SetAreaCommonData(stageInfoAreaName, StageStartTimeInfoKey, std::make_unique<DnhReal>(startTime->GetTimeMilliSec()));

    // ���̑������l��ݒ�
    data_.SetAreaCommonData(stageInfoAreaName, StageLastScoreInfoKey, std::make_unique<DnhReal>(0.0));
    data_.SetAreaCommonData(stageInfoAreaName, StageFpsSumInfoKey, std::make_unique<DnhReal>(0.0));
    data_.SetAreaCommonData(stageInfoAreaName, StageFpsListInfoKey, std::make_unique<DnhArray>());
    data_.SetAreaCommonData(stageInfoAreaName, StageVirtualKeyStateListInfoKey, std::make_unique<DnhArray>());
    data_.SetAreaCommonData(stageInfoAreaName, StageEndTimeInfoKey, std::make_unique<DnhReal>(0.0));
    data_.SetAreaCommonData(stageInfoAreaName, StagePauseCountInfoKey, std::make_unique<DnhReal>(0.0));
}

void ReplayData::EndRecordingStageInfo(StageIndex stageIdx, GameScore stageLastScore, const std::shared_ptr<TimePoint>& endTime) noexcept(true)
{
    const auto stageInfoAreaName = StageInfoAreaName(stageIdx);
    {
        // ���v���C���̃X�e�[�W�C���f�b�N�X���X�g�ɒǉ�
        const auto& indexList = GetReplayInfo(StageIndexListInfoKey);
        if (auto indexListArr = dynamic_cast<DnhArray*>(indexList.get()))
        {
            indexListArr->PushBack(std::make_unique<DnhReal>((double)stageIdx));
        }
    }

    // �X�e�[�W�I�����X�R�A���X�e�[�W���ɒǉ�
    data_.SetAreaCommonData(stageInfoAreaName, StageLastScoreInfoKey, std::make_unique<DnhReal>((double)stageLastScore));

    // �X�e�[�W�I�����Ԃ�ۑ�
    data_.SetAreaCommonData(stageInfoAreaName, StageEndTimeInfoKey, std::make_unique<DnhReal>(endTime->GetTimeMilliSec()));
}

void ReplayData::RecordFrameStageInfo(StageIndex stageIdx, float fps, const std::unordered_map<VirtualKey, KeyState>& keyStates)
{
    const auto stageInfoAreaName = StageInfoAreaName(stageIdx);
    // �L�[�����L�^
    const auto& vkStateList = data_.GetAreaCommonData(stageInfoAreaName, StageVirtualKeyStateListInfoKey, DnhValue::Nil());
    if (const auto vkStateListArr = dynamic_cast<DnhArray*>(vkStateList.get()))
    {
        auto vkStates = std::make_unique<DnhArray>(keyStates.size() * 2);
        for (const auto& entry : keyStates)
        {
            vkStates->PushBack(std::make_unique<DnhReal>((double)(entry.first)));
            vkStates->PushBack(std::make_unique<DnhReal>((double)(entry.second)));
        }
        vkStateListArr->PushBack(std::move(vkStates));
    }

    // FPS���L�^
    {
        const auto& fpsList = data_.GetAreaCommonData(stageInfoAreaName, StageFpsListInfoKey, DnhValue::Nil());
        if (auto fpsListArr = dynamic_cast<DnhArray*>(fpsList.get()))
        {
            fpsListArr->PushBack(std::make_unique<DnhReal>((double)fps));
        }
    }

    // FPS�̘a�ɉ��Z
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

void ReplayData::SetComment(const std::wstring & comment)
{
    data_.SetAreaCommonData(ReplayInfoAreaName, CommentInfoKey, std::make_unique<DnhArray>(comment));
}

bool ReplayData::LoadCommonDataArea(StageIndex stageIdx, const CommonDataDB::DataAreaName & areaName, CommonDataDB & dst) const
{
    const auto stageCommonDataAreaName = StageCommonDataAreaName(stageIdx, areaName);
    return dst.CopyCommonDataAreaFromOtherDB(areaName, stageCommonDataAreaName, data_);
}

std::wstring ReplayData::StageInfoAreaName(StageIndex stageIdx)
{
    return L"STAGE_" + std::to_wstring(stageIdx);
}

std::wstring ReplayData::StageCommonDataAreaName(StageIndex stageIdx, const CommonDataDB::DataAreaName& areaName)
{
    return StageInfoAreaName(stageIdx) + L"_" + areaName;
}
}
