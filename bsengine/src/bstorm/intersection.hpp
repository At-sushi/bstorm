#pragma once

#include <array>
#include <vector>
#include <deque>
#include <list>
#include <memory>

#include <bstorm/non_copyable.hpp>

namespace bstorm
{
struct BoundingBox
{
    BoundingBox();
    BoundingBox(float left, float top, float right, float bottom);
    bool IsIntersected(const BoundingBox& other) const;
    float left_;
    float top_;
    float right_;
    float bottom_;
};

bool IsIntersectedLineCircle(float x1, float y1, float x2, float y2, float width, float cx, float cy, float r);

class Renderer;
// Shape: 物体形状だけを保持
class Shape
{
public:
    enum class Type
    {
        CIRCLE,
        RECT
    };
    Shape(float x, float y, float r);
    Shape(float x1, float y1, float x2, float y2, float width);
    bool IsIntersected(const Shape& other) const;
    const BoundingBox& GetBoundingBox() const;
    void Trans(float dx, float dy);
    void SetWidth(float width);
    // develop only
    void Render(const std::shared_ptr<Renderer>& renderer, bool permitCamera) const;
    Type GetType() const;
    void GetCircle(float& x, float &y, float& r) const;
    void GetRect(float& x1, float& y1, float& x2, float& y2, float& width) const;
private:
    void UpdateBoundingBox();
    void TransBoundingBox(float dx, float dy);
    const Type type_;
    union
    {
        struct
        {
            float x;
            float y;
            float r;
        } Circle;
        struct
        {
            float x1;
            float y1;
            float x2;
            float y2;
            float width;
        } Rect;
    } params_;
    BoundingBox boundingBox_;
};

// ===================================================
// ◆ CollisionGroup
// ===================================================
// 当たり判定の種類ごとに設定される固有の数値
// n*nの衝突行列にIntersectionを格納する場合0~(n-1)までの値を設定する
using CollisionGroup = uint8_t;

class CollisionDetector;
// ===================================================
// ◆ Intersection
// ===================================================
// 当たり判定の基底
// これを継承していろいろな判定を作る。
// 各派生クラスは固定のCollisionGroupを持つので、CollisionMatrixごとに専用のものを作る必要がある。
class Intersection : private NonCopyable
{
public:
    Intersection(const Shape& shape, CollisionGroup colGroup);
    virtual ~Intersection();
    CollisionGroup GetCollisionGroup() const { return colGroup_; }
    bool IsIntersected(const std::shared_ptr<Intersection>& isect) const;
    virtual void Render(const std::shared_ptr<Renderer>& renderer, bool permitCamera) const;
    const Shape& GetShape() const;
    int GetTreeIndex() const;
    const std::deque<std::weak_ptr<Intersection>>& GetCollideIntersections() const;
protected:
    void ChangeCollisionGroup(CollisionGroup colGroup) { colGroup_ = colGroup; }
private:
    Shape shape_;
    CollisionGroup colGroup_;
    int treeIdx_;
    std::list<std::weak_ptr<Intersection>>::iterator posInCell_;
    std::deque<std::weak_ptr<Intersection>> collideIsects_; // 衝突した当たり判定

    friend class CollisionDetector;
};

// ===================================================
// ◆ CollisionFunction
// ===================================================
// 当たり判定時の処理を記述する関数
using CollisionFunction = void(*)(const std::shared_ptr<Intersection>&, const std::shared_ptr<Intersection>&);

// ===================================================
// ◆ CollisionMatrix
// ===================================================
// CollisionFunctionを格納した二次元配列
class CollisionMatrix : private NonCopyable
{
public:
    CollisionMatrix(int dim, const CollisionFunction* mat);
    ~CollisionMatrix();
    void Collide(const std::shared_ptr<Intersection>& isect1, const std::shared_ptr<Intersection>& isect2) const;
    int GetDimension() const { return dimension_; }
    bool IsCollidable(CollisionGroup group1, CollisionGroup group2) const;
private:
    const int dimension_;
    CollisionFunction* matrix_;
};

// CollisionDetector: 当たり判定の管理を行う。4分木構造になっている。
class CollisionDetector
{
public:
    // 4分木の分割度, 合計(4^(MaxLevel+1) - 1) / 3個のセルが生成される
    static constexpr int MaxLevel = 4;
    CollisionDetector(int fieldWidth, int fieldHeight, const std::shared_ptr<CollisionMatrix>& colMatrix);
    ~CollisionDetector();
    void Add(const std::shared_ptr<Intersection>&);
    void Remove(const std::shared_ptr<Intersection>&);
    void Update(const std::shared_ptr<Intersection>&);
    void Trans(const std::shared_ptr<Intersection>&, float dx, float dy);
    void SetWidth(const std::shared_ptr<Intersection>&, float width);
    // GetIntersectionsCollideWith ~: ある判定と当たっている判定を取得する
    // すべてのグループの判定を取得したいならtargetGroupを負にする
    std::deque<std::shared_ptr<Intersection>> GetIntersectionsCollideWithIntersection(const std::shared_ptr<Intersection>& isect, CollisionGroup targetGroup) const;
    std::deque<std::shared_ptr<Intersection>> GetIntersectionsCollideWithShape(const Shape& shape, CollisionGroup targetGroup) const;
    void TestAllCollision();
private:
    using VisitedIsects = std::vector<std::weak_ptr<Intersection>*>;
    void TestNodeCollision(int treeIdx, VisitedIsects visitedIsects[]);
    int CalcTreeIndexFromBoundingBox(const BoundingBox& boundingBox) const;
    const float fieldWidth_;
    const float fieldHeight_;
    const float unitCellWidth_;
    const float unitCellHeight_;
    std::shared_ptr<CollisionMatrix> colMatrix_;
    static constexpr int CellCount = ((1 << (2 * (MaxLevel + 1))) - 1) / 3;
    std::array<std::list<std::weak_ptr<Intersection>>, CellCount> quadTree_;
};

constexpr int DEFAULT_COLLISION_MATRIX_DIMENSION = 11;
extern const CollisionFunction DEFAULT_COLLISION_MATRIX[DEFAULT_COLLISION_MATRIX_DIMENSION * DEFAULT_COLLISION_MATRIX_DIMENSION];

constexpr CollisionGroup COL_GRP_ENEMY_SHOT = 0;
constexpr CollisionGroup COL_GRP_PLAYER_ERASE_SHOT = 1;
constexpr CollisionGroup COL_GRP_PLAYER_NON_ERASE_SHOT = 2;
constexpr CollisionGroup COL_GRP_PLAYER = 3;
constexpr CollisionGroup COL_GRP_PLAYER_GRAZE = 4;
constexpr CollisionGroup COL_GRP_ENEMY_TO_SHOT = 5;
constexpr CollisionGroup COL_GRP_ENEMY_TO_PLAYER = 6;
constexpr CollisionGroup COL_GRP_SPELL = 7;
constexpr CollisionGroup COL_GRP_PLAYER_TO_ITEM = 8;
constexpr CollisionGroup COL_GRP_ITEM = 9;
constexpr CollisionGroup COL_GRP_TEMP_ENEMY_SHOT = 10;

class ObjShot;
class ShotIntersection : public Intersection
{
public:
    ShotIntersection(float x, float y, float r, const std::shared_ptr<ObjShot>& shot, bool isTmpIntersection);
    ShotIntersection(float x1, float y1, float x2, float y2, float width, const std::shared_ptr<ObjShot>& shot, bool isTmpIntersection);
    void SetEraseShotEnable(bool enable);
    const std::weak_ptr<ObjShot>& GetShot() const { return shot_; }
    bool IsPlayerShot() const { return isPlayerShot_; }
    bool IsTempIntersection() const { return isTmpIntersection_; }
private:
    std::weak_ptr<ObjShot> shot_;
    const bool isPlayerShot_;
    const bool isTmpIntersection_;
};

class ObjEnemy;
class EnemyIntersectionToShot : public Intersection
{
public:
    EnemyIntersectionToShot(float x, float y, float r, const std::shared_ptr<ObjEnemy>& enemy);
    float GetX() { return x_; }
    float GetY() { return y_; }
    const std::weak_ptr<ObjEnemy>& GetEnemy() const { return enemy_; }
private:
    std::weak_ptr<ObjEnemy> enemy_;
    const float x_;
    const float y_;
};

class EnemyIntersectionToPlayer : public Intersection
{
public:
    EnemyIntersectionToPlayer(float x, float y, float r, const std::shared_ptr<ObjEnemy>& enemy);
    const std::weak_ptr<ObjEnemy>& GetEnemy() const { return enemy_; }
private:
    std::weak_ptr<ObjEnemy> enemy_;
};

class ObjPlayer;
class PlayerIntersection : public Intersection
{
public:
    PlayerIntersection(float x, float y, float r, const std::shared_ptr<ObjPlayer>& player);
    const std::weak_ptr<ObjPlayer>& GetPlayer() const { return player_; }
private:
    std::weak_ptr<ObjPlayer> player_;
};

class PlayerGrazeIntersection : public Intersection
{
public:
    PlayerGrazeIntersection(float x, float y, float r, const std::shared_ptr<ObjPlayer>& player);
    virtual void Render(const std::shared_ptr<Renderer>& renderer, bool permitCamera) const override {};
    const std::weak_ptr<ObjPlayer>& GetPlayer() const { return player_; }
private:
    std::weak_ptr<ObjPlayer> player_;
};

class ObjSpell;
class SpellIntersection : public Intersection
{
public:
    SpellIntersection(float x, float y, float r, const std::shared_ptr<ObjSpell>& spell);
    SpellIntersection(float x1, float y1, float x2, float y2, float width, const std::shared_ptr<ObjSpell>& spell);
    const std::weak_ptr<ObjSpell>& GetSpell() const { return spell_; }
private:
    std::weak_ptr<ObjSpell> spell_;
};

class PlayerIntersectionToItem : public Intersection
{
public:
    PlayerIntersectionToItem(float x, float y, const std::shared_ptr<ObjPlayer>& player);
    const std::weak_ptr<ObjPlayer>& GetPlayer() const { return player_; }
private:
    std::weak_ptr<ObjPlayer> player_;
};

class ObjItem;
class ItemIntersection : public Intersection
{
public:
    ItemIntersection(float x, float y, float r, const std::shared_ptr<ObjItem>& item);
    const std::weak_ptr<ObjItem>& GetItem() const { return item_; }
    void Render(const std::shared_ptr<Renderer>& renderer, bool permitCamera) const override {};
private:
    std::weak_ptr<ObjItem> item_;
};

class TempEnemyShotIntersection : public Intersection
{
public:
    TempEnemyShotIntersection(float x, float y, float r);
    TempEnemyShotIntersection(float x1, float y1, float x2, float y2, float width);
};
}