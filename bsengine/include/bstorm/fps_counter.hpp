#pragma once

#include <array>
#include <windows.h>

namespace bstorm {
  class TimePoint {
  public:
    TimePoint();
    ~TimePoint();
    float getElapsedMilliSec(const TimePoint& tp = TimePoint()) const;
  private:
    bool isHighAccuracyMode;
    INT64 time; // �L�^���_
    INT64 freq;
    // �����x���[�h���g���Ȃ����p
    DWORD timeMilli;
  };

  class FpsCounter {
  public:
    FpsCounter();
    ~FpsCounter();
    void update();
    float get() const;
    float getStable() const;
  private:
    static constexpr int SampleCnt = 64; // power of 2
    TimePoint prevFrameTime;
    float milliSecPerFrameAccum;
    int milliSecPerFrameIdx;
    float fps;
    float stableFps; // �\���p(n�t���[����1��X�V����̂ŕ\�������Ƃ��Ƀ`�����Ȃ�)
    std::array<float, SampleCnt> milliSecPerFrameList;
  };
}