// Relics / Реликвии: пассивки, которые меняют правила, а не числа.
#pragma once
#include "core/Cards.h"

namespace shichoy {

enum class RelicId : int {
    None = 0,
    ExtraSlot,      // +1 слот хода
    DeepDraw,       // +2 к добору каждый ход
    FirstFree,      // первая карта за ход не тратит слот
    LingeringMark,  // метка на враге больше не спадает по времени
    ReactionEcho,   // реакции бьют дважды по 60%
    SpotHunter,     // по уязвимым точкам не бывает промаха
    BloodPact,      // +30% урона, но каждая карта стоит 2 HP
    Hoarder,        // одна карта из руки остаётся в колоде после боя
    Undying,        // раз за бой переживаешь смертельный удар с 1 HP
    Gravedigger,    // разрушенная точка даёт вдвое больше материалов
    Alembic,        // слияние сохраняет цепочку: составная карта уходит в колоду
    COUNT
};

struct RelicDef {
    RelicId id = RelicId::None;
    LS name;
    LS desc;
    Color tint = WHITE;
};

const RelicDef& relicDef(RelicId id);
// Случайная реликвия из тех, которых ещё нет. None — если собраны все.
RelicId rollRelic(Rng& r, const std::vector<RelicId>& owned);

} // namespace shichoy
