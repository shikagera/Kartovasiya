// Biomes / Биомы: где принят бой, то и помогает.
// Связывает оверворлд с боевой сценой — исследование становится подготовкой.
#pragma once
#include "core/Content.h"

namespace shichoy {

enum class Biome : int {
    Meadow = 0,   // луг
    Rot,          // гниль
    Mire,         // топь
    Stone,        // камни
    Thicket,      // чаща
    Bramble,      // терновник
    Graves,       // могильник
    COUNT
};

struct BiomeMod {
    LS name;
    LS flavour;
    ElementId a = ElementId::None;   // две усиленные стихии
    ElementId b = ElementId::None;
    float bonus = 0.30f;             // прибавка к урону таких карт
    Color tint = WHITE;
};

const BiomeMod& biomeMod(Biome b);
// true, если стихия усилена этим местом
bool biomeFavours(Biome b, ElementId e);

} // namespace shichoy
