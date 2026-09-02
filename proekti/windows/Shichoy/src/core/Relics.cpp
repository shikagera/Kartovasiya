#include "core/Relics.h"
#include <algorithm>

namespace shichoy {

static const RelicDef kRelics[int(RelicId::COUNT)] = {
    { RelicId::None, LS("—", "—"), LS("", ""), GRAY },

    { RelicId::ExtraSlot,
      LS("Лишний палец", "The Extra Finger"),
      LS("+1 слот хода. Никто не считает, сколько их должно быть.",
         "+1 card slot each turn. Nobody counts how many there should be."),
      hex(0xF0DC8C) },

    { RelicId::DeepDraw,
      LS("Бездонный карман", "Bottomless Pocket"),
      LS("+2 карты к добору каждый ход.",
         "+2 cards drawn every turn."),
      hex(0x90E0EF) },

    { RelicId::FirstFree,
      LS("Право первой руки", "Right of First Hand"),
      LS("Первая карта за ход не тратит слот.",
         "The first card each turn costs no slot."),
      hex(0xFFD60A) },

    { RelicId::LingeringMark,
      LS("Несмываемое клеймо", "Indelible Brand"),
      LS("Метка стихии на враге больше не спадает со временем.",
         "The elemental mark no longer fades with time."),
      hex(0xE85D04) },

    { RelicId::ReactionEcho,
      LS("Эхо в пустой зале", "Echo in an Empty Hall"),
      LS("Каждая реакция срабатывает дважды, по 60% урона.",
         "Every reaction fires twice, at 60% damage each."),
      hex(0x9D4EDD) },

    { RelicId::SpotHunter,
      LS("Глаз анатома", "The Anatomist's Eye"),
      LS("По уязвимым точкам не бывает промаха.",
         "You never miss a weak spot."),
      hex(0xCAF0F8) },

    { RelicId::BloodPact,
      LS("Кровный вексель", "Blood Promissory"),
      LS("+30% ко всему урону, но каждая сыгранная карта стоит 2 HP.",
         "+30% to all damage, but every card played costs 2 HP."),
      hex(0x9B2226) },

    { RelicId::Hoarder,
      LS("Скупая ладонь", "The Miser's Palm"),
      LS("После победы одна карта из руки уходит в колоду насовсем.",
         "After a win, one card from your hand joins the deck for good."),
      hex(0xC2A24A) },

    { RelicId::Undying,
      LS("Восковой двойник", "Waxen Double"),
      LS("Раз за бой смертельный удар оставляет вас с 1 HP.",
         "Once per combat a lethal blow leaves you at 1 HP."),
      hex(0xD8CBB3) },

    { RelicId::Gravedigger,
      LS("Лопата могильщика", "Gravedigger's Spade"),
      LS("Каждая разрушенная точка даёт вдвое больше материалов.",
         "Every destroyed weak spot yields twice the materials."),
      hex(0x8A5A3B) },

    { RelicId::Alembic,
      LS("Алембик", "The Alembic"),
      LS("Слитая карта уходит в колоду после боя, а не исчезает.",
         "A merged card joins the deck after combat instead of vanishing."),
      hex(0xB5179E) },
};

const RelicDef& relicDef(RelicId id) {
    int i = int(id);
    return kRelics[(i < 0 || i >= int(RelicId::COUNT)) ? 0 : i];
}

RelicId rollRelic(Rng& r, const std::vector<RelicId>& owned) {
    std::vector<RelicId> pool;
    for (int i = 1; i < int(RelicId::COUNT); ++i) {
        RelicId id = RelicId(i);
        if (std::find(owned.begin(), owned.end(), id) == owned.end()) pool.push_back(id);
    }
    if (pool.empty()) return RelicId::None;
    return pool[size_t(r.i(0, int(pool.size()) - 1))];
}

} // namespace shichoy
