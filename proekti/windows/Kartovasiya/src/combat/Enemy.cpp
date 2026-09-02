#include "combat/Enemy.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace kv {

const EnemyMove* Enemy::currentMove() const {
    if (phases.empty()) return nullptr;
    const EnemyPhase& ph = phases[size_t(clampi(phase, 0, int(phases.size()) - 1))];
    if (ph.moves.empty()) return nullptr;
    // Приёмы, завязанные на разрушенные точки, выпадают из паттерна.
    int n = int(ph.moves.size());
    for (int k = 0; k < n; ++k) {
        const EnemyMove& m = ph.moves[size_t((moveIndex + k) % n)];
        if (m.spotIndex >= 0 && m.spotIndex < int(spots.size()) && spots[size_t(m.spotIndex)].destroyed)
            continue;
        return &m;
    }
    return &ph.moves[size_t(moveIndex % n)];
}

void Enemy::advanceMove() {
    if (phases.empty()) return;
    const EnemyPhase& ph = phases[size_t(clampi(phase, 0, int(phases.size()) - 1))];
    if (ph.moves.empty()) return;
    int n = int(ph.moves.size());
    // сдвигаемся на следующий доступный приём
    for (int k = 1; k <= n; ++k) {
        int idx = (moveIndex + k) % n;
        const EnemyMove& m = ph.moves[size_t(idx)];
        if (m.spotIndex >= 0 && m.spotIndex < int(spots.size()) && spots[size_t(m.spotIndex)].destroyed)
            continue;
        moveIndex = idx;
        return;
    }
    moveIndex = (moveIndex + 1) % n;
}

bool Enemy::checkPhase() {
    float f = hpFrac();
    int want = phase;
    for (int i = 0; i < int(phases.size()); ++i)
        if (f <= phases[size_t(i)].hpFraction) want = i;
    if (want > phase) {
        phase = want;
        moveIndex = 0;
        guard += 4 + depth;
        return true;
    }
    return false;
}

int Enemy::statusStacks(StatusId s) const {
    for (const auto& st : statuses) if (st.id == s) return st.stacks;
    return 0;
}

void Enemy::addStatus(StatusId s, int stacks, int turns) {
    if (s == StatusId::None || stacks <= 0) return;
    for (auto& st : statuses) {
        if (st.id == s) { st.stacks += stacks; st.turns = std::max(st.turns, turns); return; }
    }
    statuses.push_back(StatusInst{ s, stacks, turns });
}

void Enemy::removeStatus(StatusId s) {
    statuses.erase(std::remove_if(statuses.begin(), statuses.end(),
                                  [&](const StatusInst& x) { return x.id == s; }),
                   statuses.end());
}

// ── Generation / Генерация противника ─────────────────────────────────────
static LS mv(const char* ru, const char* en) { return LS(ru, en); }

static EnemyMove makeMove(Rng& r, int depth, MoveKind k, ElementId fl) {
    EnemyMove m;
    m.kind = k;
    float base = 7.0f + depth * 3.2f;
    switch (k) {
        case MoveKind::Attack:
            m.name = mv("Удар тростью", "Cane Strike");
            m.telegraph = mv("замахивается", "winds up");
            m.damage = int(base * r.f(0.9f, 1.15f));
            break;
        case MoveKind::Heavy:
            m.name = mv("Тяжкий приговор", "Heavy Verdict");
            m.telegraph = mv("собирает тьму — тяжёлый удар", "gathers dark — heavy blow");
            m.damage = int(base * r.f(1.7f, 2.1f));
            break;
        case MoveKind::Multi:
            m.name = mv("Череда ударов", "Flurry");
            m.telegraph = mv("готовит серию", "readies a flurry");
            m.damage = int(base * 0.45f);
            m.hits = r.i(3, 4);
            break;
        case MoveKind::Curse: {
            const StatusId pool[] = { StatusId::Burn, StatusId::Bleed, StatusId::Weaken,
                                      StatusId::Vulnerable, StatusId::Poisoned };
            m.name = mv("Проклятие", "Curse");
            m.telegraph = mv("шепчет проклятие", "whispers a curse");
            m.damage = int(base * 0.5f);
            m.applies = pool[r.i(0, 4)];
            m.stacks = 2 + depth / 3;
            m.turns = 3;
            break;
        }
        case MoveKind::Guard:
            m.name = mv("Гербовый щит", "Armorial Guard");
            m.telegraph = mv("поднимает щит", "raises a guard");
            m.damage = 0;
            break;
        case MoveKind::Mend:
            m.name = mv("Восковая штопка", "Waxen Mending");
            m.telegraph = mv("латает себя", "mends itself");
            m.damage = 0;
            break;
        case MoveKind::Enrage:
            m.name = mv("Приступ ярости", "Fit of Rage");
            m.telegraph = mv("наливается злобой", "swells with spite");
            m.damage = 0;
            break;
        case MoveKind::Sunder:
            m.name = mv("Раскол щита", "Sunder");
            m.telegraph = mv("целит в защиту", "aims at your guard");
            m.damage = int(base * 1.25f);
            break;
    }
    // Лёгкая стихийная окраска врага
    if (r.chance(0.3f) && m.applies == StatusId::None && m.damage > 0) {
        m.applies = elem(fl).mark;
        m.stacks = 1 + depth / 4;
        m.turns = 2;
    }
    return m;
}

static WeakSpot makeSpot(Rng& r, int depth, int i, bool boss) {
    static const LS names[] = {
        LS("Витражный глаз", "Stained Eye"), LS("Костяная длань", "Bone Hand"),
        LS("Гербовая цепь", "Armorial Chain"), LS("Восковое сердце", "Waxen Heart"),
        LS("Челюсть", "Jaw"), LS("Хребет", "Spine"), LS("Канделябр", "Candelabrum"),
    };
    WeakSpot s;
    s.name = names[size_t(r.i(0, 6))];
    s.maxHp = int((10 + depth * 3.0f) * (boss ? 1.6f : 1.0f) * r.f(0.85f, 1.2f));
    s.hp = s.maxHp;
    // Точки распределяем по обе стороны тела, чтобы они не слипались.
    float side = (i % 2 == 0) ? -1.0f : 1.0f;
    s.offset = Vector2{ side * r.f(0.30f, 0.48f), -0.30f + i * 0.22f + r.f(-0.06f, 0.06f) };
    static const SpotBreak breaks[] = { SpotBreak::RemoveMove, SpotBreak::Stagger,
                                        SpotBreak::Expose, SpotBreak::ForcePhase };
    s.onBreak = breaks[size_t(r.i(0, boss ? 3 : 2))];
    return s;
}

Enemy genEnemy(Rng& r, int depth, bool boss) {
    Enemy e;
    e.seed = uint32_t(r.next());
    e.depth = std::max(1, depth);
    e.boss = boss;
    e.flavour = randomElement(r, depth);

    const LS& pre = r.pick(enemyPrefixes());
    const LS& noun = r.pick(enemyNouns());
    e.name = LS(std::string(pre.ru) + " " + noun.ru, std::string(pre.en) + " " + noun.en);
    e.epithet = boss ? LS("хозяин руин", "master of the ruin")
                     : LS(elem(e.flavour).name.ru, elem(e.flavour).name.en);

    float hpBase = boss ? (150.0f + depth * 46.0f) : (55.0f + depth * 24.0f);
    e.maxHp = int(hpBase * r.f(0.9f, 1.15f));
    e.hp = e.maxHp;
    e.actions = clampi(1 + depth / 6, 1, 3) + (boss ? 1 : 0);

    int spotCount = boss ? r.i(2, 3) : (depth >= 2 ? r.i(1, 2) : r.i(0, 1));
    for (int i = 0; i < spotCount; ++i) e.spots.push_back(makeSpot(r, depth, i, boss));

    int phaseCount = boss ? 3 : (depth >= 4 ? 2 : 1);
    for (int p = 0; p < phaseCount; ++p) {
        EnemyPhase ph;
        ph.hpFraction = (p == 0) ? 1.0f : (1.0f - float(p) / float(phaseCount));
        ph.enterLine = (p == 0)
            ? LS("", "")
            : LS("Оно меняет облик.", "It changes shape.");
        int moves = 3 + p + (boss ? 1 : 0);
        for (int i = 0; i < moves; ++i) {
            MoveKind k;
            if (i == 0) k = MoveKind::Attack;
            else {
                static const MoveKind pool[] = { MoveKind::Attack, MoveKind::Heavy, MoveKind::Multi,
                                                 MoveKind::Curse, MoveKind::Guard, MoveKind::Mend,
                                                 MoveKind::Enrage, MoveKind::Sunder };
                k = pool[size_t(r.i(0, 7))];
                if ((k == MoveKind::Mend || k == MoveKind::Enrage) && !boss && r.chance(0.5f))
                    k = MoveKind::Attack;
            }
            EnemyMove m = makeMove(r, depth + p, k, e.flavour);
            m.damage = int(m.damage * (1.0f + 0.18f * p));
            // Привязываем часть приёмов к уязвимым точкам.
            if (!e.spots.empty() && r.chance(0.5f)) {
                m.spotIndex = r.i(0, int(e.spots.size()) - 1);
                m.fromSpot = true;
                if (e.spots[size_t(m.spotIndex)].linkedMove < 0)
                    e.spots[size_t(m.spotIndex)].linkedMove = i;
            }
            ph.moves.push_back(m);
        }
        e.phases.push_back(std::move(ph));
    }
    return e;
}

} // namespace kv
