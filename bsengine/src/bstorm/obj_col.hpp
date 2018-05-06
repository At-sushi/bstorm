#pragma once

#include <deque>
#include <vector>
#include <memory>

namespace bstorm
{
class Intersection;
class GameState;
class Renderer;
class ObjCol
{
public:
    ObjCol(const std::shared_ptr<GameState>& gameState);
    ~ObjCol();
    const std::deque<std::shared_ptr<Intersection>>& GetIntersections() const { return isects_; }
    const std::vector<std::shared_ptr<Intersection>>& GetTempIntersections() const { return oldTempIsects_; }
    void PushBackIntersection(const std::shared_ptr<Intersection>& isect);
    void PopFrontIntersection();
    void AddTempIntersection(const std::shared_ptr<Intersection>& isect);
    void TransIntersection(float dx, float dy);
    void SetWidthIntersection(float width);
    void RenderIntersection(const std::unique_ptr<Renderer>& renderer, bool isPermitCamera) const;
    void ClearIntersection();
    // update���ɕێ��p����ɂ��Ēǉ��p�ƕێ��p�����ւ���,
    void ClearOldTempIntersection();
    bool IsIntersected(const std::shared_ptr<ObjCol>& col) const;
    bool IsIntersected() const;
    int GetIntersectedCount() const;
    std::vector<std::weak_ptr<Intersection>> GetCollideIntersections() const;
private:
    std::deque<std::shared_ptr<Intersection>> isects_;
    std::vector<std::shared_ptr<Intersection>> tempIsects_; // �ǉ��p
    std::vector<std::shared_ptr<Intersection>> oldTempIsects_; // �ێ��p
    std::weak_ptr<GameState> gameState_;
};
}
