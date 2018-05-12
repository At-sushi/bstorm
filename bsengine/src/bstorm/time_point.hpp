#pragma once

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
}