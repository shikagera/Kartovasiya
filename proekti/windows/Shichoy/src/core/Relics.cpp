#include "core/Relics.h"
#include <algorithm>

namespace shichoy {

// Таблица строится один раз. Ссылка из add() живёт только внутри своего
// блока, а reserve снимает вопрос перевыделения.
static std::vector<RelicDef> buildRelics() {
    std::vector<RelicDef> v;
    v.reserve(256);
    auto add = [&](RelicId id, LS n, LS d, Color c, int unlock, RelicSpecial sp) -> RelicDef& {
        v.push_back(RelicDef{});
        RelicDef& r = v.back();
        r.id = id; r.name = std::move(n); r.desc = std::move(d);
        r.tint = c; r.unlock = unlock; r.special = sp;
        return r;
    };

    { RelicDef& r = add(RelicId::ExtraSlot,
        LS("Лишний палец", "The Extra Finger"),
        LS("+1 слот хода. Никто не считает, сколько их должно быть.",
           "+1 card slot each turn. Nobody counts how many there should be."),
        hex(0xF0DC8C), -1, RelicSpecial::None);
      r.slots = 1;
      (void)r; }
    { RelicDef& r = add(RelicId::DeepDraw,
        LS("Бездонный карман", "Bottomless Pocket"),
        LS("+2 карты к добору каждый ход.",
           "+2 cards drawn every turn."),
        hex(0x90E0EF), -1, RelicSpecial::None);
      r.draw = 2;
      (void)r; }
    { RelicDef& r = add(RelicId::FirstFree,
        LS("Право первой руки", "Right of First Hand"),
        LS("Первая карта за ход не тратит слот.",
           "The first card each turn costs no slot."),
        hex(0xFFD60A), -1, RelicSpecial::FirstFree);
      (void)r; }
    { RelicDef& r = add(RelicId::LingeringMark,
        LS("Несмываемое клеймо", "Indelible Brand"),
        LS("Метка стихии на враге больше не спадает со временем.",
           "The elemental mark no longer fades with time."),
        hex(0xE85D04), -1, RelicSpecial::LingeringMark);
      (void)r; }
    { RelicDef& r = add(RelicId::ReactionEcho,
        LS("Эхо в пустой зале", "Echo in an Empty Hall"),
        LS("Каждая реакция срабатывает дважды, по 60% урона.",
           "Every reaction fires twice, at 60% damage each."),
        hex(0x9D4EDD), -1, RelicSpecial::ReactionEcho);
      (void)r; }
    { RelicDef& r = add(RelicId::SpotHunter,
        LS("Глаз анатома", "The Anatomist's Eye"),
        LS("Ваш урон проходит сквозь щит врага.",
           "Your damage passes straight through enemy guard."),
        hex(0xC9B78E), -1, RelicSpecial::GuardBreak);
      (void)r; }
    { RelicDef& r = add(RelicId::BloodPact,
        LS("Кровный уговор", "The Blood Pact"),
        LS("+30% урона. Каждая карта стоит 2 HP.",
           "+30% damage. Every card costs 2 HP."),
        hex(0x9B2226), -1, RelicSpecial::BloodPact);
      (void)r; }
    { RelicDef& r = add(RelicId::Hoarder,
        LS("Скупая ладонь", "The Miser's Palm"),
        LS("Одна карта из руки остаётся в колоде после боя.",
           "One card from your hand stays in the deck after the fight."),
        hex(0xF0DC8C), -1, RelicSpecial::Hoarder);
      (void)r; }
    { RelicDef& r = add(RelicId::Undying,
        LS("Восковой двойник", "The Wax Double"),
        LS("Раз за бой вы переживаете смертельный удар с 1 HP.",
           "Once per fight you survive a lethal blow at 1 HP."),
        hex(0xC9B78E), -1, RelicSpecial::Undying);
      (void)r; }
    { RelicDef& r = add(RelicId::Gravedigger,
        LS("Лопата могильщика", "The Gravedigger's Spade"),
        LS("С каждого боя падает заметно больше материалов.",
           "Every fight yields considerably more materials."),
        hex(0x8D99AE), -1, RelicSpecial::None);
      r.rewardRolls = 3;
      (void)r; }
    { RelicDef& r = add(RelicId::Alembic,
        LS("Перегонный куб", "The Alembic"),
        LS("Слияние сохраняет цепочку: составная карта уходит в колоду.",
           "Merging keeps the chain: the compound card goes to your deck."),
        hex(0x4EA8DE), -1, RelicSpecial::Alembic);
      (void)r; }
    { RelicDef& r = add(RelicId::SecondFinger,
        LS("Второй лишний палец", "The Second Extra Finger"),
        LS("+1 слот хода. Рука привыкает ко всему.",
           "+1 card slot each turn. The hand adapts."),
        hex(0xF0DC8C), 2, RelicSpecial::None);
      r.slots = 1;
      (void)r; }
    { RelicDef& r = add(RelicId::WideCuff,
        LS("Широкий обшлаг", "The Wide Cuff"),
        LS("+1 карта к добору.",
           "+1 card drawn each turn."),
        hex(0x90E0EF), -1, RelicSpecial::None);
      r.draw = 1;
      (void)r; }
    { RelicDef& r = add(RelicId::LinedCoat,
        LS("Подбитый фрак", "The Lined Tailcoat"),
        LS("+3 карты к добору, входящий урон выше на 10%.",
           "+3 cards drawn; you take 10% more damage."),
        hex(0x90E0EF), 1, RelicSpecial::None);
      r.takenMul = 1.10f; r.draw = 3;
      (void)r; }
    { RelicDef& r = add(RelicId::ThinDeck,
        LS("Тонкая колода", "The Thin Deck"),
        LS("+2 слота хода, добор меньше на 2 карты.",
           "+2 card slots; you draw 2 cards fewer."),
        hex(0xF0DC8C), 3, RelicSpecial::None);
      r.slots = 2; r.draw = -2;
      (void)r; }
    { RelicDef& r = add(RelicId::GlovedHand,
        LS("Лайковая перчатка", "The Kid Glove"),
        LS("+1 слот хода и +1 карта к добору. Цена — 10% урона.",
           "+1 slot and +1 card drawn. The price is 10% of your damage."),
        hex(0xF0DC8C), 2, RelicSpecial::None);
      r.dmgMul = 0.90f; r.slots = 1; r.draw = 1;
      (void)r; }
    { RelicDef& r = add(RelicId::FreeMerge,
        LS("Щипцы ювелира", "The Jeweller's Pliers"),
        LS("Первое слияние за ход не тратит слот.",
           "The first merge each turn costs no slot."),
        hex(0x4EA8DE), 1, RelicSpecial::FreeMerge);
      (void)r; }
    { RelicDef& r = add(RelicId::SilkLining,
        LS("Шёлковая подкладка", "The Silk Lining"),
        LS("+2 карты к добору.",
           "+2 cards drawn each turn."),
        hex(0x90E0EF), 2, RelicSpecial::None);
      r.draw = 2;
      (void)r; }
    { RelicDef& r = add(RelicId::CardCase,
        LS("Визитница", "The Card Case"),
        LS("+1 слот хода, лечение слабее на четверть.",
           "+1 card slot; healing is a quarter weaker."),
        hex(0xF0DC8C), 1, RelicSpecial::None);
      r.healMul = 0.75f; r.slots = 1;
      (void)r; }
    { RelicDef& r = add(RelicId::LooseThread,
        LS("Распущенная нить", "The Loose Thread"),
        LS("+4 карты к добору, максимум здоровья меньше на 12.",
           "+4 cards drawn; 12 less maximum health."),
        hex(0x90E0EF), 3, RelicSpecial::None);
      r.bonusHp = -12; r.draw = 4;
      (void)r; }
    { RelicDef& r = add(RelicId::ThirdFinger,
        LS("Третий лишний палец", "The Third Extra Finger"),
        LS("+1 слот хода. Перчатку давно шьют на заказ.",
           "+1 card slot. The glove is bespoke by now."),
        hex(0xF0DC8C), 5, RelicSpecial::None);
      r.slots = 1;
      (void)r; }
    { RelicDef& r = add(RelicId::EmberRing,
        LS("Перстень углей", "The Ember Ring"),
        LS("Карты-стихии бьют на 20% сильнее.",
           "Element cards hit 20% harder."),
        hex(0xE85D04), -1, RelicSpecial::None);
      r.elemMul = 1.20f;
      (void)r; }
    { RelicDef& r = add(RelicId::SaltCellar,
        LS("Солонка", "The Salt Cellar"),
        LS("Карты-стихии бьют на 35% сильнее, лечение слабее на треть.",
           "Element cards hit 35% harder; healing is a third weaker."),
        hex(0xE85D04), 2, RelicSpecial::None);
      r.elemMul = 1.35f; r.healMul = 0.66f;
      (void)r; }
    { RelicDef& r = add(RelicId::StormGlass,
        LS("Штормовое стекло", "The Storm Glass"),
        LS("Метка держится на два хода дольше.",
           "Marks last two turns longer."),
        hex(0x4EA8DE), -1, RelicSpecial::None);
      r.markTurns = 2;
      (void)r; }
    { RelicDef& r = add(RelicId::ColdHearth,
        LS("Остывший очаг", "The Cold Hearth"),
        LS("Карты-стихии бьют на 15% сильнее, метка держится на ход дольше.",
           "Element cards hit 15% harder and marks last a turn longer."),
        hex(0xE85D04), 1, RelicSpecial::None);
      r.elemMul = 1.15f; r.markTurns = 1;
      (void)r; }
    { RelicDef& r = add(RelicId::DoubleMark,
        LS("Двуглавая печать", "The Two-Headed Seal"),
        LS("Первая стихия за бой ставит сразу две метки.",
           "The first element each fight sets two marks at once."),
        hex(0x9D4EDD), 4, RelicSpecial::DoubleMark);
      (void)r; }
    { RelicDef& r = add(RelicId::PrismShard,
        LS("Осколок призмы", "The Prism Shard"),
        LS("Карты-стихии бьют на 25% сильнее.",
           "Element cards hit 25% harder."),
        hex(0x4EA8DE), 2, RelicSpecial::None);
      r.elemMul = 1.25f;
      (void)r; }
    { RelicDef& r = add(RelicId::WaxTablet,
        LS("Восковая табличка", "The Wax Tablet"),
        LS("Метка держится на три хода дольше, ваш урон слабее на 10%.",
           "Marks last three turns longer; your damage is 10% weaker."),
        hex(0xC9B78E), 3, RelicSpecial::None);
      r.dmgMul = 0.90f; r.markTurns = 3;
      (void)r; }
    { RelicDef& r = add(RelicId::BurntSpoon,
        LS("Обожжённая ложка", "The Burnt Spoon"),
        LS("Карты-стихии бьют на 40% сильнее, заклинания — на 20% слабее.",
           "Element cards hit 40% harder; spells 20% weaker."),
        hex(0xE85D04), 4, RelicSpecial::None);
      r.elemMul = 1.40f; r.spellMul = 0.80f;
      (void)r; }
    { RelicDef& r = add(RelicId::TideMarker,
        LS("Отметка прилива", "The Tide Marker"),
        LS("Карты-стихии бьют на 18% сильнее, щит в начале боя 8.",
           "Element cards hit 18% harder; 8 shield each fight."),
        hex(0x4EA8DE), 1, RelicSpecial::None);
      r.elemMul = 1.18f; r.startShield = 8;
      (void)r; }
    { RelicDef& r = add(RelicId::CatalystVial,
        LS("Склянка катализатора", "The Catalyst Vial"),
        LS("Реакции бьют на 25% сильнее.",
           "Reactions hit 25% harder."),
        hex(0x9D4EDD), -1, RelicSpecial::None);
      r.reactionMul = 1.25f;
      (void)r; }
    { RelicDef& r = add(RelicId::TwinFlask,
        LS("Двойная колба", "The Twin Flask"),
        LS("Реакции бьют на 45% сильнее, обычный урон слабее на 10%.",
           "Reactions hit 45% harder; ordinary damage 10% weaker."),
        hex(0x9D4EDD), 2, RelicSpecial::None);
      r.dmgMul = 0.90f; r.reactionMul = 1.45f;
      (void)r; }
    { RelicDef& r = add(RelicId::ReagentBelt,
        LS("Пояс реагентов", "The Reagent Belt"),
        LS("Реакции бьют на 20% сильнее, метка держится на ход дольше.",
           "Reactions hit 20% harder; marks last a turn longer."),
        hex(0x9D4EDD), 1, RelicSpecial::None);
      r.reactionMul = 1.20f; r.markTurns = 1;
      (void)r; }
    { RelicDef& r = add(RelicId::AlchemistMask,
        LS("Маска алхимика", "The Alchemist's Mask"),
        LS("Реакции бьют на 30% сильнее.",
           "Reactions hit 30% harder."),
        hex(0x52B788), 3, RelicSpecial::None);
      r.reactionMul = 1.30f;
      (void)r; }
    { RelicDef& r = add(RelicId::MortarPestle,
        LS("Ступка и пестик", "Mortar and Pestle"),
        LS("Реакции бьют на 35% сильнее, добор меньше на карту.",
           "Reactions hit 35% harder; one card fewer drawn."),
        hex(0x52B788), 2, RelicSpecial::None);
      r.reactionMul = 1.35f; r.draw = -1;
      (void)r; }
    { RelicDef& r = add(RelicId::QuenchBasin,
        LS("Закалочный чан", "The Quenching Basin"),
        LS("Реакции бьют на 22% сильнее, входящий урон меньше на 10%.",
           "Reactions hit 22% harder; you take 10% less damage."),
        hex(0x4EA8DE), 3, RelicSpecial::None);
      r.reactionMul = 1.22f; r.takenMul = 0.90f;
      (void)r; }
    { RelicDef& r = add(RelicId::RetortGlass,
        LS("Реторта", "The Retort"),
        LS("Реакции бьют вдвое сильнее, но здоровья меньше на 20.",
           "Reactions hit twice as hard; 20 less health."),
        hex(0x9D4EDD), 6, RelicSpecial::None);
      r.reactionMul = 2.00f; r.bonusHp = -20;
      (void)r; }
    { RelicDef& r = add(RelicId::WeldingSeal,
        LS("Сварная печать", "The Welded Seal"),
        LS("Слитые карты бьют на 25% сильнее.",
           "Merged cards hit 25% harder."),
        hex(0xF0DC8C), -1, RelicSpecial::None);
      r.mergeMul = 1.25f;
      (void)r; }
    { RelicDef& r = add(RelicId::GreatCrucible,
        LS("Большой тигель", "The Great Crucible"),
        LS("Слитые карты бьют на 50% сильнее, обычные — на 15% слабее.",
           "Merged cards hit 50% harder; unmerged ones 15% weaker."),
        hex(0xE85D04), 2, RelicSpecial::None);
      r.dmgMul = 0.85f; r.mergeMul = 1.50f;
      (void)r; }
    { RelicDef& r = add(RelicId::ChainTongs,
        LS("Цепные клещи", "The Chain Tongs"),
        LS("Слитые карты бьют на 30% сильнее, первое слияние за ход бесплатно.",
           "Merged cards hit 30% harder; the first merge each turn is free."),
        hex(0x4EA8DE), 3, RelicSpecial::FreeMerge);
      r.mergeMul = 1.30f;
      (void)r; }
    { RelicDef& r = add(RelicId::UltimaKey,
        LS("Ключ от Ultima", "The Ultima Key"),
        LS("Слитые карты бьют на 70% сильнее. Входящий урон выше на четверть.",
           "Merged cards hit 70% harder. You take a quarter more damage."),
        hex(0xFFD60A), 5, RelicSpecial::None);
      r.mergeMul = 1.70f; r.takenMul = 1.25f;
      (void)r; }
    { RelicDef& r = add(RelicId::GoldLeaf,
        LS("Сусальное золото", "The Gold Leaf"),
        LS("Слитые карты бьют на 20% сильнее.",
           "Merged cards hit 20% harder."),
        hex(0xF0DC8C), 1, RelicSpecial::None);
      r.mergeMul = 1.20f;
      (void)r; }
    { RelicDef& r = add(RelicId::SolderIron,
        LS("Паяльник", "The Soldering Iron"),
        LS("Слитые карты бьют на 35% сильнее.",
           "Merged cards hit 35% harder."),
        hex(0x8D99AE), 3, RelicSpecial::None);
      r.mergeMul = 1.35f;
      (void)r; }
    { RelicDef& r = add(RelicId::FurnaceHeart,
        LS("Сердце печи", "The Furnace Heart"),
        LS("Слитые карты бьют вдвое сильнее, лечение не работает.",
           "Merged cards hit twice as hard; healing does nothing."),
        hex(0xE85D04), 7, RelicSpecial::None);
      r.mergeMul = 2.00f; r.healMul = 0.00f;
      (void)r; }
    { RelicDef& r = add(RelicId::AmberBead,
        LS("Янтарная бусина", "The Amber Bead"),
        LS("Слитые карты бьют на 22% сильнее, щит в начале боя 10.",
           "Merged cards hit 22% harder; 10 shield each fight."),
        hex(0xFFD60A), 2, RelicSpecial::None);
      r.mergeMul = 1.22f; r.startShield = 10;
      (void)r; }
    { RelicDef& r = add(RelicId::TwinCrucible,
        LS("Парный тигель", "The Paired Crucible"),
        LS("Слитые карты бьют на 40% сильнее, реакции — на 15% слабее.",
           "Merged cards hit 40% harder; reactions 15% weaker."),
        hex(0xF0DC8C), 4, RelicSpecial::None);
      r.reactionMul = 0.85f; r.mergeMul = 1.40f;
      (void)r; }
    { RelicDef& r = add(RelicId::IronCorset,
        LS("Железный корсет", "The Iron Corset"),
        LS("Входящий урон меньше на 15%.",
           "You take 15% less damage."),
        hex(0x8D99AE), -1, RelicSpecial::None);
      r.takenMul = 0.85f;
      (void)r; }
    { RelicDef& r = add(RelicId::LeadLining,
        LS("Свинцовая подкладка", "The Lead Lining"),
        LS("Входящий урон меньше на 25%, ваш урон — на 10%.",
           "You take 25% less damage; you deal 10% less."),
        hex(0x8D99AE), 1, RelicSpecial::None);
      r.dmgMul = 0.90f; r.takenMul = 0.75f;
      (void)r; }
    { RelicDef& r = add(RelicId::StartShield,
        LS("Нагрудник наготове", "The Ready Breastplate"),
        LS("Бой начинается с щитом 12.",
           "Each fight begins with 12 shield."),
        hex(0x4EA8DE), -1, RelicSpecial::None);
      r.startShield = 12;
      (void)r; }
    { RelicDef& r = add(RelicId::ThornBrooch,
        LS("Терновая брошь", "The Thorn Brooch"),
        LS("Атакующий получает 4 урона в ответ.",
           "Attackers take 4 damage in return."),
        hex(0x9B2226), -1, RelicSpecial::None);
      r.thorns = 4;
      (void)r; }
    { RelicDef& r = add(RelicId::BramblePin,
        LS("Шипастая булавка", "The Bramble Pin"),
        LS("Атакующий получает 9 урона в ответ.",
           "Attackers take 9 damage in return."),
        hex(0x9B2226), 3, RelicSpecial::None);
      r.thorns = 9;
      (void)r; }
    { RelicDef& r = add(RelicId::HeavyWatch,
        LS("Тяжёлый хронометр", "The Heavy Chronometer"),
        LS("+20 к максимуму здоровья.",
           "+20 maximum health."),
        hex(0xC9B78E), -1, RelicSpecial::None);
      r.bonusHp = 20;
      (void)r; }
    { RelicDef& r = add(RelicId::SignetOfYears,
        LS("Перстень лет", "The Signet of Years"),
        LS("+35 к максимуму здоровья, добор меньше на карту.",
           "+35 maximum health; one card fewer drawn."),
        hex(0xC9B78E), 2, RelicSpecial::None);
      r.bonusHp = 35; r.draw = -1;
      (void)r; }
    { RelicDef& r = add(RelicId::ColdOpen,
        LS("Выстуженная зала", "The Chilled Hall"),
        LS("Враг пропускает свой первый ход.",
           "The enemy skips its first turn."),
        hex(0x90E0EF), 4, RelicSpecial::ColdOpen);
      (void)r; }
    { RelicDef& r = add(RelicId::StoneCollar,
        LS("Каменный воротник", "The Stone Collar"),
        LS("Входящий урон меньше на 20%, добор меньше на карту.",
           "You take 20% less damage; one card fewer drawn."),
        hex(0x8D99AE), 2, RelicSpecial::None);
      r.takenMul = 0.80f; r.draw = -1;
      (void)r; }
    { RelicDef& r = add(RelicId::MourningVeil,
        LS("Траурная вуаль", "The Mourning Veil"),
        LS("Щит в начале боя 20, ваш урон слабее на 15%.",
           "20 shield each fight; your damage is 15% weaker."),
        hex(0xC9B78E), 3, RelicSpecial::None);
      r.dmgMul = 0.85f; r.startShield = 20;
      (void)r; }
    { RelicDef& r = add(RelicId::IronThorns,
        LS("Железные шипы", "The Iron Thorns"),
        LS("Атакующий получает 15 урона в ответ, здоровья меньше на 15.",
           "Attackers take 15 damage in return; 15 less health."),
        hex(0x9B2226), 5, RelicSpecial::None);
      r.bonusHp = -15; r.thorns = 15;
      (void)r; }
    { RelicDef& r = add(RelicId::DeadBolt,
        LS("Мёртвый засов", "The Dead Bolt"),
        LS("Входящий урон меньше на треть, слотов хода на один меньше.",
           "You take a third less damage; one fewer card slot."),
        hex(0x8D99AE), 6, RelicSpecial::None);
      r.takenMul = 0.66f; r.slots = -1;
      (void)r; }
    { RelicDef& r = add(RelicId::PaleLocket,
        LS("Бледный медальон", "The Pale Locket"),
        LS("+30 к максимуму здоровья.",
           "+30 maximum health."),
        hex(0xC9B78E), 4, RelicSpecial::None);
      r.bonusHp = 30;
      (void)r; }
    { RelicDef& r = add(RelicId::WarmDecoction,
        LS("Горячий отвар", "The Warm Decoction"),
        LS("Всё лечение сильнее на 40%.",
           "All healing is 40% stronger."),
        hex(0x95D5B2), -1, RelicSpecial::None);
      r.healMul = 1.40f;
      (void)r; }
    { RelicDef& r = add(RelicId::SlowMend,
        LS("Медленное срастание", "The Slow Knitting"),
        LS("В конце своего хода вы лечитесь на 3.",
           "You heal 3 at the end of your turn."),
        hex(0x95D5B2), -1, RelicSpecial::None);
      r.regen = 3;
      (void)r; }
    { RelicDef& r = add(RelicId::DeepMend,
        LS("Глубокое срастание", "The Deep Knitting"),
        LS("В конце своего хода вы лечитесь на 7, урон слабее на 10%.",
           "You heal 7 at end of turn; your damage is 10% weaker."),
        hex(0x95D5B2), 2, RelicSpecial::None);
      r.dmgMul = 0.90f; r.regen = 7;
      (void)r; }
    { RelicDef& r = add(RelicId::LeechSeal,
        LS("Пиявочная печать", "The Leech Seal"),
        LS("Лечение сильнее вдвое, максимум здоровья меньше на 10.",
           "Healing is doubled; maximum health is 10 lower."),
        hex(0x52B788), 3, RelicSpecial::None);
      r.healMul = 2.00f; r.bonusHp = -10;
      (void)r; }
    { RelicDef& r = add(RelicId::GreenPoultice,
        LS("Зелёная припарка", "The Green Poultice"),
        LS("В конце своего хода вы лечитесь на 5.",
           "You heal 5 at the end of your turn."),
        hex(0x95D5B2), 2, RelicSpecial::None);
      r.regen = 5;
      (void)r; }
    { RelicDef& r = add(RelicId::SurgeonKit,
        LS("Набор хирурга", "The Surgeon's Kit"),
        LS("Лечение сильнее на 60%, щит в начале боя 8.",
           "Healing is 60% stronger; 8 shield each fight."),
        hex(0x95D5B2), 4, RelicSpecial::None);
      r.healMul = 1.60f; r.startShield = 8;
      (void)r; }
    { RelicDef& r = add(RelicId::DeepPockets,
        LS("Глубокие карманы", "Deep Pockets"),
        LS("С каждого боя падает больше материалов.",
           "Every fight yields more materials."),
        hex(0xF0DC8C), -1, RelicSpecial::None);
      r.rewardRolls = 2;
      (void)r; }
    { RelicDef& r = add(RelicId::CollectorEye,
        LS("Глаз собирателя", "The Collector's Eye"),
        LS("Заметно больше материалов, но здоровья меньше на 10.",
           "Far more materials, but 10 less health."),
        hex(0xF0DC8C), 2, RelicSpecial::None);
      r.bonusHp = -10; r.rewardRolls = 4;
      (void)r; }
    { RelicDef& r = add(RelicId::LedgerClasp,
        LS("Застёжка гроссбуха", "The Ledger Clasp"),
        LS("Больше материалов, лечение слабее на четверть.",
           "More materials; healing a quarter weaker."),
        hex(0xF0DC8C), 1, RelicSpecial::None);
      r.healMul = 0.75f; r.rewardRolls = 3;
      (void)r; }
    { RelicDef& r = add(RelicId::MagpieNest,
        LS("Сорочье гнездо", "The Magpie's Nest"),
        LS("Очень много материалов, но входящий урон выше на 20%.",
           "A great deal more materials; you take 20% more damage."),
        hex(0xF0DC8C), 5, RelicSpecial::None);
      r.takenMul = 1.20f; r.rewardRolls = 6;
      (void)r; }
    { RelicDef& r = add(RelicId::DuelistCane,
        LS("Трость дуэлянта", "The Duellist's Cane"),
        LS("Весь урон выше на 15%.",
           "All damage is 15% higher."),
        hex(0xE56B6F), -1, RelicSpecial::None);
      r.dmgMul = 1.15f;
      (void)r; }
    { RelicDef& r = add(RelicId::BlackPowder,
        LS("Чёрный порох", "The Black Powder"),
        LS("Весь урон выше на 30%, входящий — тоже на 30%.",
           "All damage is 30% higher; so is damage taken."),
        hex(0xE85D04), 1, RelicSpecial::None);
      r.dmgMul = 1.30f; r.takenMul = 1.30f;
      (void)r; }
    { RelicDef& r = add(RelicId::LastStand,
        LS("Последний рубеж", "The Last Stand"),
        LS("Ниже трети здоровья ваш урон удваивается.",
           "Below a third of your health your damage doubles."),
        hex(0x9B2226), 3, RelicSpecial::LastStand);
      (void)r; }
    { RelicDef& r = add(RelicId::SpotSplit,
        LS("Раскол", "The Cleaving"),
        LS("Весь урон выше на 20%, щит врага больше не держит.",
           "All damage 20% higher, and enemy guard no longer holds."),
        hex(0x8D99AE), 2, RelicSpecial::GuardBreak);
      r.dmgMul = 1.20f;
      (void)r; }
    { RelicDef& r = add(RelicId::GrimoireStub,
        LS("Обрывок гримуара", "The Grimoire Stub"),
        LS("Карты-заклинания бьют на 30% сильнее.",
           "Spell cards hit 30% harder."),
        hex(0x9D4EDD), -1, RelicSpecial::None);
      r.spellMul = 1.30f;
      (void)r; }
    { RelicDef& r = add(RelicId::InkedFingers,
        LS("Пальцы в чернилах", "Ink-Stained Fingers"),
        LS("Карты-заклинания бьют на 50% сильнее, стихии — на 10% слабее.",
           "Spell cards hit 50% harder; elements 10% weaker."),
        hex(0x9D4EDD), 2, RelicSpecial::None);
      r.elemMul = 0.90f; r.spellMul = 1.50f;
      (void)r; }
    { RelicDef& r = add(RelicId::DuelPistol,
        LS("Дуэльный пистолет", "The Duelling Pistol"),
        LS("Весь урон выше на 22%.",
           "All damage is 22% higher."),
        hex(0xE56B6F), 3, RelicSpecial::None);
      r.dmgMul = 1.22f;
      (void)r; }
    { RelicDef& r = add(RelicId::RazorTie,
        LS("Бритвенный шейный платок", "The Razor Cravat"),
        LS("Весь урон выше на 40%, максимум здоровья меньше на 20.",
           "All damage is 40% higher; 20 less maximum health."),
        hex(0x9B2226), 5, RelicSpecial::None);
      r.dmgMul = 1.40f; r.bonusHp = -20;
      (void)r; }
    { RelicDef& r = add(RelicId::SilverNib,
        LS("Серебряное перо", "The Silver Nib"),
        LS("Заклинания бьют на 35% сильнее, щит в начале боя 6.",
           "Spells hit 35% harder; 6 shield each fight."),
        hex(0x9D4EDD), 2, RelicSpecial::None);
      r.spellMul = 1.35f; r.startShield = 6;
      (void)r; }
    { RelicDef& r = add(RelicId::WidowKnife,
        LS("Вдовий нож", "The Widow's Knife"),
        LS("Весь урон выше на 18%, лечение слабее вдвое.",
           "All damage is 18% higher; healing is halved."),
        hex(0x9B2226), 1, RelicSpecial::None);
      r.dmgMul = 1.18f; r.healMul = 0.50f;
      (void)r; }
    { RelicDef& r = add(RelicId::BrokenMirror,
        LS("Разбитое зеркало", "The Broken Mirror"),
        LS("Весь урон выше на 25%, входящий — на 15%.",
           "All damage is 25% higher; damage taken 15% higher."),
        hex(0x9D4EDD), 2, RelicSpecial::None);
      r.dmgMul = 1.25f; r.takenMul = 1.15f;
      (void)r; }
    { RelicDef& r = add(RelicId::HangedSeal,
        LS("Печать повешенного", "The Hanged Man's Seal"),
        LS("Весь урон выше на 55%, слотов хода на один меньше.",
           "All damage is 55% higher; one fewer card slot."),
        hex(0x9B2226), 7, RelicSpecial::None);
      r.dmgMul = 1.55f; r.slots = -1;
      (void)r; }
    { RelicDef& r = add(RelicId::NotaryStamp,
        LS("Нотариальный штамп", "The Notary's Stamp"),
        LS("+1 слот хода, +1 карта к добору, здоровья меньше на 15.",
           "+1 slot, +1 card drawn; 15 less health."),
        hex(0xF0DC8C), 4, RelicSpecial::None);
      r.bonusHp = -15; r.slots = 1; r.draw = 1;
      (void)r; }
    { RelicDef& r = add(RelicId::GildedCage,
        LS("Золочёная клетка", "The Gilded Cage"),
        LS("Щит в начале боя 25, сбежать нельзя — но это решает ступень.",
           "25 shield each fight; the cage is gilded all the same."),
        hex(0xFFD60A), 4, RelicSpecial::None);
      r.startShield = 25;
      (void)r; }
    { RelicDef& r = add(RelicId::SootGlove,
        LS("Закопчённая перчатка", "The Sooted Glove"),
        LS("Стихии и слияния бьют на 20% сильнее.",
           "Elements and merges hit 20% harder."),
        hex(0xE85D04), 3, RelicSpecial::None);
      r.elemMul = 1.20f; r.mergeMul = 1.20f;
      (void)r; }
    { RelicDef& r = add(RelicId::OldDebt,
        LS("Старый долг", "The Old Debt"),
        LS("+2 слота хода, входящий урон выше на 35%.",
           "+2 card slots; you take 35% more damage."),
        hex(0x9B2226), 6, RelicSpecial::None);
      r.takenMul = 1.35f; r.slots = 2;
      (void)r; }
    { RelicDef& r = add(RelicId::CandleStub,
        LS("Огарок", "The Candle Stub"),
        LS("В конце хода лечение на 4, метка держится на ход дольше.",
           "Heal 4 at end of turn; marks last a turn longer."),
        hex(0x95D5B2), 1, RelicSpecial::None);
      r.regen = 4; r.markTurns = 1;
      (void)r; }
    { RelicDef& r = add(RelicId::GlassEye,
        LS("Стеклянный глаз", "The Glass Eye"),
        LS("Урон проходит сквозь щит, а метка держится на два хода дольше.",
           "Damage pierces guard, and marks last two turns longer."),
        hex(0xC9B78E), 6, RelicSpecial::GuardBreak);
      r.markTurns = 2;
      (void)r; }
    { RelicDef& r = add(RelicId::DeathMask,
        LS("Посмертная маска", "The Death Mask"),
        LS("Ниже трети здоровья урон удваивается, щит в начале боя 15.",
           "Below a third health your damage doubles; 15 shield each fight."),
        hex(0xC9B78E), 8, RelicSpecial::LastStand);
      r.startShield = 15;
      (void)r; }
    { RelicDef& r = add(RelicId::TarotBack,
        LS("Рубашка колоды", "The Card Back"),
        LS("+2 карты к добору и +12 к здоровью.",
           "+2 cards drawn and +12 health."),
        hex(0x90E0EF), 3, RelicSpecial::None);
      r.bonusHp = 12; r.draw = 2;
      (void)r; }
    { RelicDef& r = add(RelicId::SilentBell,
        LS("Беззвучный колокол", "The Silent Bell"),
        LS("Враг пропускает первый ход, ваш урон слабее на 10%.",
           "The enemy skips its first turn; your damage is 10% weaker."),
        hex(0x90E0EF), 7, RelicSpecial::ColdOpen);
      r.dmgMul = 0.90f;
      (void)r; }
    { RelicDef& r = add(RelicId::CrackedLens,
        LS("Треснувшая линза", "The Cracked Lens"),
        LS("Реакции и заклинания бьют на 20% сильнее.",
           "Reactions and spells hit 20% harder."),
        hex(0x4EA8DE), 2, RelicSpecial::None);
      r.spellMul = 1.20f; r.reactionMul = 1.20f;
      (void)r; }
    { RelicDef& r = add(RelicId::BoneDice,
        LS("Костяные кости", "The Bone Dice"),
        LS("Весь урон выше на 20%, материалов больше, здоровья меньше на 10.",
           "All damage 20% higher, more materials, 10 less health."),
        hex(0xC9B78E), 4, RelicSpecial::None);
      r.dmgMul = 1.20f; r.bonusHp = -10; r.rewardRolls = 2;
      (void)r; }
    { RelicDef& r = add(RelicId::VelvetRope,
        LS("Бархатный шнур", "The Velvet Rope"),
        LS("Входящий урон меньше на 12%, лечение сильнее на 25%.",
           "You take 12% less damage; healing is 25% stronger."),
        hex(0xE56B6F), 1, RelicSpecial::None);
      r.takenMul = 0.88f; r.healMul = 1.25f;
      (void)r; }
    { RelicDef& r = add(RelicId::LastWill,
        LS("Завещание", "The Last Will"),
        LS("Раз за бой переживаете смертельный удар, затем урон удваивается.",
           "Survive one lethal blow per fight; your damage doubles below a third."),
        hex(0xC9B78E), 9, RelicSpecial::Undying);
      r.dmgMul = 1.10f;
      (void)r; }
    { RelicDef& r = add(RelicId::EtherLamp,
        LS("Эфирная лампа", "The Aether Lamp"),
        LS("Метка держится на два хода дольше, реакции на 25% сильнее.",
           "Marks last two turns longer; reactions 25% stronger."),
        hex(0x9D4EDD), 5, RelicSpecial::None);
      r.reactionMul = 1.25f; r.markTurns = 2;
      (void)r; }
    { RelicDef& r = add(RelicId::SealedLetter,
        LS("Запечатанное письмо", "The Sealed Letter"),
        LS("Первая карта за ход бесплатна, добор меньше на карту.",
           "The first card each turn is free; one card fewer drawn."),
        hex(0xFFD60A), 3, RelicSpecial::FirstFree);
      r.draw = -1;
      (void)r; }
    { RelicDef& r = add(RelicId::HeirloomRing,
        LS("Фамильный перстень", "The Heirloom Ring"),
        LS("+25 к здоровью, лечение сильнее на треть.",
           "+25 health; healing a third stronger."),
        hex(0xC9B78E), 2, RelicSpecial::None);
      r.healMul = 1.33f; r.bonusHp = 25;
      (void)r; }
    { RelicDef& r = add(RelicId::ScorchedMap,
        LS("Обугленная карта", "The Scorched Map"),
        LS("Стихии бьют на 30% сильнее, входящий урон выше на 15%.",
           "Elements hit 30% harder; you take 15% more damage."),
        hex(0xE85D04), 3, RelicSpecial::None);
      r.elemMul = 1.30f; r.takenMul = 1.15f;
      (void)r; }
    { RelicDef& r = add(RelicId::NightLedger,
        LS("Ночной гроссбух", "The Night Ledger"),
        LS("Слияния на 25% сильнее, материалов больше.",
           "Merges 25% stronger; more materials."),
        hex(0xF0DC8C), 4, RelicSpecial::None);
      r.mergeMul = 1.25f; r.rewardRolls = 2;
      (void)r; }
    { RelicDef& r = add(RelicId::CrowFeather,
        LS("Воронье перо", "The Crow's Feather"),
        LS("+1 карта к добору, атакующий получает 5 урона в ответ.",
           "+1 card drawn; attackers take 5 damage in return."),
        hex(0x9B2226), 2, RelicSpecial::None);
      r.draw = 1; r.thorns = 5;
      (void)r; }
    { RelicDef& r = add(RelicId::SaltRing,
        LS("Соляной круг", "The Circle of Salt"),
        LS("Входящий урон меньше на 18%, метка держится дольше на ход.",
           "You take 18% less damage; marks last a turn longer."),
        hex(0x4EA8DE), 3, RelicSpecial::None);
      r.takenMul = 0.82f; r.markTurns = 1;
      (void)r; }
    { RelicDef& r = add(RelicId::PorcelainHand,
        LS("Фарфоровая ладонь", "The Porcelain Hand"),
        LS("+1 слот хода, входящий урон выше на 15%.",
           "+1 card slot; you take 15% more damage."),
        hex(0xC9B78E), 1, RelicSpecial::None);
      r.takenMul = 1.15f; r.slots = 1;
      (void)r; }
    { RelicDef& r = add(RelicId::ObsidianStud,
        LS("Обсидиановая запонка", "The Obsidian Stud"),
        LS("Слияния на 30% сильнее, лечение слабее на треть.",
           "Merges 30% stronger; healing a third weaker."),
        hex(0x9D4EDD), 3, RelicSpecial::None);
      r.mergeMul = 1.30f; r.healMul = 0.66f;
      (void)r; }

    return v;
}

static const std::vector<RelicDef>& table() {
    static const std::vector<RelicDef> t = buildRelics();
    return t;
}

// Поиск по идентификатору через индекс: relicDef зовут в циклах отрисовки.
static const std::vector<const RelicDef*>& index() {
    static const std::vector<const RelicDef*> idx = [] {
        std::vector<const RelicDef*> v(size_t(RelicId::COUNT), nullptr);
        for (const RelicDef& r : table()) {
            size_t i = size_t(r.id);
            if (i < v.size()) v[i] = &r;
        }
        return v;
    }();
    return idx;
}

const RelicDef& relicDef(RelicId id) {
    static const RelicDef none{};
    size_t i = size_t(id);
    const auto& idx = index();
    if (i < idx.size() && idx[i]) return *idx[i];
    return none;
}

int relicCount() { return int(table().size()); }

RelicMods relicMods(const std::vector<RelicId>& owned) {
    RelicMods m;
    for (RelicId id : owned) {
        const RelicDef& d = relicDef(id);
        m.dmgMul      *= d.dmgMul;
        m.elemMul     *= d.elemMul;
        m.spellMul    *= d.spellMul;
        m.reactionMul *= d.reactionMul;
        m.mergeMul    *= d.mergeMul;
        m.takenMul    *= d.takenMul;
        m.healMul     *= d.healMul;
        m.startShield += d.startShield;
        m.bonusHp     += d.bonusHp;
        m.slots       += d.slots;
        m.draw        += d.draw;
        m.regen       += d.regen;
        m.markTurns   += d.markTurns;
        m.rewardRolls += d.rewardRolls;
        m.thorns      += d.thorns;
    }
    return m;
}

bool hasSpecial(const std::vector<RelicId>& owned, RelicSpecial s) {
    if (s == RelicSpecial::None) return false;
    for (RelicId id : owned) if (relicDef(id).special == s) return true;
    return false;
}

RelicId rollRelic(Rng& r, const std::vector<RelicId>& owned, unsigned unlocks) {
    std::vector<RelicId> pool;
    pool.reserve(table().size());
    for (const RelicDef& d : table()) {
        if (d.id == RelicId::None) continue;
        // Запертая реликвия в пул не попадает: разблокировки и дают причину
        // начать следующий забег.
        if (d.unlock >= 0 && !((unlocks >> unsigned(d.unlock)) & 1u)) continue;
        if (std::find(owned.begin(), owned.end(), d.id) != owned.end()) continue;
        pool.push_back(d.id);
    }
    if (pool.empty()) return RelicId::None;
    return pool[size_t(r.i(0, int(pool.size()) - 1))];
}

} // namespace shichoy
