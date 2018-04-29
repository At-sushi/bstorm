﻿#include <bstorm/obj_spell.hpp>

#include <bstorm/dnh_const.hpp>
#include <bstorm/intersection.hpp>
#include <bstorm/game_state.hpp>

namespace bstorm
{
ObjSpell::ObjSpell(const std::shared_ptr<GameState>& gameState) :
    ObjPrim2D(gameState),
    ObjCol(gameState),
    damage_(0),
    isRegistered_(false),
    eraseShotEnable_(true)
{
    SetType(OBJ_SPELL);
}

ObjSpell::~ObjSpell() {}

void ObjSpell::Update()
{
    ClearOldTempIntersection();
}

void ObjSpell::Render()
{
    if (IsRegistered())
    {
        ObjPrim2D::Render();
        ObjCol::RenderIntersection(IsPermitCamera());
    }
}

bool ObjSpell::IsRegistered() const
{
    return isRegistered_;
}

void ObjSpell::Regist() { isRegistered_ = true; }

double ObjSpell::GetDamage() const { return damage_; }

void ObjSpell::SetDamage(double damage) { this->damage_ = damage; }

bool ObjSpell::IsEraseShotEnabled() const { return eraseShotEnable_; }

void ObjSpell::SetEraseShotEnable(bool enable) { eraseShotEnable_ = enable; }

void ObjSpell::AddTempIntersection(const std::shared_ptr<SpellIntersection>& isect)
{
    if (auto gameState = GetGameState())
    {
        gameState->colDetector->Add(isect);
        ObjCol::AddTempIntersection(isect);
    }
}

void ObjSpell::AddTempIntersectionCircle(float x, float y, float r)
{
    AddTempIntersection(std::make_shared<SpellIntersection>(x, y, r, shared_from_this()));
}

void ObjSpell::AddTempIntersectionLine(float x1, float y1, float x2, float y2, float width)
{
    AddTempIntersection(std::make_shared<SpellIntersection>(x1, y1, x2, y2, width, shared_from_this()));
}
ObjSpellManage::ObjSpellManage(const std::shared_ptr<GameState>& gameState) :
    Obj(gameState)
{
    SetType(OBJ_SPELL_MANAGE);
}
ObjSpellManage::~ObjSpellManage()
{
}
void ObjSpellManage::Update() {}
}