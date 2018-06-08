#pragma once

#include <deque>
#include <vector>
#include <memory>

namespace bstorm
{
class Intersection;
class Renderer;
class CollisionDetector;
class Package;
class ObjCol
{
public:
    ObjCol(const std::shared_ptr<CollisionDetector>& colDetector, const std::shared_ptr<Package>& package);
    ~ObjCol();
    const std::deque<std::shared_ptr<Intersection>>& GetIntersections() const { return isects_; }
    const std::vector<std::shared_ptr<Intersection>>& GetTempIntersections() const { return tempIsects_; }
    bool IsIntersected(const std::shared_ptr<ObjCol>& col) const;
    bool IsIntersected() const;
    int GetIntersectedCount() const;
    std::vector<std::weak_ptr<Intersection>> GetCollideIntersections() const;
protected:
    void AddIntersection(const std::shared_ptr<Intersection>& isect);
    void RemoveOldestIntersection();
    void AddTempIntersection(const std::shared_ptr<Intersection>& isect);
    void TransIntersection(float dx, float dy);
    void SetWidthIntersection(float width);
    void RenderIntersection(const std::shared_ptr<Renderer>& renderer, bool isPermitCamera) const;
    void ClearIntersection();
    // Obj::Update���ɌĂ�ŕێ��p����ɂ��Ēǉ��p�ƕێ��p�����ւ���,
    void UpdateTempIntersection();
private:
    std::deque<std::shared_ptr<Intersection>> isects_;
    std::vector<std::shared_ptr<Intersection>> addedTempIsects_; // �ǉ��p
    std::vector<std::shared_ptr<Intersection>> tempIsects_; // �ێ��p
    std::shared_ptr<CollisionDetector> colDetector_;
    std::weak_ptr<Package> package_;
};
}
