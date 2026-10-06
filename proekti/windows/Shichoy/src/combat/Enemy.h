// Enemy / Противник: без колоды — фиксированные паттерны, фазы, уязвимые точки.
#pragma once
#include "core/Cards.h"

namespace shichoy {

// Кто именно сидит по ту сторону стола — определяет и облик, и повадки.
enum class Archetype : int {
    Reaper = 0,    // жнец: рваный плащ и череп
    Candelabrum,   // канделябр: подсвечник с живым пламенем
    Portrait,      // портрет: фигура лезет из рамы
    Hound,         // егерь: четвероногое, быстрое
    Automaton,     // автоматон: латунь и шестерни
    Choir,         // хор: столб из поющих ртов
    Doll,          // восковая кукла: швы и трещины
    COUNT
};
LS archetypeName(Archetype a);

enum class MoveKind : int {
    Attack = 0,   // обычный удар
    Heavy,        // тяжёлый удар (медленный, больно)
    Multi,        // серия ударов
    Curse,        // навешивает статус на игрока
    Guard,        // защита/щит себе
    Mend,         // лечится
    Enrage,       // усиливает свои удары
    Sunder,       // ломает щит игрока и бьёт
};

struct EnemyMove {
    LS name;
    LS telegraph;          // что видно игроку заранее
    MoveKind kind = MoveKind::Attack;
    int damage = 8;
    int hits = 1;
    StatusId applies = StatusId::None;
    int stacks = 0;
    int turns = 0;
};

struct EnemyPhase {
    LS enterLine;
    std::vector<EnemyMove> moves;
    float hpFraction = 1.0f;   // фаза активна, пока HP выше этой доли
};

struct Enemy {
    LS name;
    LS epithet;
    int hp = 60, maxHp = 60;
    int depth = 1;
    uint32_t seed = 1;
    ElementId flavour = ElementId::Shadow;
    Archetype arche = Archetype::Reaper;
    bool boss = false;

    std::vector<EnemyPhase> phases;
    int phase = 0;
    int moveIndex = 0;

    std::vector<StatusInst> statuses;

    int  actions = 1;      // действий за ход (растёт с глубиной)
    int  guard = 0;        // временный щит врага
    int  enrage = 0;       // прибавка к урону

    float shake = 0.0f;    // визуальная отдача

    const EnemyMove* currentMove() const;
    void advanceMove();
    bool checkPhase();                 // true если фаза сменилась
    int  statusStacks(StatusId s) const;
    void addStatus(StatusId s, int stacks, int turns);
    void removeStatus(StatusId s);
    bool alive() const { return hp > 0; }
    float hpFrac() const { return maxHp > 0 ? float(hp) / float(maxHp) : 0.0f; }
};

// ── Именные боссы ─────────────────────────────────────────────────────────
// Босс перестал быть «усиленным врагом»: у каждого своё имя, облик, стихия,
// реплики на смене фаз и фирменный приём, который точно есть в последней фазе.
struct BossDef {
    LS name, epithet;
    Archetype arche = Archetype::Reaper;
    ElementId flavour = ElementId::Shadow;
    float hpMul = 1.0f;
    int   minDepth = 1;        // раньше этой глубины не встречается
    LS    phase2, phase3;      // что он говорит, меняя облик
    MoveKind sigKind = MoveKind::Heavy;
    LS    sigName, sigTelegraph;
    int   sigDamage = 18;
};

int bossCount();
const BossDef& bossDef(int i);
int  pickBossIndex(Rng& r, int depth);
Enemy genNamedBoss(Rng& r, int depth, int index);

// forcedArche / forcedFlavour — чтобы бой совпал с фигуркой на карте.
Enemy genEnemy(Rng& r, int depth, bool boss,
               Archetype forcedArche = Archetype::COUNT,
               ElementId forcedFlavour = ElementId::None);

} // namespace shichoy
