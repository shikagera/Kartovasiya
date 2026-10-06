#include "world/Locale.h"

namespace shichoy {

static std::vector<LocaleDef> buildLocales() {
    std::vector<LocaleDef> v;
    v.reserve(8);
    auto add = [&](LS n, LS d) -> LocaleDef& {
        v.push_back(LocaleDef{});
        LocaleDef& L = v.back();
        L.name = std::move(n); L.desc = std::move(d);
        return L;
    };

    {   // ── Лес: исходная местность, теперь теплее и гуще ─────────────────
        LocaleDef& L = add(LS("Старый лес", "The Old Wood"),
                           LS("Дубы стоят вплотную, под ногами папоротник и валежник.",
                              "Oaks stand close; fern and deadfall underfoot."));
        L.grass = hex(0x4E7A3A); L.darkGrass = hex(0x33522C);
        L.path  = hex(0x8A6A42); L.mud       = hex(0x5E452A);
        L.water = hex(0x2A5A86); L.rock      = hex(0x6A6A74);
        L.tree  = hex(0x3A5E2E); L.bramble   = hex(0x4A4028);
        L.grave = hex(0x535660); L.flagstone = hex(0x3A3A44);
        L.accent = hex(0x8FBF5E);
        L.treeStyle = TreeStyle::Oak;
    }
    {   // ── Снежник ───────────────────────────────────────────────────────
        LocaleDef& L = add(LS("Снежник", "The Snowfield"),
                           LS("Наст держит не везде. Под ним слышно воду.",
                              "The crust does not hold everywhere. Water runs beneath."));
        L.grass = hex(0xD6DEE8); L.darkGrass = hex(0xAEBCCC);
        L.path  = hex(0x9AA6B4); L.mud       = hex(0x76828E);
        L.water = hex(0x3E7CA8); L.rock      = hex(0x7C8696);
        L.tree  = hex(0x2E4A3C); L.bramble   = hex(0x6A7280);
        L.grave = hex(0x8A94A2); L.flagstone = hex(0x68727E);
        L.accent = hex(0xFFFFFF);
        L.treeStyle = TreeStyle::Fir;
        L.snowK = 0.75f;
    }
    {   // ── Побережье ─────────────────────────────────────────────────────
        LocaleDef& L = add(LS("Побережье", "The Shore"),
                           LS("Песок горячий даже в тени. Море выносит чужое.",
                              "The sand is hot even in shade. The sea brings things ashore."));
        L.grass = hex(0xD9C07E); L.darkGrass = hex(0xB89C5E);
        L.path  = hex(0xE8D7A4); L.mud       = hex(0x9A7F4E);
        L.water = hex(0x2E9AB8); L.rock      = hex(0x9A9080);
        L.tree  = hex(0x5E8A3E); L.bramble   = hex(0xA8926A);
        L.grave = hex(0x8E8676); L.flagstone = hex(0xAFA48C);
        L.accent = hex(0xFFF0C0);
        L.treeStyle = TreeStyle::Palm;
    }
    {   // ── Джунгли ───────────────────────────────────────────────────────
        LocaleDef& L = add(LS("Джунгли", "The Jungle"),
                           LS("Лист закрывает небо. Дышать тяжело, видно недалеко.",
                              "Leaves close off the sky. The air is thick, the view short."));
        L.grass = hex(0x2E6B2A); L.darkGrass = hex(0x1E4A20);
        L.path  = hex(0x6A5634); L.mud       = hex(0x46381F);
        L.water = hex(0x1E6E64); L.rock      = hex(0x5A6456);
        L.tree  = hex(0x25551F); L.bramble   = hex(0x3A5A24);
        L.grave = hex(0x4A5248); L.flagstone = hex(0x3A4438);
        L.accent = hex(0x7EDC5A);
        L.treeStyle = TreeStyle::Canopy;
    }
    {   // ── Усадьба: место героя, врагов нет ──────────────────────────────
        LocaleDef& L = add(LS("Усадьба", "The Estate"),
                           LS("Здесь никто не нападёт. Пока здесь и делать особо нечего.",
                              "Nothing attacks here. There is also little to do here yet."));
        L.grass = hex(0x5A7A46); L.darkGrass = hex(0x44603A);
        L.path  = hex(0xA09070); L.mud       = hex(0x6E5A3E);
        L.water = hex(0x3A6E96); L.rock      = hex(0x7A7682);
        L.tree  = hex(0x3E6634); L.bramble   = hex(0x58603C);
        L.grave = hex(0x6A6E78); L.flagstone = hex(0x8A8694);
        L.accent = hex(0xD9C98E);
        L.treeStyle = TreeStyle::Hedge;
        L.peaceful = true;
    }
    return v;
}

static const std::vector<LocaleDef>& table() {
    static const std::vector<LocaleDef> t = buildLocales();
    return t;
}

const LocaleDef& localeDef(Locale l) {
    const auto& t = table();
    size_t i = size_t(l);
    return (i < t.size()) ? t[i] : t[0];
}

LS localeName(Locale l) { return localeDef(l).name; }

} // namespace shichoy
