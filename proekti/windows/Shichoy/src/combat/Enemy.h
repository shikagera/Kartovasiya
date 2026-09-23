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
    bool fromSpot = false; // атака завязана на уязвимую точку
    int  spotIndex = -1;   // если точка разрушена — приём выпадает из паттерна
};

enum class SpotBreak : int {
    RemoveMove = 0,   // приём выпадает из паттерна
    Stagger,          // враг пропускает ход
    Expose,           // постоянная уязвимость
    ForcePhase        // немедленный переход к следующей фазе
};

struct WeakSpot {
    LS name;
    int hp = 20, maxHp = 20;
    bool destroyed = false;
    Vector2 offset{ 0, 0 };   // относительно центра тела, в долях размера
    SpotBreak onBreak = SpotBreak::RemoveMove;
    int linkedMove = -1;
    float flash = 0.0f;
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

    std::vector<WeakSpot> spots;
    std::vector<StatusInst> statuses;

    int  actions = 1;      // действий за ход (растёт с глубиной)
    int  guard = 0;        // временный щит врага
    int  enrage = 0;       // прибавка к урону
    bool exposed = false;  // постоянная уязвимость от разрушенной точки

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

Enemy genEnemy(Rng& r, int depth, bool boss);

} // namespace shichoy
