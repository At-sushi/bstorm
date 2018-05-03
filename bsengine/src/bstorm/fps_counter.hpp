#pragma once

#include <array>
#include <windows.h>

namespace bstorm
{
class TimePoint
{
public:
    TimePoint();
    ~TimePoint();
    float GetElapsedMilliSec(const TimePoint& tp = TimePoint()) const;
    float GetTimeMilliSec() const;
private:
    bool isHighAccuracyMode_;
    INT64 timeMicro_; // �L�^���_
    INT64 freq_;
    // �����x���[�h���g���Ȃ����p�̋L�^���_
    DWORD timeMilli_;
};

class FpsCounter
{
public:
    FpsCounter();
    ~FpsCounter();
    void Update();
    float Get() const;
    float GetStable() const;
private:
    static constexpr int SampleCnt = 64; // power of 2
    TimePoint prevFrameTime_;
    float milliSecPerFrameAccum_;
    int milliSecPerFrameIdx_;
    float fps_;
    float stableFps_; // �\���p(n�t���[����1��X�V����̂ŕ\�������Ƃ��Ƀ`�����Ȃ�)
    std::array<float, SampleCnt> milliSecPerFrameList_;
};
}