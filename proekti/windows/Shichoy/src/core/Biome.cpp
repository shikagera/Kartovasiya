#include "core/Biome.h"

namespace shichoy {

static const BiomeMod kBiomes[int(Biome::COUNT)] = {
    { LS("Луг", "Meadow"),
      LS("Открыто и ветрено — есть где размахнуться.",
         "Open and windy — room to swing."),
      ElementId::Air, ElementId::Light, 0.30f, hex(0x9CC7D6) },

    { LS("Гниль", "Rot"),
      LS("Земля здесь тронута порчей и тянет к себе тьму.",
         "The ground is tainted here, and pulls the dark closer."),
      ElementId::Shadow, ElementId::Aether, 0.30f, hex(0x5A189A) },

    { LS("Топь", "Mire"),
      LS("Вода стоит по щиколотку и всё держит.",
         "Ankle-deep water holds on to everything."),
      ElementId::Water, ElementId::Poison, 0.30f, hex(0x0077B6) },

    { LS("Камни", "Stone"),
      LS("Голый камень и железные жилы под ним.",
         "Bare stone with iron veins beneath."),
      ElementId::Earth, ElementId::Metal, 0.30f, hex(0x6C757D) },

    { LS("Чаща", "Thicket"),
      LS("Корни и сушняк — вспыхнет от одной искры.",
         "Roots and deadwood — one spark takes it all."),
      ElementId::Nature, ElementId::Fire, 0.30f, hex(0x2D6A4F) },

    { LS("Терновник", "Bramble"),
      LS("Всё колется и всё в чужой крови.",
         "Everything pricks, and it is all bloodied."),
      ElementId::Blood, ElementId::Nature, 0.30f, hex(0x9B2226) },

    { LS("Могильник", "Graves"),
      LS("Здесь лежат давно и не все спокойно.",
         "They have lain here a long time, and not all of them quietly."),
      ElementId::Shadow, ElementId::Blood, 0.35f, hex(0x240046) },
};

const BiomeMod& biomeMod(Biome b) {
    int i = int(b);
    return kBiomes[(i < 0 || i >= int(Biome::COUNT)) ? 0 : i];
}

bool biomeFavours(Biome b, ElementId e) {
    if (e == ElementId::None) return false;
    const BiomeMod& m = biomeMod(b);
    return e == m.a || e == m.b;
}

} // namespace shichoy
