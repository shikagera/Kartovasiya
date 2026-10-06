// Relics / Реликвии: пассивки, которые меняют правила, а не числа.
//
// Реликвий около сотни, поэтому они устроены данными, а не ветками в коде.
// Числовые поля RelicDef складываются в RelicMods, и бой читает уже сумму —
// иначе каждая новая реликвия требовала бы своей строчки в боевом цикле.
// Ветка в коде остаётся только у тех, чей эффект нельзя выразить числом
// (они помечены полем special).
#pragma once
#include "core/Cards.h"

namespace shichoy {

// Сгенерировано genrelics.py — правьте генератор, а не этот блок.
enum class RelicId : int {
    None = 0,
    ExtraSlot,
    DeepDraw,
    FirstFree,
    LingeringMark,
    ReactionEcho,
    SpotHunter,
    BloodPact,
    Hoarder,
    Undying,
    Gravedigger,
    Alembic,
    SecondFinger,
    WideCuff,
    LinedCoat,
    ThinDeck,
    GlovedHand,
    FreeMerge,
    SilkLining,
    CardCase,
    LooseThread,
    ThirdFinger,
    EmberRing,
    SaltCellar,
    StormGlass,
    ColdHearth,
    DoubleMark,
    PrismShard,
    WaxTablet,
    BurntSpoon,
    TideMarker,
    CatalystVial,
    TwinFlask,
    ReagentBelt,
    AlchemistMask,
    MortarPestle,
    QuenchBasin,
    RetortGlass,
    WeldingSeal,
    GreatCrucible,
    ChainTongs,
    UltimaKey,
    GoldLeaf,
    SolderIron,
    FurnaceHeart,
    AmberBead,
    TwinCrucible,
    IronCorset,
    LeadLining,
    StartShield,
    ThornBrooch,
    BramblePin,
    HeavyWatch,
    SignetOfYears,
    ColdOpen,
    StoneCollar,
    MourningVeil,
    IronThorns,
    DeadBolt,
    PaleLocket,
    WarmDecoction,
    SlowMend,
    DeepMend,
    LeechSeal,
    GreenPoultice,
    SurgeonKit,
    DeepPockets,
    CollectorEye,
    LedgerClasp,
    MagpieNest,
    DuelistCane,
    BlackPowder,
    LastStand,
    SpotSplit,
    GrimoireStub,
    InkedFingers,
    DuelPistol,
    RazorTie,
    SilverNib,
    WidowKnife,
    BrokenMirror,
    HangedSeal,
    NotaryStamp,
    GildedCage,
    SootGlove,
    OldDebt,
    CandleStub,
    GlassEye,
    DeathMask,
    TarotBack,
    SilentBell,
    CrackedLens,
    BoneDice,
    VelvetRope,
    LastWill,
    EtherLamp,
    SealedLetter,
    HeirloomRing,
    ScorchedMap,
    NightLedger,
    CrowFeather,
    SaltRing,
    PorcelainHand,
    ObsidianStud,
    COUNT
};

// Эффекты, которые числом не выражаются и разбираются отдельно.
enum class RelicSpecial : int {
    None = 0,
    FirstFree,       // первая карта за ход не тратит слот
    LingeringMark,   // метка не спадает по времени
    ReactionEcho,    // реакция срабатывает дважды по 60%
    GuardBreak,      // урон проходит сквозь щит врага
    BloodPact,       // +30% урона, каждая карта стоит 2 HP
    Hoarder,         // карта из руки остаётся в колоде после боя
    Undying,         // раз за бой переживаешь смертельный удар
    Alembic,         // слияние сохраняет цепочку в колоде
    DoubleMark,      // первая стихия за бой ставит две метки
    FreeMerge,       // первое слияние за ход не тратит слот
    LastStand,       // ниже трети здоровья урон вырастает вдвое
    ColdOpen,        // первый ход врага пропускается
    COUNT
};

struct RelicDef {
    RelicId id = RelicId::None;
    LS name;
    LS desc;
    Color tint = WHITE;
    int  unlock = -1;             // бит разблокировки; -1 — доступна всегда
    RelicSpecial special = RelicSpecial::None;

    // ── числовые эффекты ──────────────────────────────────────────────────
    float dmgMul      = 1.0f;     // весь исходящий урон
    float elemMul     = 1.0f;     // урон карт-стихий
    float spellMul    = 1.0f;     // урон карт-заклинаний
    float reactionMul = 1.0f;     // урон реакций
    float mergeMul    = 1.0f;     // урон слитых карт (уровень выше Base)
    float takenMul    = 1.0f;     // входящий урон
    float healMul     = 1.0f;     // всё лечение
    int   startShield = 0;        // щит в начале боя
    int   bonusHp     = 0;        // прибавка к максимуму здоровья
    int   slots       = 0;        // слоты хода
    int   draw        = 0;        // добор в начале хода
    int   regen       = 0;        // лечение в конце своего хода
    int   markTurns   = 0;        // стойкость метки
    int   rewardRolls = 0;        // лишние броски материалов
    int   thorns      = 0;        // урон в ответ атакующему
};

// Сумма всех числовых полей надетых реликвий.
struct RelicMods {
    float dmgMul = 1.0f, elemMul = 1.0f, spellMul = 1.0f;
    float reactionMul = 1.0f, mergeMul = 1.0f, takenMul = 1.0f, healMul = 1.0f;
    int startShield = 0, bonusHp = 0, slots = 0, draw = 0;
    int regen = 0, markTurns = 0, rewardRolls = 0, thorns = 0;
};

const RelicDef& relicDef(RelicId id);
int relicCount();                                   // сколько всего описано
RelicMods relicMods(const std::vector<RelicId>& owned);
bool hasSpecial(const std::vector<RelicId>& owned, RelicSpecial s);

// Случайная реликвия из доступных по разблокировкам и ещё не собранных.
RelicId rollRelic(Rng& r, const std::vector<RelicId>& owned, unsigned unlocks = ~0u);

} // namespace shichoy
