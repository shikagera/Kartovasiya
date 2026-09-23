// Crafting / Крафт: материалы → карты. Успех решает мини-игра (потолок 70%).
#pragma once
#include "core/Cards.h"
#include "craft/MiniGames.h"

namespace shichoy {

struct Inventory {
    int mats[MATERIAL_COUNT] = { 0 };
    int  get(int id) const { return mats[clampi(id, 0, MATERIAL_COUNT - 1)]; }
    void add(int id, int n) { mats[clampi(id, 0, MATERIAL_COUNT - 1)] += n; }
    bool canPay(const int cost[MATERIAL_COUNT]) const;
    void pay(const int cost[MATERIAL_COUNT]);
    int  total() const;
};

enum class CraftAction : int {
    Forge = 0,    // выковать новую карту
    Upgrade,      // усилить существующую
    Mutate,       // рискованная мутация
    Transmute,    // переплавить лишние материалы
    Burn          // сжечь карту и вернуть часть материала
};

struct CraftPlan {
    CraftAction action = CraftAction::Forge;
    int cost[MATERIAL_COUNT] = { 0 };
    float chance = 0.55f;        // базовая вероятность, потолок 0.70
    int targetCard = -1;         // индекс в библиотеке колоды
    LS title;
    LS desc;
};

CraftPlan makePlan(CraftAction a, int depth, const Deck& deck, int targetCard, Rng& r);

// ── Timing mini-game / Мини-игра на тайминг ───────────────────────────────
// Полоса, по которой бегает маркер. Зона успеха = chance (максимум 70%),
// внутри неё узкая «идеальная» зона — бонус к результату.
class TimingMiniGame {
public:
    void start(float chance, Rng& r);
    void update(float dt);
    void commit();                 // игрок нажал
    bool running() const { return running_; }
    bool finished() const { return finished_; }
    bool success() const { return success_; }
    bool perfect() const { return perfect_; }
    void reset() { running_ = false; finished_ = false; }

    float marker() const { return marker_; }
    float zoneStart() const { return zoneStart_; }
    float zoneEnd() const { return zoneEnd_; }
    float perfectStart() const { return perfStart_; }
    float perfectEnd() const { return perfEnd_; }
    float speed() const { return speed_; }

private:
    float marker_ = 0.0f, dir_ = 1.0f, speed_ = 1.1f;
    float zoneStart_ = 0.3f, zoneEnd_ = 0.7f;
    float perfStart_ = 0.47f, perfEnd_ = 0.53f;
    bool  running_ = false, finished_ = false, success_ = false, perfect_ = false;
    float holdAfter_ = 0.0f;
};

// Применяет результат мини-игры. Возвращает текст исхода.
LS applyCraft(const CraftPlan& plan, bool success, bool perfect,
              Deck& deck, Inventory& inv, int depth, Rng& r);

} // namespace shichoy
