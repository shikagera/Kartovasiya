// Locale / Местность: куда именно уехал герой.
//
// Биом (Biome в core/Biome.h) — это тайл под ногами, он решает исход боя.
// Местность — другое: это целый регион со своей палитрой земли, своей флорой
// и своими правилами заселения. Между местностями герой путешествует.
#pragma once
#include "core/Types.h"

namespace shichoy {

enum class Locale : int {
    Forest = 0,   // стартовый лес: дубы, папоротник, валежник
    Snow,         // снежник: ели под шапками, наст, полыньи
    Beach,        // побережье: песок, пальмы, плавник, ракушки
    Jungle,       // джунгли: лианы, широкий лист, духота
    Estate,       // усадьба героя: тихо, врагов нет
    COUNT
};

// Как рисуется дерево в этой местности.
enum class TreeStyle : int { Oak = 0, Fir, Palm, Canopy, Hedge };

struct LocaleDef {
    LS name, desc;
    // Палитра земли: ровно те тайлы, что знает Overworld.
    Color grass, darkGrass, path, mud, water, rock, tree, bramble, grave, flagstone;
    Color accent;          // мелкая крошка и подсветка
    TreeStyle treeStyle = TreeStyle::Oak;
    bool  peaceful = false;   // в усадьбе не водятся бродячие враги
    float snowK = 0.0f;       // доля белого налёта поверх всего
};

const LocaleDef& localeDef(Locale l);
LS localeName(Locale l);

} // namespace shichoy
