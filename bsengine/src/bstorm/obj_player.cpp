﻿#include <bstorm/obj_player.hpp>

#include <bstorm/dnh_const.hpp>
#include <bstorm/virtual_key_input_source.hpp>
#include <bstorm/script.hpp>
#include <bstorm/intersection.hpp>
#include <bstorm/obj_enemy_boss_scene.hpp>
#include <bstorm/obj_shot.hpp>
#include <bstorm/obj_item.hpp>
#include <bstorm/obj_spell.hpp>
#include <bstorm/item_data.hpp>
#include <bstorm/dnh_value.hpp>
#include <bstorm/game_state.hpp>

#undef VK_RIGHT
#undef VK_LEFT
#undef VK_UP
#undef VK_DOWN
#undef VK_CANCEL
#undef VK_PAUSE

namespace bstorm
{
GlobalPlayerParams::GlobalPlayerParams() :
    life(2),
    spell(3),
    power(1),
    score(0),
    graze(0),
    point(0)
{
}

ObjPlayer::ObjPlayer(const std::shared_ptr<GameState>& gameState, const std::shared_ptr<GlobalPlayerParams>& globalParams) :
    ObjSprite2D(gameState),
    ObjMove(this),
    ObjCol(gameState),
    permitPlayerShot_(true),
    permitPlayerSpell_(true),
    state_(STATE_NORMAL),
    normalSpeed_(4.0f),
    slowSpeed_(1.6f),
    clipLeft_(0),
    clipTop_(0),
    clipRight_(384),
    clipBottom_(448),
    invincibilityFrame_(0),
    downStateFrame_(120),
    rebirthFrame_(15),
    rebirthLossFrame_(3),
    globalParams_(globalParams),
    autoItemCollectLineY_(-1),
    hitStateTimer_(0),
    downStateTimer_(0),
    currentFrameGrazeCnt_(0)
{
    SetType(OBJ_PLAYER);
    InitPosition();
}

ObjPlayer::~ObjPlayer() {}

void ObjPlayer::Update()
{
    if (auto gameState = GetGameState())
    {
        if (state_ == STATE_NORMAL)
        {
            MoveByKeyInput();
            ApplyClip();
            if (currentFrameGrazeCnt_ > 0)
            {
                if (auto playerScript = gameState->stagePlayerScript.lock())
                {
                    // Notify EV_GRAZE
                    auto grazeInfo = std::make_unique<DnhArray>();
                    grazeInfo->PushBack(std::make_unique<DnhReal>((double)currentFrameGrazeCnt_));
                    grazeInfo->PushBack(std::make_unique<DnhArray>(currentFrameGrazeObjIds_));
                    grazeInfo->PushBack(std::make_unique<DnhArray>(currentFrameGrazeShotPoints_));
                    playerScript->NotifyEvent(EV_GRAZE, grazeInfo);
                }
            }
        }
        if (state_ == STATE_HIT)
        {
            if (hitStateTimer_ <= 0)
            {
                ShootDown();
            } else
            {
                hitStateTimer_--;
            }
        }

        // ボム入力処理
        // hitStateTimerのカウント処理の後に行う
        if (state_ == STATE_NORMAL || state_ == STATE_HIT)
        {
            int spellKey = gameState->vKeyInputSource->GetVirtualKeyState(VK_SPELL);
            if (spellKey == KEY_PUSH)
            {
                CallSpell();
            }
        }

        if (state_ == STATE_DOWN)
        {
            if (downStateTimer_ <= 0)
            {
                Rebirth();
            } else
            {
                downStateTimer_--;
            }
        }
        if (IsInvincible())
        {
            invincibilityFrame_--;
        } else
        {
            invincibilityFrame_ = 0;
        }
        currentFrameGrazeCnt_ = 0;
        currentFrameGrazeObjIds_.clear();
        currentFrameGrazeShotPoints_.clear();
    }
}

void ObjPlayer::Render()
{
    ObjSprite2D::Render();
    ObjCol::RenderIntersection(IsPermitCamera());
}

void ObjPlayer::AddIntersectionCircleA1(float dx, float dy, float r, float dr)
{
    if (auto gameState = GetGameState())
    {
        ObjCol::PushBackIntersection(gameState->colDetector->Create<PlayerIntersection>(GetX() + dx, GetY() + dy, r, shared_from_this()));
        ObjCol::PushBackIntersection(gameState->colDetector->Create<PlayerGrazeIntersection>(GetX() + dx, GetY() + dy, r + dr, shared_from_this()));
    }
}

void ObjPlayer::AddIntersectionCircleA2(float dx, float dy, float r)
{
    if (auto gameState = GetGameState())
    {
        ObjCol::PushBackIntersection(gameState->colDetector->Create<PlayerGrazeIntersection>(GetX() + dx, GetY() + dy, r, shared_from_this()));
    }
}

void ObjPlayer::AddIntersectionToItem()
{
    if (auto gameState = GetGameState())
    {
        isectToItem_ = gameState->colDetector->Create<PlayerIntersectionToItem>(GetX(), GetY(), shared_from_this());
    }
}

void ObjPlayer::SetNormalSpeed(double speed)
{
    normalSpeed_ = speed;
}

void ObjPlayer::SetSlowSpeed(double speed)
{
    slowSpeed_ = speed;
}

void ObjPlayer::SetClip(float left, float top, float right, float bottom)
{
    clipLeft_ = left;
    clipTop_ = top;
    clipRight_ = right;
    clipBottom_ = bottom;
}

void ObjPlayer::SetLife(double life) { globalParams_->life = life; }

void ObjPlayer::SetSpell(double spell) { globalParams_->spell = spell; }

void ObjPlayer::SetPower(double power) { globalParams_->power = power; }

void ObjPlayer::SetDownStateFrame(int frame)
{
    downStateFrame_ = frame;
}

void ObjPlayer::SetRebirthFrame(int frame)
{
    rebirthFrame_ = frame;
}

void ObjPlayer::SetRebirthLossFrame(int frame)
{
    rebirthLossFrame_ = frame;
}

double ObjPlayer::GetLife() const { return globalParams_->life; }

double ObjPlayer::GetSpell() const { return globalParams_->spell; }

double ObjPlayer::GetPower() const { return globalParams_->power; }

bool ObjPlayer::IsPermitPlayerSpell() const
{
    if (auto gameState = GetGameState())
    {
        auto bossScene = gameState->enemyBossSceneObj.lock();
        if (bossScene && bossScene->IsLastSpell())
        {
            return false;
        }
    }
    return permitPlayerSpell_;
}

bool ObjPlayer::IsLastSpellWait() const
{
    return GetState() == STATE_HIT;
}

bool ObjPlayer::IsSpellActive() const
{
    if (auto gameState = GetGameState())
    {
        if (gameState->spellManageObj.lock())
        {
            return true;
        }
    }
    return false;
}

int64_t ObjPlayer::GetScore() const { return globalParams_->score; }

int64_t ObjPlayer::GetGraze() const { return globalParams_->graze; }

int64_t ObjPlayer::GetPoint() const { return globalParams_->point; }

void ObjPlayer::AddScore(int64_t score) { globalParams_->score += score; }

void ObjPlayer::AddGraze(int64_t graze) { globalParams_->graze += graze; }

void ObjPlayer::AddGraze(int shotObjId, int64_t graze)
{
    if (auto gameState = GetGameState())
    {
        if (auto shot = gameState->objTable->Get<ObjShot>(shotObjId))
        {
            globalParams_->graze += graze;
            currentFrameGrazeCnt_ += graze;
            currentFrameGrazeObjIds_.push_back((double)shot->GetID());
            currentFrameGrazeShotPoints_.emplace_back(shot->GetX(), shot->GetY());
        }
    }
}

void ObjPlayer::AddPoint(int64_t point) { globalParams_->point += point; }

void ObjPlayer::Hit(int collisionObjId)
{
    if (auto gameState = GetGameState())
    {
        if (gameState->forcePlayerInvincibleEnable) return;
        if (state_ == STATE_NORMAL && !IsInvincible())
        {
            state_ = STATE_HIT;
            hitStateTimer_ = rebirthFrame_;
            // NOTE: 状態を変更してからイベントを送る
            if (auto playerScript = gameState->stagePlayerScript.lock())
            {
                playerScript->NotifyEvent(EV_HIT, std::make_unique<DnhArray>(std::vector<double>{ (double)collisionObjId }));
            }
            // アイテム自動回収をキャンセル
            gameState->autoItemCollectionManager->CancelCollectItems();
        }
    }
}

void ObjPlayer::TransIntersection(float dx, float dy)
{
    if (auto gameState = GetGameState())
    {
        ObjCol::TransIntersection(dx, dy);
        if (isectToItem_)
        {
            gameState->colDetector->Trans(isectToItem_, dx, dy);
        }
    }
}

bool ObjPlayer::IsInvincible() const
{
    return invincibilityFrame_ > 0;
}

void ObjPlayer::ShootDown()
{
    downStateTimer_ = downStateFrame_;
    globalParams_->life--;
    if (auto gameState = GetGameState())
    {
        if (auto bossScene = gameState->enemyBossSceneObj.lock())
        {
            bossScene->AddPlayerShootDownCount(1);
        }
        gameState->scriptManager->NotifyEventAll(EV_PLAYER_SHOOTDOWN);
    }
    // Eventを送ってから状態を変更する
    if (globalParams_->life >= 0)
    {
        state_ = STATE_DOWN;
    } else
    {
        state_ = STATE_END;
    }
    SetVisible(false);
}

void ObjPlayer::Rebirth()
{
    state_ = STATE_NORMAL;
    SetVisible(true);
    if (auto gameState = GetGameState())
    {
        InitPosition();
        gameState->scriptManager->NotifyEventAll(EV_PLAYER_REBIRTH);
    }
}

void ObjPlayer::MoveByKeyInput()
{
    if (auto gameState = GetGameState())
    {
        auto r = gameState->vKeyInputSource->GetVirtualKeyState(VK_RIGHT);
        auto l = gameState->vKeyInputSource->GetVirtualKeyState(VK_LEFT);
        auto u = gameState->vKeyInputSource->GetVirtualKeyState(VK_UP);
        auto d = gameState->vKeyInputSource->GetVirtualKeyState(VK_DOWN);

        auto shift = gameState->vKeyInputSource->GetVirtualKeyState(VK_SLOWMOVE);
        bool isSlowMode = shift == KEY_HOLD || shift == KEY_PUSH;
        float speed = (isSlowMode ? slowSpeed_ : normalSpeed_);

        int dx = 0;
        int dy = 0;

        if (r == KEY_HOLD || r == KEY_PUSH) { dx++; }
        if (l == KEY_HOLD || l == KEY_PUSH) { dx--; }
        if (u == KEY_HOLD || u == KEY_PUSH) { dy--; }
        if (d == KEY_HOLD || d == KEY_PUSH) { dy++; }

        SetMovePosition(GetX() + speed * dx, GetY() + speed * dy);
    }
}

void ObjPlayer::ApplyClip()
{
    SetMovePosition(std::min(std::max(GetX(), clipLeft_), clipRight_), std::min(std::max(GetY(), clipTop_), clipBottom_));
}

void ObjPlayer::InitPosition()
{
    if (auto gameState = GetGameState())
    {
        SetMovePosition((gameState->stgFrame->right - gameState->stgFrame->left) / 2.0f, gameState->stgFrame->bottom - 48.0f);
    }
}

void ObjPlayer::CallSpell()
{
    auto gameState = GetGameState();
    if (!gameState) return;
    auto playerScript = gameState->stagePlayerScript.lock();
    auto spellManageObj = gameState->spellManageObj.lock();
    if ((!spellManageObj || spellManageObj->IsDead()) && IsPermitPlayerSpell() && playerScript)
    {
        gameState->spellManageObj = gameState->objTable->Create<ObjSpellManage>(gameState);
        playerScript->NotifyEvent(EV_REQUEST_SPELL);
        if (playerScript->GetScriptResult()->ToBool())
        {
            // スペル発動
            if (auto bossScene = gameState->enemyBossSceneObj.lock())
            {
                bossScene->AddPlayerSpellCount(1);
            }
            if (state_ == STATE_HIT)
            {
                // 喰らいボム
                rebirthFrame_ = std::max(0, rebirthFrame_ - rebirthLossFrame_);
                state_ = STATE_NORMAL;
            }
            gameState->scriptManager->NotifyEventAll(EV_PLAYER_SPELL);
        } else
        {
            // スペル不発
            if (gameState->spellManageObj.lock())
            {
                gameState->objTable->Delete(gameState->spellManageObj.lock()->GetID());
            }
        }
    }
}

void ObjPlayer::ObtainItem(int itemObjId)
{
    if (auto gameState = GetGameState())
    {
        if (auto item = gameState->objTable->Get<ObjItem>(itemObjId))
        {
            if (item->IsScoreItem())
            {
                AddScore(item->GetScore());
            }
            // ボーナスアイテムの場合はイベントを送らない
            if (item->GetItemType() != ITEM_DEFAULT_BONUS)
            {
                int itemType = -1;
                if (item->GetItemType() == ITEM_USER)
                {
                    if (auto itemData = item->GetItemData()) itemType = itemData->type;
                } else
                {
                    itemType = item->GetItemType();
                }
                if (auto gameState = GetGameState())
                {
                    // EV_GET_ITEM
                    auto evArgs = std::make_unique<DnhArray>(std::vector<double>{ (double)itemType, (double)item->GetID() });
                    if (auto playerScript = gameState->stagePlayerScript.lock())
                    {
                        playerScript->NotifyEvent(EV_GET_ITEM, evArgs);
                    }
                    if (auto itemScript = gameState->itemScript.lock())
                    {
                        itemScript->NotifyEvent(EV_GET_ITEM, evArgs);
                    }
                }
            }
        }
    }
}

bool ObjPlayer::IsGrazeEnabled() const
{
    return state_ == STATE_NORMAL && !IsInvincible();
}
}