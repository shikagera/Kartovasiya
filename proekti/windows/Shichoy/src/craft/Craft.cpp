#include "craft/Craft.h"
#include <algorithm>

namespace shichoy {

constexpr float CRAFT_CHANCE_CAP = 0.70f;   // потолок успеха по концепту

bool Inventory::canPay(const int cost[MATERIAL_COUNT]) const {
    for (int i = 0; i < MATERIAL_COUNT; ++i) if (mats[i] < cost[i]) return false;
    return true;
}
void Inventory::pay(const int cost[MATERIAL_COUNT]) {
    for (int i = 0; i < MATERIAL_COUNT; ++i) mats[i] = std::max(0, mats[i] - cost[i]);
}
int Inventory::total() const {
    int s = 0; for (int i = 0; i < MATERIAL_COUNT; ++i) s += mats[i];
    return s;
}

CraftPlan makePlan(CraftAction a, int depth, const Deck& deck, int targetCard, Rng& r) {
    CraftPlan p;
    p.action = a;
    p.targetCard = targetCard;

    auto cost = [&](int kinds, int lo, int hi) {
        for (int k = 0; k < kinds; ++k)
            p.cost[r.i(0, MATERIAL_COUNT - 1)] += r.i(lo, hi);
    };

    switch (a) {
        case CraftAction::Forge:
            p.title = LS("Выковать карту", "Forge a Card");
            p.desc  = LS("Новая процедурная карта. Провал — материалы сгорают наполовину.",
                         "A new procedural card. On failure half the materials are lost.");
            cost(2, 2, 3);
            p.chance = 0.55f;
            break;
        case CraftAction::Upgrade:
            p.title = LS("Усилить карту", "Upgrade a Card");
            p.desc  = LS("+1 уровень выбранной карте.",
                         "+1 level on the chosen card.");
            cost(2, 1, 3);
            p.chance = 0.60f;
            break;
        case CraftAction::Mutate:
            p.title = LS("Мутировать карту", "Mutate a Card");
            p.desc  = LS("Случайный аффикс и сдвиг чисел. При провале карта портится.",
                         "A random affix and shifted numbers. On failure the card is spoiled.");
            cost(3, 1, 2);
            p.chance = 0.45f;
            break;
        case CraftAction::Transmute:
            p.title = LS("Переплавить материалы", "Transmute Materials");
            p.desc  = LS("Меняет излишки на редкие материалы.",
                         "Turns surplus into rarer materials.");
            cost(1, 4, 6);
            p.chance = 0.65f;
            break;
    }
    // Глубже — сложнее, но потолок всё равно 70%
    p.chance = clampf(p.chance + depth * 0.01f, 0.20f, CRAFT_CHANCE_CAP);
    (void)deck;
    return p;
}

// ── Mini-game ─────────────────────────────────────────────────────────────
void TimingMiniGame::start(float chance, Rng& r) {
    float c = clampf(chance, 0.05f, CRAFT_CHANCE_CAP);
    float width = c;                                   // ширина зоны = вероятность
    zoneStart_ = r.f(0.05f, 0.95f - width);
    zoneEnd_ = zoneStart_ + width;
    float pw = std::max(0.03f, width * 0.18f);
    float pc = (zoneStart_ + zoneEnd_) * 0.5f + r.f(-width * 0.2f, width * 0.2f);
    perfStart_ = clampf(pc - pw * 0.5f, zoneStart_, zoneEnd_);
    perfEnd_ = clampf(pc + pw * 0.5f, zoneStart_, zoneEnd_);

    marker_ = r.f(0.0f, 1.0f);
    dir_ = r.chance(0.5f) ? 1.0f : -1.0f;
    speed_ = 0.85f + (1.0f - c) * 0.9f;                // ниже шанс — быстрее бегунок
    running_ = true; finished_ = false; success_ = false; perfect_ = false;
    holdAfter_ = 0.0f;
}

void TimingMiniGame::update(float dt) {
    if (finished_) { holdAfter_ += dt; return; }
    if (!running_) return;
    marker_ += dir_ * speed_ * dt;
    if (marker_ > 1.0f) { marker_ = 1.0f; dir_ = -1.0f; }
    if (marker_ < 0.0f) { marker_ = 0.0f; dir_ = 1.0f; }
}

void TimingMiniGame::commit() {
    if (!running_ || finished_) return;
    finished_ = true;
    running_ = false;
    success_ = (marker_ >= zoneStart_ && marker_ <= zoneEnd_);
    perfect_ = success_ && (marker_ >= perfStart_ && marker_ <= perfEnd_);
}

// ── Applying results ──────────────────────────────────────────────────────
LS applyCraft(const CraftPlan& plan, bool success, bool perfect,
              Deck& deck, Inventory& inv, int depth, Rng& r) {
    inv.pay(plan.cost);
    auto& lib = deck.libraryMut();
    int idx = plan.targetCard;
    bool validTarget = idx >= 0 && idx < int(lib.size());

    if (!success) {
        switch (plan.action) {
            case CraftAction::Mutate:
                if (validTarget) {
                    Card& c = lib[size_t(idx)];
                    c.power = std::max(1, int(c.power * 0.8f));
                    c.flags |= CardFlags::Corrupted;
                    c.refreshText();
                    return LS("Мутация сорвалась: карта испорчена.",
                              "The mutation slipped: the card is spoiled.");
                }
                break;
            case CraftAction::Transmute:
                inv.add(r.i(0, MATERIAL_COUNT - 1), 1);
                return LS("Переплавка не удалась — остался шлак.",
                          "The transmutation failed — only slag remains.");
            default: break;
        }
        return LS("Неудача. Материалы потрачены впустую.",
                  "Failure. The materials are wasted.");
    }

    switch (plan.action) {
        case CraftAction::Forge: {
            Card c = genCard(r, depth + (perfect ? 2 : 0));
            if (perfect) mutateCard(c, r, depth);
            deck.addCard(c);
            return LS(std::string(perfect ? "Идеально! Выкована: " : "Выкована: ") + c.name.ru,
                      std::string(perfect ? "Perfect! Forged: " : "Forged: ") + c.name.en);
        }
        case CraftAction::Upgrade: {
            if (!validTarget) return LS("Нечего усиливать.", "Nothing to upgrade.");
            Card& c = lib[size_t(idx)];
            upgradeCard(c);
            if (perfect) upgradeCard(c);
            return LS(std::string("Усилено: ") + c.name.ru + (perfect ? " (дважды!)" : ""),
                      std::string("Upgraded: ") + c.name.en + (perfect ? " (twice!)" : ""));
        }
        case CraftAction::Mutate: {
            if (!validTarget) return LS("Нечего мутировать.", "Nothing to mutate.");
            Card& c = lib[size_t(idx)];
            mutateCard(c, r, depth);
            if (perfect) mutateCard(c, r, depth);
            return LS(std::string("Мутация: ") + c.name.ru,
                      std::string("Mutated: ") + c.name.en);
        }
        case CraftAction::Transmute: {
            int gained = perfect ? 5 : 3;
            for (int i = 0; i < gained; ++i) inv.add(r.i(0, MATERIAL_COUNT - 1), 1);
            return LS("Переплавка удалась.", "Transmutation succeeded.");
        }
    }
    return LS("", "");
}

} // namespace shichoy
