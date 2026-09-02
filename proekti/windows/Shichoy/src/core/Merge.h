// Card merging / Смешивание карт прямо в руке.
// Перетаскивание карты на карту поднимает результат по лестнице уровней:
//   0  чистая стихия                                  x1
//   1  база + база (каноничная реакция)                x2
//   2  ур.1 + новая стихия                             x3
//   2+ ур.1 + ур.1                                     x3.75
//   3  ур.2 + стихия или ур.1                          x5
//   3+ ур.2 + ур.2                                     x6.5
//   4  ур.3 + ур.3, минимум 5 разных стихий — Ultima   x10 + % HP
#pragma once
#include "core/Cards.h"

namespace shichoy {

struct MergeInfo {
    bool ok = false;
    MergeTier tier = MergeTier::Base;
    float mult = 1.0f;
    int   power = 0;
    int   cost = 1;
    std::vector<ElementId> chain;
    LS    name;      // как будет называться результат
    LS    effect;    // что он делает
    LS    reason;    // почему нельзя, если ok == false
};

// a — карта, на которую бросают; b — та, которую тащат.
MergeInfo evaluateMerge(const Card& a, const Card& b);
Card      applyMerge(const Card& a, const Card& b, const MergeInfo& info);

} // namespace shichoy
