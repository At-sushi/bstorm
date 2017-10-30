#pragma once

#include <deque>
#include <vector>

namespace bstorm {
  class Intersection;
  class GameState;
  class ObjCol {
  public:
    ObjCol(const std::shared_ptr<GameState>& gameState);
    ~ObjCol();
    const std::deque<std::shared_ptr<Intersection>>& getIntersections() const { return isects; }
    const std::vector<std::shared_ptr<Intersection>>& getTempIntersections() const { return oldTempIsects; }
    void pushIntersection(const std::shared_ptr<Intersection>& isect);
    void shiftIntersection();
    void addTempIntersection(const std::shared_ptr<Intersection>& isect);
    void transIntersection(float dx, float dy);
    void setWidthIntersection(float width);
    void renderIntersection(bool isPermitCamera) const;
    void clearIntersection();
    // update���ɕێ��p����ɂ��Ēǉ��p�ƕێ��p�����ւ���,
    void clearOldTempIntersection();
    bool isIntersected(const std::shared_ptr<ObjCol>& col) const;
    bool isIntersected() const;
    int getIntersectedCount() const;
    std::vector<std::weak_ptr<Intersection>> getCollideIntersections() const;
  private:
    std::deque<std::shared_ptr<Intersection>> isects;
    std::vector<std::shared_ptr<Intersection>> tempIsects; // �ǉ��p
    std::vector<std::shared_ptr<Intersection>> oldTempIsects; // �ێ��p
    std::weak_ptr<GameState> gameState;
  };
}
