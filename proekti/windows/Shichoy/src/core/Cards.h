// Cards / Карты: описание, процедурная генерация, колода
#pragma once
#include "core/Content.h"

namespace shichoy {

// Прямые действия карт-заклинаний.
enum class SpellEffect : int {
    Damage = 0,   // прямой урон
    Pierce,       // урон строго по выбранной точке (точный удар)
    Drain,        // урон + лечение
    Heal,         // лечение
    Amplify,      // усиливает следующую реакцию
    Detonate,     // подрывает метку без новой стихии
    Volley,       // несколько слабых ударов
    COUNT
};

// Немагические эффекты.
enum class EffectKind : int {
    Shield = 0,   // щит
    Draw,         // добор карт
    LimitUp,      // +лимит карт на ход
    Stun,         // оглушение врага
    Focus,        // точность/крит
    Cleanse,      // снять свои дебаффы
    Weaken,       // ослабить врага
    Regen,        // регенерация
    COUNT
};

// Лестница смешивания карт прямо в руке.
// Base — чистая стихия; дальше карта копит цепочку и множитель, вершина —
// именная Ultima, разыгрываемая один раз за бой.
enum class MergeTier : int {
    Base = 0,   // одна стихия              x1
    T1,         // база + база              x2   каноничная реакция
    T2,         // ур.1 + новая стихия      x3
    T2plus,     // ур.1 + ур.1              x3.75
    T3,         // ур.2 + стихия/ур.1       x5   бьёт по всем точкам
    T3plus,     // ур.2 + ур.2              x6.5 + оглушение
    Ultima      // ур.3 + ур.3, 5+ стихий   x10 + % текущего HP
};
int   tierRank(MergeTier t);        // 0..4 — «уровень» из таблицы
float tierMult(MergeTier t);
const char* tierLabel(MergeTier t); // I, II, II+, III, III+, ULT
LS    tierName(MergeTier t);

struct Card {
    uint32_t  uid      = 0;
    CardType  type     = CardType::Element;
    ElementId element  = ElementId::None;
    MergeTier tier     = MergeTier::Base;
    std::vector<ElementId> chain;   // стихии в порядке попадания в карту
    LS        name;
    LS        text;
    CardRarity rarity  = CardRarity::Common;
    SpellEffect spell  = SpellEffect::Damage;
    EffectKind  effect = EffectKind::Shield;
    int power  = 6;    // основное число (урон / лечение / щит)
    int amount = 1;    // вторичное число (стаки, ходы, добор)
    int cost   = 1;    // сколько «слотов хода» съедает
    int level  = 0;    // уровень апгрейда (крафт)
    CardFlags flags = CardFlags::None;

    Color tint() const;
    // Пересобирает name/text после мутации или апгрейда.
    void refreshText();
};

// ── Procedural generation / Процедурная генерация ─────────────────────────
CardRarity rollRarity(Rng& r, int depth);
Card genElementCard(Rng& r, int depth, ElementId forced = ElementId::None);
Card genSpellCard(Rng& r, int depth);
Card genEffectCard(Rng& r, int depth);
Card genCard(Rng& r, int depth);
void mutateCard(Card& c, Rng& r, int depth);   // добавляет аффикс/сдвигает числа
void upgradeCard(Card& c);                     // +1 уровень от крафта

std::vector<Card> starterDeck(Rng& r);

// ── Deck / Колода забега ──────────────────────────────────────────────────
class Deck {
public:
    void reset(std::vector<Card> cards);
    void shuffleDraw(Rng& r);
    void beginCombat(Rng& r);
    bool  draw(Card& out, Rng& r);   // false если карт нет даже после перетасовки сброса
    void  discard(const Card& c);
    void  exhaust(const Card& c);
    const std::vector<Card>& library() const { return library_; }
    std::vector<Card>& libraryMut() { return library_; }
    int drawCount() const { return int(drawPile_.size()); }
    int discardCount() const { return int(discardPile_.size()); }
    void addCard(const Card& c) { library_.push_back(c); }
    void removeCard(int i) {
        if (i >= 0 && i < int(library_.size())) library_.erase(library_.begin() + i);
    }
private:
    std::vector<Card> library_;      // всё, что есть у игрока в забеге
    std::vector<Card> drawPile_;
    std::vector<Card> discardPile_;
    std::vector<Card> exhausted_;
};

} // namespace shichoy
