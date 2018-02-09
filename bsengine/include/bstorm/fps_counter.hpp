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
  private:
    static constexpr int sampleCnt = 16; // 2�ׂ̂���
    TimePoint prevFrameTime;
    float milliSecPerFrameAccum;
    int milliSecPerFrameIdx;
    float fps;
    std::array<float, sampleCnt> milliSecPerFrameList;
  };
}