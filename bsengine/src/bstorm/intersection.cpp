#include <bstorm/intersection.hpp>

#include <bstorm/dnh_const.hpp>
#include <bstorm/point2D.hpp>
#include <bstorm/vertex.hpp>
#include <bstorm/math_util.hpp>
#include <bstorm/ptr_util.hpp>
#include <bstorm/dx_util.hpp>
#include <bstorm/renderer.hpp>
#include <bstorm/obj_enemy.hpp>
#include <bstorm/obj_shot.hpp>
#include <bstorm/obj_item.hpp>
#include <bstorm/obj_player.hpp>
#include <bstorm/obj_spell.hpp>

#include <array>
#include <deque>
#include <algorithm>
#include <d3dx9.h>
#include <cassert>

namespace bstorm
{

// Rect = std::array<Point2D, 4>
// 頂点の順番は以下の通り ([0] = (x1, y1), [2] = (x2, y2)
// [0] - [1]
//  |     |
// [3] - [2]

static std::array<Point2D, 4> LineToRect(float x1, float y1, float x2, float y2, float width)
{
    const float halfWidth = width / 2.0f;
    const float normalDir = atan2(y2 - y1, x2 - x1) + D3DX_PI / 2.0f;
    const float dx = halfWidth * cos(normalDir);
    const float dy = halfWidth * sin(normalDir);
    return std::array<Point2D, 4>{Point2D(x1 + dx, y1 + dy),
        Point2D(x1 - dx, y1 - dy),
        Point2D(x2 - dx, y2 - dy),
        Point2D(x2 + dx, y2 + dy)};
}

BoundingBox::BoundingBox() :
    left_(0.0f),
    top_(0.0f),
    right_(0.0f),
    bottom_(0.0f)
{
}

BoundingBox::BoundingBox(float l, float t, float r, float b) :
    left_(l),
    top_(t),
    right_(r),
    bottom_(b)
{
}

bool BoundingBox::IsIntersected(const BoundingBox& other) const
{
    return left_ <= other.right_ && top_ <= other.bottom_ && other.left_ <= right_ && other.top_ <= bottom_;
}

// 外積
static inline float cross2(float x1, float y1, float x2, float y2)
{
    return x1 * y2 - x2 * y1;
}

// 内積
static inline float dot2(float x1, float y1, float x2, float y2)
{
    return x1 * x2 + y1 * y2;
}

// 線分と線分の交差判定
static bool IsIntersectedSegmentSegment(const Point2D& a, const Point2D& b, const Point2D& c, const Point2D& d)
{
    // x : cross
    // * : multiply
    // check
    // AC x DC * BC x DC <= 0 && AC x AB * AD x AB <= 0

    float acX = c.x - a.x;
    float acY = c.y - a.y;
    float abX = b.x - a.x;
    float abY = b.y - a.y;
    float adX = d.x - a.x;
    float adY = d.y - a.y;
    float bcX = c.x - b.x;
    float bcY = c.y - b.y;
    float dcX = c.x - d.x;
    float dcY = c.y - d.y;
    float cp1 = cross2(acX, acY, dcX, dcY);
    float cp2 = cross2(bcX, bcY, dcX, dcY);
    float cp3 = cross2(acX, acY, abX, abY);
    float cp4 = cross2(adX, adY, abX, abY);

    return cp1 * cp2 <= 0.0f && cp3 * cp4 <= 0.0f;
}

// 点が矩形内にあるかどうか判定
static bool IsPointInRect(const Point2D& p, const std::array<Point2D, 4>& rect)
{
    const Point2D& a = rect[0];
    const Point2D& b = rect[1];
    const Point2D& c = rect[2];
    const Point2D& d = rect[3];
    float apX = p.x - a.x;
    float apY = p.y - a.y;
    float abX = b.x - a.x;
    float abY = b.y - a.y;
    float bpX = p.x - b.x;
    float bpY = p.y - b.y;
    float bcX = c.x - b.x;
    float bcY = c.y - b.y;
    float cpX = p.x - c.x;
    float cpY = p.y - c.y;
    float cdX = d.x - c.x;
    float cdY = d.y - c.y;
    float dpX = p.x - d.x;
    float dpY = p.y - d.y;
    float daX = a.x - d.x;
    float daY = a.y - d.y;
    float cp1 = cross2(apX, apY, abX, abY);
    float cp2 = cross2(bpX, bpY, bcX, bcY);
    float cp3 = cross2(cpX, cpY, cdX, cdY);
    float cp4 = cross2(dpX, dpY, daX, daY);
    // 全て負なら矩形内に含まれる
    // 矩形は必ず時計回りなので全て正の場合を調べる必要はない
    return cp1 <= 0.0f && cp2 <= 0.0f && cp3 <= 0.0f && cp4 <= 0.0f;
}

static bool IsIntersectedCircleSegment(float cx, float cy, float r, const Point2D& a, const Point2D& b)
{
    float abX = b.x - a.x;
    float abY = b.y - a.y;
    float acX = cx - a.x;
    float acY = cy - a.y;
    float bcX = cx - b.x;
    float bcY = cy - b.y;
    // d : 円の中心からABを通る直線への垂線の長さ
    float d = abs(cross2(abX, abY, acX, acY)) / std::hypotf(abX, abY);
    if (d > r) return false;
    if (dot2(abX, abY, acX, acY) * dot2(abX, abY, bcX, bcY) <= 0.0f) return true;
    return std::hypotf(acX, acY) <= r || std::hypotf(bcX, bcY) <= r;
}

// 弾幕風のLine = Rect
bool IsIntersectedLineCircle(float x1, float y1, float x2, float y2, float width, float cx, float cy, float r)
{
    const auto rect = LineToRect(x1, y1, x2, y2, width);
    // 円が矩形の辺と交わっている場合
    for (int i = 0; i < 4; i++)
    {
        if (IsIntersectedCircleSegment(cx, cy, r, rect[i], rect[(i + 1) & 3])) return true;
    }
    // 円が矩形に入っている場合
    return IsPointInRect(Point2D(cx, cy), rect);
}

Shape::Shape(float x, float y, float r) :
    type_(Type::CIRCLE)
{
    params_.Circle.x = x;
    params_.Circle.y = y;
    params_.Circle.r = r;
    UpdateBoundingBox();
}

Shape::Shape(float x1, float y1, float x2, float y2, float width) :
    type_(Type::RECT)
{
    params_.Rect.x1 = x1;
    params_.Rect.y1 = y1;
    params_.Rect.x2 = x2;
    params_.Rect.y2 = y2;
    params_.Rect.width = width;
    UpdateBoundingBox();
}

bool Shape::IsIntersected(const Shape& other) const
{
    if (!boundingBox_.IsIntersected(other.boundingBox_))
    {
        // BB同士が当たっていない場合は当たっていない
        return false;
    }

    if (type_ == Type::CIRCLE && other.type_ == Type::CIRCLE)
    {
        // 円と円
        float dx = params_.Circle.x - other.params_.Circle.x;
        float dy = params_.Circle.y - other.params_.Circle.y;
        float d = params_.Circle.r + other.params_.Circle.r;
        return dx * dx + dy * dy <= d * d;
    } else if (type_ == Type::CIRCLE && other.type_ == Type::RECT)
    {
        // 円と矩形
        float x1 = other.params_.Rect.x1;
        float y1 = other.params_.Rect.y1;
        float x2 = other.params_.Rect.x2;
        float y2 = other.params_.Rect.y2;
        float width = other.params_.Rect.width;
        float cx = params_.Circle.x;
        float cy = params_.Circle.y;
        float r = params_.Circle.r;
        return IsIntersectedLineCircle(x1, y1, x2, y2, width, cx, cy, r);
    } else if (type_ == Type::RECT && other.type_ == Type::CIRCLE)
    {
        // 円と矩形
        float x1 = params_.Rect.x1;
        float y1 = params_.Rect.y1;
        float x2 = params_.Rect.x2;
        float y2 = params_.Rect.y2;
        float width = params_.Rect.width;
        float cx = other.params_.Circle.x;
        float cy = other.params_.Circle.y;
        float r = other.params_.Circle.r;
        return IsIntersectedLineCircle(x1, y1, x2, y2, width, cx, cy, r);
    } else if (type_ == Type::RECT && other.type_ == Type::RECT)
    {
        // 矩形と矩形
        const auto rect1 = LineToRect(params_.Rect.x1, params_.Rect.y1, params_.Rect.x2, params_.Rect.y2, params_.Rect.width);
        const auto rect2 = LineToRect(other.params_.Rect.x1, other.params_.Rect.y1, other.params_.Rect.x2, other.params_.Rect.y2, other.params_.Rect.width);
        // 辺同士が交わってる場合
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                if (IsIntersectedSegmentSegment(rect1[i], rect1[(i + 1) & 3], rect2[j], rect2[(j + 1) & 3])) return true;
            }
        }
        // 矩形が片方の矩形に完全に含まれる場合
        return IsPointInRect(rect1[0], rect2) || IsPointInRect(rect2[0], rect1);
    }
    return false;
}

const BoundingBox & Shape::GetBoundingBox() const
{
    return boundingBox_;
}

void Shape::Trans(float dx, float dy)
{
    if (type_ == Type::CIRCLE)
    {
        params_.Circle.x += dx;
        params_.Circle.y += dy;
    } else if (type_ == Type::RECT)
    {
        params_.Rect.x1 += dx; params_.Rect.x2 += dx;
        params_.Rect.y1 += dy; params_.Rect.y2 += dy;
    }
    TransBoundingBox(dx, dy);
}

void Shape::SetWidth(float width)
{
    width = abs(width);
    if (type_ == Type::CIRCLE)
    {
        params_.Circle.r = width / 2.0f;
    } else if (type_ == Type::RECT)
    {
        params_.Rect.width = width;
    }
    UpdateBoundingBox();
}

Shape::Type Shape::GetType() const
{
    return type_;
}

void Shape::GetCircle(float & x, float & y, float & r) const
{
    x = params_.Circle.x;
    y = params_.Circle.y;
    r = params_.Circle.r;
}

void Shape::GetRect(float & x1, float & y1, float & x2, float & y2, float & width) const
{
    x1 = params_.Rect.x1;
    y1 = params_.Rect.y1;
    x2 = params_.Rect.x2;
    y2 = params_.Rect.y2;
    width = params_.Rect.width;
}

void Shape::UpdateBoundingBox()
{
    if (type_ == Type::CIRCLE)
    {
        boundingBox_.left_ = params_.Circle.x - params_.Circle.r;
        boundingBox_.top_ = params_.Circle.y - params_.Circle.r;
        boundingBox_.right_ = params_.Circle.x + params_.Circle.r;
        boundingBox_.bottom_ = params_.Circle.y + params_.Circle.r;
    } else if (type_ == Type::RECT)
    {
        const auto rect = LineToRect(params_.Rect.x1, params_.Rect.y1, params_.Rect.x2, params_.Rect.y2, params_.Rect.width);
        float minX = rect[0].x;
        float maxX = minX;
        float minY = rect[0].y;
        float maxY = minY;
        for (int i = 1; i < 4; i++)
        {
            float x = rect[i].x;
            minX = std::min(minX, x);
            maxX = std::max(maxX, x);
            float y = rect[i].y;
            minY = std::min(minY, y);
            maxY = std::max(maxY, y);
        }
        boundingBox_.left_ = minX;
        boundingBox_.top_ = minY;
        boundingBox_.right_ = maxX;
        boundingBox_.bottom_ = maxY;
    }
}

void Shape::TransBoundingBox(float dx, float dy)
{
    boundingBox_.left_ += dx;
    boundingBox_.top_ += dy;
    boundingBox_.right_ += dx;
    boundingBox_.bottom_ += dy;
}

CollisionMatrix::CollisionMatrix(int dim, const CollisionFunction * mat) :
    dimension_(dim)
{
    assert(dimension_ >= 0);

    matrix_ = new CollisionFunction[dimension_ * dimension_];
    for (int i = 0; i < dimension_ * dimension_; i++)
    {
        matrix_[i] = mat[i];
    }
}

CollisionMatrix::~CollisionMatrix()
{
    safe_delete_array(matrix_);
}

void CollisionMatrix::Collide(const std::shared_ptr<Intersection>& isect1, const std::shared_ptr<Intersection>& isect2) const
{
    const auto group1 = isect1->GetCollisionGroup();
    const auto group2 = isect2->GetCollisionGroup();

    assert(group1 >= 0 && group1 < dimension_);
    assert(group2 >= 0 && group2 < dimension_);

    const auto func1 = matrix_[group1 * dimension_ + group2];
    const auto func2 = matrix_[group2 * dimension_ + group1];

    // どちらか片方だけ実行
    if (func1)
    {
        func1(isect1, isect2);
    } else if (func2)
    {
        func2(isect2, isect1);
    }
}

bool CollisionMatrix::IsCollidable(CollisionGroup group1, CollisionGroup group2) const
{
    const auto func1 = matrix_[group1 * dimension_ + group2];
    const auto func2 = matrix_[group2 * dimension_ + group1];
    return func1 || func2;
}


Intersection::Intersection(const Shape& shape, CollisionGroup colGroup) :
    shape_(shape),
    colGroup_(colGroup),
    treeIdx_(-1)
{
    assert(colGroup_ >= 0);
}

Intersection::~Intersection() {}

bool Intersection::IsIntersected(const std::shared_ptr<Intersection>& isect) const
{
    return shape_.IsIntersected(isect->shape_);
}

void Intersection::Render(const std::shared_ptr<Renderer>& renderer, bool permitCamera) const
{
    shape_.Render(renderer, permitCamera);
}

const Shape & Intersection::GetShape() const
{
    return shape_;
}

int Intersection::GetTreeIndex() const
{
    return treeIdx_;
}

const std::deque<std::weak_ptr<Intersection>>& Intersection::GetCollideIntersections() const
{
    return collideIsects_;
}

static inline int CalcTreeIndex(int level, int morton)
{
    return ((1 << (level << 1)) - 1) / 3 + morton;
}

CollisionDetector::CollisionDetector(int fieldWidth, int fieldHeight, const std::shared_ptr<CollisionMatrix>& colMatrix) :
    fieldWidth_((float)fieldWidth),
    fieldHeight_((float)fieldHeight),
    unitCellWidth_(1.0f * fieldWidth / (1 << MaxLevel)),
    unitCellHeight_(1.0f * fieldHeight / (1 << MaxLevel)),
    colMatrix_(colMatrix)
{
    assert(fieldWidth_ >= 0.0f);
    assert(fieldHeight_ >= 0.0f);
    assert(MaxLevel >= 0);
}

CollisionDetector::~CollisionDetector()
{
}

void CollisionDetector::Add(const std::shared_ptr<Intersection>& isect)
{
    if (isect->treeIdx_ >= 0)
    {
        Remove(isect);
    }
    isect->treeIdx_ = CalcTreeIndexFromBoundingBox(isect->shape_.GetBoundingBox());
    auto& cell = quadTree_[isect->treeIdx_];
    isect->posInCell_ = cell.insert(cell.end(), isect);
}

void CollisionDetector::Remove(const std::shared_ptr<Intersection>& isect)
{
    if (isect->treeIdx_ >= 0)
    {
        isect->posInCell_->reset();
        isect->treeIdx_ = -1;
    }
}

void CollisionDetector::Update(const std::shared_ptr<Intersection>& isect)
{
    Remove(isect);
    Add(isect);
}

void CollisionDetector::Trans(const std::shared_ptr<Intersection>& isect, float dx, float dy)
{
    isect->shape_.Trans(dx, dy);
    Update(isect);
}

void CollisionDetector::SetWidth(const std::shared_ptr<Intersection>& isect, float width)
{
    isect->shape_.SetWidth(width);
    Update(isect);
}

std::deque<std::shared_ptr<Intersection>> CollisionDetector::GetIntersectionsCollideWithIntersection(const std::shared_ptr<Intersection>& self, CollisionGroup targetGroup) const
{
    std::deque<std::shared_ptr<Intersection>> ret;
    const CollisionGroup group1 = self->GetCollisionGroup();

    // 幅優先探索
    const int startTreeIndex = self->GetTreeIndex();
    std::deque<int> treeIndices;
    treeIndices.push_back(startTreeIndex);
    while (!treeIndices.empty())
    {
        const int treeIdx = treeIndices.front();
        treeIndices.pop_front();
        for (const auto& p : quadTree_.at(treeIdx))
        {
            if (auto other = p.lock())
            {
                const CollisionGroup group2 = other->GetCollisionGroup();
                // ターゲットグループでないなら無視
                if (targetGroup >= 0 && group2 != targetGroup) continue;
                // 衝突しないグループ同士なら無視
                if (!colMatrix_->IsCollidable(group1, group2)) continue;
                if (self->IsIntersected(other))
                {
                    ret.push_back(other);
                }
            }
        }
        // 下位レベル
        if (treeIdx >= startTreeIndex)
        {
            int lowLevelNode1 = (treeIdx << 2) + 1;
            if (lowLevelNode1 < quadTree_.size())
            {
                treeIndices.push_back(lowLevelNode1);
                treeIndices.push_back(lowLevelNode1 + 1);
                treeIndices.push_back(lowLevelNode1 + 2);
                treeIndices.push_back(lowLevelNode1 + 3);
            }
        }
        // 上位レベル
        if (treeIdx <= startTreeIndex)
        {
            if (treeIdx != 0)
            {
                treeIndices.push_back(((treeIdx - 1) >> 2));
            }
        }
    }
    return ret;
}

std::deque<std::shared_ptr<Intersection>> CollisionDetector::GetIntersectionsCollideWithShape(const Shape & self, CollisionGroup targetGroup) const
{
    std::deque<std::shared_ptr<Intersection>> ret;

    // 幅優先探索
    const int startTreeIndex = CalcTreeIndexFromBoundingBox(self.GetBoundingBox());
    std::deque<int> treeIndices;
    treeIndices.push_back(startTreeIndex);
    while (!treeIndices.empty())
    {
        const int treeIdx = treeIndices.front();
        treeIndices.pop_front();
        for (const auto& p : quadTree_.at(treeIdx))
        {
            if (auto other = p.lock())
            {
                // ターゲットグループでないなら無視
                if (targetGroup >= 0 && other->GetCollisionGroup() != targetGroup) continue;
                if (self.IsIntersected(other->shape_)) ret.push_back(other);
            }
        }
        // 下位レベル
        if (treeIdx >= startTreeIndex)
        {
            int lowLevelNode1 = (treeIdx << 2) + 1;
            if (lowLevelNode1 < quadTree_.size())
            {
                treeIndices.push_back(lowLevelNode1);
                treeIndices.push_back(lowLevelNode1 + 1);
                treeIndices.push_back(lowLevelNode1 + 2);
                treeIndices.push_back(lowLevelNode1 + 3);
            }
        }
        // 上位レベル
        if (treeIdx <= startTreeIndex)
        {
            if (treeIdx != 0)
            {
                treeIndices.push_back(((treeIdx - 1) >> 2));
            }
        }
    }
    return ret;
}

void CollisionDetector::TestAllCollision()
{
    static std::unique_ptr<VisitedIsects[]> visitedIsects(new VisitedIsects[colMatrix_->GetDimension()], std::default_delete<VisitedIsects[]>());
    TestNodeCollision(0, visitedIsects.get());
}

// 指定したノードの上位と下位にある全当たり判定のペアに対して衝突検査を行う
// treeIdx: ノード番号
// visitedIsects: 上位レベルのノードか、このノードで既に発見された当たり判定
//                shared_ptrをそのまま格納するとコピーのコストが重いので、weak_ptrの場所を示す生ポインタを保持
//                visitedに追加されたポインタの指す先がTestCollision時に削除されることはないので問題ない。

// NOTE: CollisionFunction内でオブジェクトを移動させたりしてCollisionDetector内のIntersectionの位置が変わると、移動先でさらに判定が取られてしまう
//       弾幕風の場合、衝突時に移動することはないのでこれを仕様とし特に対策は行わない
void CollisionDetector::TestNodeCollision(int treeIdx, VisitedIsects visitedIsects[])
{
    // 上位のレベルの判定の数をグループごとに覚えておく
    std::unique_ptr<size_t[]> prevVisitedIsectCounts(new size_t[colMatrix_->GetDimension()], std::default_delete<size_t[]>());
    for (int i = 0; i < colMatrix_->GetDimension(); ++i)
    {
        prevVisitedIsectCounts[i] = visitedIsects[i].size();
    }

    // 上位レベルに所属する全てのIntersectionと衝突判定を取る（グループごと)
    auto& cell = quadTree_[treeIdx];
    auto it = cell.begin();
    while (it != cell.end())
    {
        if (auto newVisit = it->lock())
        {
            // 前フレームで衝突した当たり判定を空にする
            newVisit->collideIsects_.clear();
            const CollisionGroup group1 = newVisit->GetCollisionGroup();
            for (int group2 = 0; group2 < colMatrix_->GetDimension(); ++group2)
            {
                // 衝突しないグループは無視
                if (!colMatrix_->IsCollidable(group1, group2)) continue;

                for (auto& p : visitedIsects[group2])
                {
                    if (auto visited = p->lock())
                    {
                        if (newVisit->IsIntersected(visited))
                        {
                            // 衝突した相手を保存
                            newVisit->collideIsects_.push_back(visited);
                            visited->collideIsects_.push_back(newVisit);
                            colMatrix_->Collide(newVisit, visited);
                        }
                    }
                }
            }
            // 発見済みに追加
            visitedIsects[group1].push_back(&(*it));
            ++it;
        } else
        {
            // 弱参照が切れてたらリストから削除
            it = cell.erase(it);
        }
    }
    const int lowLevelNode1 = (treeIdx << 2) + 1;
    if (lowLevelNode1 < quadTree_.size())
    {
        TestNodeCollision(lowLevelNode1, visitedIsects);
        TestNodeCollision(lowLevelNode1 + 1, visitedIsects);
        TestNodeCollision(lowLevelNode1 + 2, visitedIsects);
        TestNodeCollision(lowLevelNode1 + 3, visitedIsects);
    }

    // このノード以下で得た当たり判定を除外
    for (int i = 0; i < colMatrix_->GetDimension(); ++i)
    {
        visitedIsects[i].resize(prevVisitedIsectCounts[i]);
    }
}

static inline DWORD separateBit(DWORD n)
{
    n = (n | (n << 8)) & 0x00ff00ff;
    n = (n | (n << 4)) & 0x0f0f0f0f;
    n = (n | (n << 2)) & 0x33333333;
    return (n | (n << 1)) & 0x55555555;
}

static inline DWORD pointToMorton(float x, float y, float unitWidth, float unitHeight)
{
    return separateBit(x / unitWidth) | (separateBit(y / unitHeight) << 1);
}

int CollisionDetector::CalcTreeIndexFromBoundingBox(const BoundingBox & boundingBox) const
{
    float left = constrain(boundingBox.left_, 0.0f, fieldWidth_ - 1);
    float right = constrain(boundingBox.right_, 0.0f, fieldWidth_ - 1);
    float top = constrain(boundingBox.top_, 0.0f, fieldHeight_ - 1);
    float bottom = constrain(boundingBox.bottom_, 0.0f, fieldHeight_ - 1);

    DWORD m1 = pointToMorton(left, top, unitCellWidth_, unitCellHeight_);
    DWORD m2 = pointToMorton(right, bottom, unitCellWidth_, unitCellHeight_);
    DWORD m = m1 ^ m2;
    int k = 0;
    for (int i = 0; i < MaxLevel; m >>= 2, i++)
    {
        if ((m & 0x3) != 0) { k = i + 1; }
    }
    int level = MaxLevel - k;
    DWORD morton = m2 >> (k << 1);
    return CalcTreeIndex(level, morton);
}

ShotIntersection::ShotIntersection(float x, float y, float r, const std::shared_ptr<ObjShot>& shot, bool isTmpIntersection) :
    Intersection(Shape(x, y, r),
                 shot->IsPlayerShot() ?
                 (shot->IsEraseShotEnabled() ? COL_GRP_PLAYER_ERASE_SHOT : COL_GRP_PLAYER_NON_ERASE_SHOT) :
                 COL_GRP_ENEMY_SHOT),
    shot_(shot),
    isPlayerShot_(shot->IsPlayerShot()),
    isTmpIntersection_(isTmpIntersection)
{
}

ShotIntersection::ShotIntersection(float x1, float y1, float x2, float y2, float width, const std::shared_ptr<ObjShot>& shot, bool isTmpIntersection) :
    Intersection(Shape(x1, y1, x2, y2, width),
                 shot->IsPlayerShot() ?
                 (shot->IsEraseShotEnabled() ? COL_GRP_PLAYER_ERASE_SHOT : COL_GRP_PLAYER_NON_ERASE_SHOT) :
                 COL_GRP_ENEMY_SHOT),
    shot_(shot),
    isPlayerShot_(shot->IsPlayerShot()),
    isTmpIntersection_(isTmpIntersection)
{
}

void ShotIntersection::SetEraseShotEnable(bool enable)
{
    ChangeCollisionGroup(enable ? COL_GRP_PLAYER_ERASE_SHOT : COL_GRP_PLAYER_NON_ERASE_SHOT);
}

EnemyIntersectionToShot::EnemyIntersectionToShot(float x, float y, float r, const std::shared_ptr<ObjEnemy>& enemy) :
    Intersection(Shape(x, y, r), COL_GRP_ENEMY_TO_SHOT),
    enemy_(enemy),
    x_(x),
    y_(y)
{
}

EnemyIntersectionToPlayer::EnemyIntersectionToPlayer(float x, float y, float r, const std::shared_ptr<ObjEnemy>& enemy) :
    Intersection(Shape(x, y, r), COL_GRP_ENEMY_TO_PLAYER),
    enemy_(enemy)
{
}

PlayerIntersection::PlayerIntersection(float x, float y, float r, const std::shared_ptr<ObjPlayer>& player) :
    Intersection(Shape(x, y, r), COL_GRP_PLAYER),
    player_(player)
{
}

PlayerGrazeIntersection::PlayerGrazeIntersection(float x, float y, float r, const std::shared_ptr<ObjPlayer>& player) :
    Intersection(Shape(x, y, r), COL_GRP_PLAYER_GRAZE),
    player_(player)
{
}

SpellIntersection::SpellIntersection(float x, float y, float r, const std::shared_ptr<ObjSpell>& spell) :
    Intersection(Shape(x, y, r), COL_GRP_SPELL),
    spell_(spell)
{
}

SpellIntersection::SpellIntersection(float x1, float y1, float x2, float y2, float width, const std::shared_ptr<ObjSpell>& spell) :
    Intersection(Shape(x1, y1, x2, y2, width), COL_GRP_SPELL),
    spell_(spell)
{
}

PlayerIntersectionToItem::PlayerIntersectionToItem(float x, float y, const std::shared_ptr<ObjPlayer>& player) :
    Intersection(Shape(x, y, 0), COL_GRP_PLAYER_TO_ITEM),
    player_(player)
{
}

ItemIntersection::ItemIntersection(float x, float y, float r, const std::shared_ptr<ObjItem>& item) :
    Intersection(Shape(x, y, r), COL_GRP_ITEM),
    item_(item)
{
}

TempEnemyShotIntersection::TempEnemyShotIntersection(float x, float y, float r) :
    Intersection(Shape(x, y, r), COL_GRP_TEMP_ENEMY_SHOT)
{
}

TempEnemyShotIntersection::TempEnemyShotIntersection(float x1, float y1, float x2, float y2, float width) :
    Intersection(Shape(x1, y1, x2, y2, width), COL_GRP_TEMP_ENEMY_SHOT)
{
}

static inline bool isShotIntersectionEnabled(const std::shared_ptr<ShotIntersection>& isect)
{
    if (auto shot = isect->GetShot().lock())
    {
        // Regist前でShotDataで元から設定されている当たり判定の時は衝突無効
        if (!shot->IsRegistered() && !isect->IsTempIntersection()) return false;
        // 判定が無効になっているとき、または遅延時は衝突無効
        if (!shot->IsIntersectionEnabled() || shot->IsDelay()) return false;
        return true;
    }
    return false;
}

static void collideEnemyShotWithPlayerEraseShot(const std::shared_ptr<Intersection>& isect1, const std::shared_ptr<Intersection>& isect2)
{
    auto playerShotIsect = std::dynamic_pointer_cast<ShotIntersection>(isect2);
    if (auto playerShot = playerShotIsect->GetShot().lock())
    {
        if (playerShot->IsEraseShotEnabled())
        {
            auto enemyShotIsect = std::dynamic_pointer_cast<ShotIntersection>(isect1);
            if (auto enemyShot = enemyShotIsect->GetShot().lock())
            {
                if (isShotIntersectionEnabled(enemyShotIsect) && isShotIntersectionEnabled(playerShotIsect))
                {
                    if (playerShot->GetType() == OBJ_SHOT)
                    {
                        playerShot->SetPenetration(playerShot->GetPenetration() - 1);
                    }
                    enemyShot->EraseWithSpell();
                }
            }
        }
    }
}

static void collideEnemyShotWithPlayer(const std::shared_ptr<Intersection>& isect1, const std::shared_ptr<Intersection>& isect2)
{
    auto enemyShotIsect = std::dynamic_pointer_cast<ShotIntersection>(isect1);
    if (isShotIntersectionEnabled(enemyShotIsect))
    {
        if (auto enemyShot = enemyShotIsect->GetShot().lock())
        {
            if (auto player = std::dynamic_pointer_cast<PlayerIntersection>(isect2)->GetPlayer().lock())
            {
                player->Hit(enemyShot->GetID());
            }
        }
    }
}

static void collideEnemyShotWithPlayerGraze(const std::shared_ptr<Intersection>& isect1, const std::shared_ptr<Intersection>& isect2)
{
    auto enemyShotIsect = std::dynamic_pointer_cast<ShotIntersection>(isect1);
    if (isShotIntersectionEnabled(enemyShotIsect))
    {
        if (auto enemyShot = enemyShotIsect->GetShot().lock())
        {
            if (auto player = std::dynamic_pointer_cast<PlayerGrazeIntersection>(isect2)->GetPlayer().lock())
            {
                if (enemyShot->IsGrazeEnabled() && player->IsGrazeEnabled())
                {
                    player->GrazeToShot(enemyShot->GetID(), 1);
                    enemyShot->Graze();
                }
            }
        }
    }
}

static void collideEnemyShotWithSpell(const std::shared_ptr<Intersection>& isect1, const std::shared_ptr<Intersection>& isect2)
{
    auto enemyShotIsect = std::dynamic_pointer_cast<ShotIntersection>(isect1);
    if (auto spell = std::dynamic_pointer_cast<SpellIntersection>(isect2)->GetSpell().lock())
    {
        if (spell->IsEraseShotEnabled())
        {
            if (isShotIntersectionEnabled(enemyShotIsect))
            {
                if (auto enemyShot = enemyShotIsect->GetShot().lock())
                {
                    enemyShot->EraseWithSpell();
                }
            }
        }
    }
}

static void collidePlayerShotWithEnemyIntersectionToShot(const std::shared_ptr<Intersection>& isect1, const std::shared_ptr<Intersection>& isect2)
{
    auto playerShotIsect = std::dynamic_pointer_cast<ShotIntersection>(isect1);
    auto enemyIsectToShot = std::dynamic_pointer_cast<EnemyIntersectionToShot>(isect2);
    if (isShotIntersectionEnabled(playerShotIsect))
    {
        if (auto playerShot = playerShotIsect->GetShot().lock())
        {
            if (auto enemy = enemyIsectToShot->GetEnemy().lock())
            {
                if (playerShot->GetType() == OBJ_SHOT)
                {
                    playerShot->SetPenetration(playerShot->GetPenetration() - 1);
                }
                if (playerShot->IsSpellFactorEnabled())
                {
                    enemy->AddSpellDamage(playerShot->GetDamage());
                } else
                {
                    enemy->AddShotDamage(playerShot->GetDamage());
                }
            }
        }
    }
}

static void collidePlayerWithEnemyIntersectionToPlayer(const std::shared_ptr<Intersection>& isect1, const std::shared_ptr<Intersection>& isect2)
{
    if (auto player = std::dynamic_pointer_cast<PlayerIntersection>(isect1)->GetPlayer().lock())
    {
        if (auto enemy = std::dynamic_pointer_cast<EnemyIntersectionToPlayer>(isect2)->GetEnemy().lock())
        {
            player->Hit(enemy->GetID());
        }
    }
}

static void collideEnemyIntersectionToShotWithSpell(const std::shared_ptr<Intersection>& isect1, const std::shared_ptr<Intersection>& isect2)
{
    if (auto enemy = std::dynamic_pointer_cast<EnemyIntersectionToShot>(isect1)->GetEnemy().lock())
    {
        if (auto spell = std::dynamic_pointer_cast<SpellIntersection>(isect2)->GetSpell().lock())
        {
            enemy->AddSpellDamage(spell->GetDamage());
        }
    }
}

static void collideWithPlayerToItemWithItem(const std::shared_ptr<Intersection>& isect1, const std::shared_ptr<Intersection>& isect2)
{
    if (auto player = std::dynamic_pointer_cast<PlayerIntersectionToItem>(isect1)->GetPlayer().lock())
    {
        if (auto item = std::dynamic_pointer_cast<ItemIntersection>(isect2)->GetItem().lock())
        {
            if (player->GetState() == STATE_NORMAL)
            {
                player->ObtainItem(item->GetID());
                item->Obtained();
            }
        }
    }
}

static void collidePlayerWithTempEnemyShot(const std::shared_ptr<Intersection>& isect1, const std::shared_ptr<Intersection>& isect2)
{
    if (auto player = std::dynamic_pointer_cast<PlayerIntersection>(isect1)->GetPlayer().lock())
    {
        player->Hit(ID_INVALID);
    }
}

// col matrix
const CollisionFunction DEFAULT_COLLISION_MATRIX[DEFAULT_COLLISION_MATRIX_DIMENSION * DEFAULT_COLLISION_MATRIX_DIMENSION] = {
    /* COL_GRP_ENEMY_SHOT           */  nullptr, collideEnemyShotWithPlayerEraseShot, nullptr, collideEnemyShotWithPlayer, collideEnemyShotWithPlayerGraze, nullptr, nullptr, collideEnemyShotWithSpell, nullptr, nullptr, nullptr,
    /* COL_GRP_PLAYER_ERASE_SHOT    */  nullptr, nullptr, nullptr, nullptr, nullptr, collidePlayerShotWithEnemyIntersectionToShot, nullptr, nullptr, nullptr, nullptr, nullptr,
    /* COL_GRP_PLAYER_NON_ERASESHOT */  nullptr, nullptr, nullptr, nullptr, nullptr, collidePlayerShotWithEnemyIntersectionToShot, nullptr, nullptr, nullptr, nullptr, nullptr,
    /* COL_GRP_PLAYER               */  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, collidePlayerWithEnemyIntersectionToPlayer, nullptr, nullptr, nullptr, collidePlayerWithTempEnemyShot,
    /* COL_GRP_PLAYER_GRAZE         */  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    /* COL_GRP_ENEMY_TO_SHOT        */  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, collideEnemyIntersectionToShotWithSpell, nullptr, nullptr, nullptr,
    /* COL_GRP_ENEMY_TO_PLAYER      */  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    /* COL_GRP_SPELL                */  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    /* COL_GRP_PLAYER_TO_ITEM       */  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, collideWithPlayerToItemWithItem, nullptr,
    /* COL_GRP_ITEM                 */  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    /* COL_GRP_TEMP_ENEMY_SHOT      */  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
};
}