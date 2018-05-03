#pragma once

#include <bstorm/type.hpp>
#include <bstorm/common_data_db.hpp>

#include <string>
#include <memory>
#include <unordered_map>

namespace bstorm
{
class TimePoint;
class ReplayData
{
public:
    ReplayData();

    // =======
    // �Đ��p
    // =======

    void Load(const std::wstring& filePath) noexcept(false);
    // ���v���C�̏����擾����
    const std::unique_ptr<DnhValue>& GetReplayInfo(const CommonDataDB::DataKey& infoKey) const;
    // �S�X�e�[�W�̏����X�e�[�W�̃C���f�b�N�X���X�g���Ɏ擾����
    std::unique_ptr<DnhValue> GetStageInfoList(const CommonDataDB::DataKey& infoKey) const;
    // �v���C����FPS���擾
    float GetFps(StageIndex stageIdx, FrameCount stageElapsedFrame) const;
    // �v���C���̃L�[���͂��擾
    std::unordered_map<VirtualKey, KeyState> GetVirtualKeyStates(StageIndex stageIdx, FrameCount stageElapsedFrame) const;
    // �X�e�[�W���狤�ʃf�[�^�G���A��ǂݏo��
    bool LoadCommonDataArea(StageIndex stageIdx, const CommonDataDB::DataAreaName& areaName, CommonDataDB& dst) const;

    // ======
    // �ۑ��p
    // ======

    void Save(const std::wstring& filePath,
              const std::wstring& userName,
              GameScore totalScore,
              const std::wstring& playerName) noexcept(false);
    // �X�e�[�W���L�^�J�n
    void StartRecordingStageInfo(StageIndex stageIdx, GameScore stageStartScore, RandValue randSeed, const std::shared_ptr<TimePoint>& startTime);
    // �X�e�[�W���L�^�I��
    void EndRecordingStageInfo(StageIndex stageIdx, GameScore stageLastScore, const std::shared_ptr<TimePoint>& endTime) noexcept(true);
    // �t���[�����̃X�e�[�W�����L�^
    void RecordFrameStageInfo(StageIndex stageIdx, float fps, const std::unordered_map<VirtualKey, KeyState>& keyStates);
    // �|�[�Y�������Ƃ��L�^
    void RecordPause(StageIndex stageIdx);
    // �X�e�[�W�ɋ��ʃf�[�^�G���A��ۑ�����
    bool SaveCommonDataArea(StageIndex stageIdx, const CommonDataDB::DataAreaName& areaName, const CommonDataDB& src);
    // �R�����g��ݒ�
    void SetComment(const std::wstring& comment);

    // ========
    // �L�[�ꗗ
    // ========

    // ���v���C���
    static constexpr wchar_t* FilePathInfoKey = L"REPLAY_FILE_PATH";
    static constexpr wchar_t* DateTimeInfoKey = L"DATE_TIME";
    static constexpr wchar_t* UserNameInfoKey = L"USER_NAME";
    static constexpr wchar_t* TotalScoreInfoKey = L"TOTAL_SCORE";
    static constexpr wchar_t* FpsAverageInfoKey = L"FPS_AVERAGE";
    static constexpr wchar_t* PlayerNameInfoKey = L"PLAYER_NAME";
    static constexpr wchar_t* StageIndexListInfoKey = L"STAGE_INDEX_LIST";
    static constexpr wchar_t* CommentInfoKey = L"COMMENT";
    static constexpr wchar_t* AreaNameListInfoKey = L"AREA_NAME_LIST";
    // �X�e�[�W���
    static constexpr wchar_t* StageStartScoreInfoKey = L"START_SCORE";
    static constexpr wchar_t* StageLastScoreInfoKey = L"LAST_SCORE";
    static constexpr wchar_t* StageFpsSumInfoKey = L"TOTAL_FPS_SUM";
    static constexpr wchar_t* StageFpsListInfoKey = L"FPS_LIST";
    static constexpr wchar_t* StageVirtualKeyStateListInfoKey = L"VKEY_STATE_LIST";
    static constexpr wchar_t* StageStartRandSeedInfoKey = L"START_RAND_SEED";
    static constexpr wchar_t* StageStartTimeInfoKey = L"START_TIME";
    static constexpr wchar_t* StageEndTimeInfoKey = L"END_TIME";
    static constexpr wchar_t* StagePauseCountInfoKey = L"PAUSE_COUNT";
private:
    // �e��f�[�^�͂��ׂ�CommonDataDB�ɕۑ�����, �����������ʂȂ̂Œ��ԃf�[�^�\���͍��Ȃ��B
    CommonDataDB data_;
    // �e�f�[�^�G���A�͈ȉ��Q�ƁB
    // INFO: ���v���C�S�̂̏����L�^����G���A
    static constexpr wchar_t* ReplayInfoAreaName = CommonDataDB::DefaultDataAreaName;
    // STAGE_<stageIdx>: �X�e�[�W�̏����L�^����G���A
    static std::wstring StageInfoAreaName(StageIndex stageIdx);
    // STAGE_<stageIdx>_<areaName>: �X�e�[�W�ɕۑ����ꂽ���ʃf�[�^�p�G���A
    static std::wstring StageCommonDataAreaName(StageIndex stageIdx, const CommonDataDB::DataAreaName& areaName);
};
}