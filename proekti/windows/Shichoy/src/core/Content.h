// Static game content / Статический контент: стихии, реакции, статусы, материалы
#pragma once
#include "core/Types.h"

namespace shichoy {

// ── Statuses / Статусы ────────────────────────────────────────────────────
enum class StatusId : int {
    None = 0,
    Burn,        // урон в конце хода
    Bleed,       // урон в конце хода, растёт от ударов
    Poisoned,    // урон + режет броню
    Freeze,      // пропуск действия
    Stun,        // пропуск действия (немагический)
    Shock,       // усиление следующего урона по цели
    Corrode,     // -защита
    Vulnerable,  // +входящий урон
    Weaken,      // -урон врага
    Slow,        // враг пропускает шаг паттерна
    Regen,       // лечение в конце хода (игрок)
    Shield,      // поглощение урона (игрок)
    Focus,       // +крит/точность (игрок)
    COUNT
};

struct StatusDef { L name; L desc; Color tint; bool onPlayer; };
const StatusDef& statusDef(StatusId s);

struct StatusInst {
    StatusId id = StatusId::None;
    int stacks = 0;    // сила
    int turns  = 0;    // длительность (-1 = постоянный)
};

// ── Elements / Стихии ─────────────────────────────────────────────────────
struct ElementDef {
    ElementId id;
    ElementTier tier;
    L name;
    L desc;
    Color primary;
    Color accent;
    ElementId opposed;
    const char* glyph;      // ASCII-safe сигил
    StatusId mark;          // статус, который вешает метка
};
const ElementDef& elem(ElementId id);
const std::vector<ElementDef>& allElements();
ElementId randomElement(Rng& r, int depth);   // расширенные стихии — глубже

// ── Reactions / Реакции (Element + Element) ───────────────────────────────
enum class ReactionKind : int {
    Damage = 0,   // чистый бонусный урон
    Dot,          // навешивает DoT
    Control,      // заморозка / стан / замедление
    Debuff,       // уязвимость / коррозия / ослабление
    Drain,        // урон + лечение игрока
    Risky         // сильный урон + шанс отдачи
};

struct ReactionDef {
    ElementId a = ElementId::None, b = ElementId::None;
    LS name;
    LS desc;
    ReactionKind kind = ReactionKind::Damage;
    int   bonus = 8;          // плоский бонус к урону
    float mult  = 1.3f;       // множитель урона карты-триггера
    float risk  = 0.0f;       // шанс отдачи по игроку
    StatusId applies = StatusId::None;
    int   statusStacks = 0;
    int   statusTurns  = 0;
    Color tint = WHITE;
    bool  valid = false;
};

// Возвращает реакцию для пары (порядок не важен). Всегда валидна:
// для незаданных пар генерируется «Диссонанс» из свойств стихий.
ReactionDef reactionFor(ElementId a, ElementId b);

// ── Materials / Материалы крафта ──────────────────────────────────────────
struct MaterialDef { L name; Color tint; };
const MaterialDef& material(int id);
constexpr int MATERIAL_COUNT = 8;
enum MaterialId : int {
    MAT_ASH = 0,    // Пепел
    MAT_BONE,       // Кость
    MAT_ICHOR,      // Ихор
    MAT_SILVER,     // Серебро
    MAT_SHARD,      // Осколок
    MAT_VELLUM,     // Пергамент
    MAT_SIGIL,      // Печать
    MAT_HEARTWOOD   // Сердцевина
};

// ── Name pools / Пулы имён для процедурной генерации ──────────────────────
const std::vector<LS>& spellNouns();
const std::vector<LS>& spellPrefixes();
const std::vector<LS>& effectNouns();
const std::vector<LS>& enemyPrefixes();
const std::vector<LS>& enemyNouns();
const std::vector<LS>& affixNames();

} // namespace shichoy
