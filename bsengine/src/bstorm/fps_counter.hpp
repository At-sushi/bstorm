#pragma once

#include <memory>
#include <array>

namespace bstorm
{
class TimePoint;
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
    std::shared_ptr<TimePoint> prevFrameTimePoint_;
    float milliSecPerFrameAccum_;
    int milliSecPerFrameIdx_;
    float fps_;
    float stableFps_; // �\���p(n�t���[����1��X�V����̂ŕ\�������Ƃ��Ƀ`�����Ȃ�)
    std::array<float, SampleCnt> milliSecPerFrameList_;
};
}