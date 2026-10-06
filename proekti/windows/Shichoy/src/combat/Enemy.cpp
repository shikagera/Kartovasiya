#include "combat/Enemy.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace shichoy {

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

LS archetypeName(Archetype a) {
    switch (a) {
        case Archetype::Reaper:      return LS("Жнец", "Reaper");
        case Archetype::Candelabrum: return LS("Канделябр", "Candelabrum");
        case Archetype::Portrait:    return LS("Портрет", "Portrait");
        case Archetype::Hound:       return LS("Егерь", "Huntsman");
        case Archetype::Automaton:   return LS("Автоматон", "Automaton");
        case Archetype::Choir:       return LS("Хор", "Choir");
        case Archetype::Doll:        return LS("Кукла", "Doll");
        default:                     return LS("Нечто", "Something");
    }
}

// Каждый облик тянет к своим приёмам — повадки читаются по силуэту.
static MoveKind archeMove(Archetype a, Rng& r) {
    switch (a) {
        case Archetype::Candelabrum: {
            static const MoveKind p[] = { MoveKind::Curse, MoveKind::Multi, MoveKind::Attack,
                                          MoveKind::Enrage, MoveKind::Curse };
            return p[r.i(0, 4)];
        }
        case Archetype::Portrait: {
            static const MoveKind p[] = { MoveKind::Curse, MoveKind::Guard, MoveKind::Mend,
                                          MoveKind::Curse, MoveKind::Attack };
            return p[r.i(0, 4)];
        }
        case Archetype::Hound: {
            static const MoveKind p[] = { MoveKind::Multi, MoveKind::Attack, MoveKind::Multi,
                                          MoveKind::Attack, MoveKind::Heavy };
            return p[r.i(0, 4)];
        }
        case Archetype::Automaton: {
            static const MoveKind p[] = { MoveKind::Guard, MoveKind::Sunder, MoveKind::Heavy,
                                          MoveKind::Attack, MoveKind::Guard };
            return p[r.i(0, 4)];
        }
        case Archetype::Choir: {
            static const MoveKind p[] = { MoveKind::Curse, MoveKind::Enrage, MoveKind::Multi,
                                          MoveKind::Curse, MoveKind::Heavy };
            return p[r.i(0, 4)];
        }
        case Archetype::Doll: {
            static const MoveKind p[] = { MoveKind::Mend, MoveKind::Guard, MoveKind::Enrage,
                                          MoveKind::Attack, MoveKind::Curse };
            return p[r.i(0, 4)];
        }
        default: {
            static const MoveKind p[] = { MoveKind::Attack, MoveKind::Heavy, MoveKind::Multi,
                                          MoveKind::Curse, MoveKind::Sunder, MoveKind::Guard,
                                          MoveKind::Mend, MoveKind::Enrage };
            return p[r.i(0, 7)];
        }
    }
}

// Названия приёмов подгоняются под облик — «Удар тростью» у пса ни к чему.
static void flavourMove(EnemyMove& m, Archetype a) {
    switch (a) {
        case Archetype::Candelabrum:
            if (m.kind == MoveKind::Attack) { m.name = mv("Капля воска", "Drop of Wax");
                                              m.telegraph = mv("наклоняет пламя", "tips its flame"); }
            if (m.kind == MoveKind::Multi)  { m.name = mv("Град искр", "Hail of Sparks");
                                              m.telegraph = mv("разгорается", "flares up"); }
            break;
        case Archetype::Portrait:
            if (m.kind == MoveKind::Attack) { m.name = mv("Мазок кистью", "Brushstroke");
                                              m.telegraph = mv("тянется из рамы", "leans out of the frame"); }
            if (m.kind == MoveKind::Guard)  { m.name = mv("Слой лака", "Coat of Varnish");
                                              m.telegraph = mv("покрывается лаком", "glazes over"); }
            break;
        case Archetype::Hound:
            if (m.kind == MoveKind::Attack) { m.name = mv("Хватка", "Snap");
                                              m.telegraph = mv("прижимается к земле", "crouches low"); }
            if (m.kind == MoveKind::Multi)  { m.name = mv("Свора", "The Pack");
                                              m.telegraph = mv("заходит кругом", "circles in"); }
            if (m.kind == MoveKind::Heavy)  { m.name = mv("Загон", "Run to Ground");
                                              m.telegraph = mv("берёт разгон", "gathers speed"); }
            break;
        case Archetype::Automaton:
            if (m.kind == MoveKind::Attack) { m.name = mv("Шатунный удар", "Piston Blow");
                                              m.telegraph = mv("заводит механизм", "winds its works"); }
            if (m.kind == MoveKind::Guard)  { m.name = mv("Латунный кожух", "Brass Casing");
                                              m.telegraph = mv("смыкает пластины", "closes its plates"); }
            if (m.kind == MoveKind::Sunder) { m.name = mv("Клещи", "The Pincers");
                                              m.telegraph = mv("разводит клещи", "spreads its pincers"); }
            break;
        case Archetype::Choir:
            if (m.kind == MoveKind::Curse)  { m.name = mv("Литания", "Litany");
                                              m.telegraph = mv("набирает воздух", "draws breath"); }
            if (m.kind == MoveKind::Multi)  { m.name = mv("Многоголосье", "Many Voices");
                                              m.telegraph = mv("рты открываются разом", "every mouth opens"); }
            break;
        case Archetype::Doll:
            if (m.kind == MoveKind::Mend)   { m.name = mv("Новый стежок", "A Fresh Stitch");
                                              m.telegraph = mv("зашивает шов", "sews a seam"); }
            if (m.kind == MoveKind::Attack) { m.name = mv("Фарфоровая ладонь", "Porcelain Palm");
                                              m.telegraph = mv("поднимает руку", "raises a hand"); }
            break;
        default: break;
    }
}

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

Enemy genEnemy(Rng& r, int depth, bool boss, Archetype forcedArche, ElementId forcedFlavour) {
    Enemy e;
    e.seed = uint32_t(r.next());
    e.depth = std::max(1, depth);
    e.boss = boss;
    e.flavour = (forcedFlavour != ElementId::None) ? forcedFlavour : randomElement(r, depth);

    const LS& pre = r.pick(enemyPrefixes());
    const LS& noun = r.pick(enemyNouns());
    e.name = LS(std::string(pre.ru) + " " + noun.ru, std::string(pre.en) + " " + noun.en);
    e.arche = (forcedArche != Archetype::COUNT) ? forcedArche
                                                : Archetype(r.i(0, int(Archetype::COUNT) - 1));
    e.epithet = boss ? LS("хозяин руин", "master of the ruin")
                     : LS(std::string(archetypeName(e.arche).ru) + ", " + elem(e.flavour).name.ru,
                          std::string(archetypeName(e.arche).en) + ", " + elem(e.flavour).name.en);

    float hpBase = boss ? (150.0f + depth * 46.0f) : (55.0f + depth * 24.0f);
    // Облик правит телосложение: пёс лёгкий и быстрый, автоматон — наоборот.
    switch (e.arche) {
        case Archetype::Hound:     hpBase *= 0.80f; break;
        case Archetype::Automaton: hpBase *= 1.25f; break;
        case Archetype::Doll:      hpBase *= 1.10f; break;
        case Archetype::Choir:     hpBase *= 0.92f; break;
        default: break;
    }
    e.maxHp = int(hpBase * r.f(0.9f, 1.15f));
    e.hp = e.maxHp;
    e.actions = clampi(1 + depth / 6, 1, 3) + (boss ? 1 : 0);
    if (e.arche == Archetype::Hound) e.actions++;              // пёс бьёт чаще
    if (e.arche == Archetype::Automaton) e.guard += 4 + depth; // и сразу в броне

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
                k = archeMove(e.arche, r);
                if ((k == MoveKind::Mend || k == MoveKind::Enrage) && !boss && r.chance(0.4f))
                    k = MoveKind::Attack;
            }
            EnemyMove m = makeMove(r, depth + p, k, e.flavour);
            flavourMove(m, e.arche);
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


// ── Именные боссы ─────────────────────────────────────────────────────────
// Босс перестал быть «усиленным врагом»: у каждого своё имя, облик, стихия,
// реплики на смене фаз и фирменный приём в последней фазе.
static std::vector<BossDef> buildBosses() {
    std::vector<BossDef> v;
    v.reserve(16);
    auto add = [&](LS n, LS ep, Archetype a, ElementId f, float hp, int minD,
                   LS p2, LS p3, MoveKind sk, LS sn, LS st, int sd) {
        BossDef b;
        b.name = std::move(n); b.epithet = std::move(ep);
        b.arche = a; b.flavour = f; b.hpMul = hp; b.minDepth = minD;
        b.phase2 = std::move(p2); b.phase3 = std::move(p3);
        b.sigKind = sk; b.sigName = std::move(sn); b.sigTelegraph = std::move(st);
        b.sigDamage = sd;
        v.push_back(std::move(b));
    };

    add(LS("Душеприказчик Вечерних Комнат", "Executor of the Evening Rooms"),
        LS("он дочитывает завещание", "he is still reading the will"),
        Archetype::Portrait, ElementId::Shadow, 1.00f, 1,
        LS("Рама трещит. Он подаётся вперёд.", "The frame cracks. He leans forward."),
        LS("Холст рвётся по шву.", "The canvas tears along the seam."),
        MoveKind::Heavy, LS("Последняя воля", "The Last Will"),
        LS("разворачивает бумагу", "unrolls the paper"), 22);

    add(LS("Канделябр Святого Молчания", "Candelabrum of Holy Silence"),
        LS("семь огней, ни одного слова", "seven flames, not one word"),
        Archetype::Candelabrum, ElementId::Light, 0.92f, 1,
        LS("Гаснет первый рожок.", "The first horn goes out."),
        LS("Остаётся одно пламя, и оно белое.", "One flame remains, and it is white."),
        MoveKind::Multi, LS("Семисвечие", "The Sevenfold"),
        LS("разводит огни веером", "fans its flames apart"), 9);

    add(LS("Гончая Последней Охоты", "Hound of the Last Hunt"),
        LS("она помнит запах всех, кто бежал", "it remembers everyone who ran"),
        Archetype::Hound, ElementId::Blood, 0.84f, 2,
        LS("Она перестаёт кружить.", "It stops circling."),
        LS("Шерсть на загривке встаёт дыбом.", "The hackles rise."),
        MoveKind::Multi, LS("Загон", "The Running Down"),
        LS("прижимается к земле", "crouches low"), 11);

    add(LS("Часовщик Обратного Хода", "Clockmaker of the Reverse Stroke"),
        LS("он чинит то, что ещё не сломалось", "he mends what is not yet broken"),
        Archetype::Automaton, ElementId::Metal, 1.22f, 3,
        LS("Шестерни меняют направление.", "The gears reverse."),
        LS("Внутри что-то считает вслух.", "Something inside counts aloud."),
        MoveKind::Sunder, LS("Обратный ход", "The Reverse Stroke"),
        LS("заводит пружину до упора", "winds the spring to its limit"), 20);

    add(LS("Хор Утонувших", "Choir of the Drowned"),
        LS("они поют на вдохе", "they sing on the inhale"),
        Archetype::Choir, ElementId::Water, 0.95f, 3,
        LS("Вода поднимается до второго ряда.", "The water reaches the second row."),
        LS("Поют уже все рты разом.", "Every mouth sings at once."),
        MoveKind::Curse, LS("Вдох", "The Inhale"),
        LS("набирает воздух за всех", "draws breath for all of them"), 14);

    add(LS("Кукла в Подвенечном", "The Doll in Bridal White"),
        LS("её ждали к шести", "she was expected at six"),
        Archetype::Doll, ElementId::Poison, 1.08f, 4,
        LS("Шов на плече расходится.", "The shoulder seam gives way."),
        LS("Из разрыва идёт сладкий запах.", "A sweet smell comes from the tear."),
        MoveKind::Curse, LS("Приданое", "The Dowry"),
        LS("протягивает раскрытую ладонь", "holds out an open palm"), 13);

    add(LS("Жнец Пустых Полей", "Reaper of the Empty Fields"),
        LS("он давно сжал всё, что росло", "he reaped all that grew, long ago"),
        Archetype::Reaper, ElementId::Earth, 1.05f, 4,
        LS("Плащ тяжелеет от земли.", "The cloak grows heavy with soil."),
        LS("Под капюшоном нет лица. И не было.", "There is no face under the hood. There never was."),
        MoveKind::Heavy, LS("Жатва", "The Harvest"),
        LS("заносит косу за спину", "draws the scythe back"), 26);

    add(LS("Витражный Епископ", "The Stained-Glass Bishop"),
        LS("свет проходит сквозь него и не греет", "light passes through him and does not warm"),
        Archetype::Portrait, ElementId::Crystal, 1.14f, 5,
        LS("По стеклу идёт первая трещина.", "The first crack runs through the glass."),
        LS("Он рассыпается и собирается снова.", "He shatters and gathers himself again."),
        MoveKind::Multi, LS("Преломление", "The Refraction"),
        LS("поворачивается к свету", "turns toward the light"), 12);

    add(LS("Громовая Псарня", "The Thunder Kennel"),
        LS("их держали на одной цепи", "they were kept on a single chain"),
        Archetype::Hound, ElementId::Lightning, 0.88f, 6,
        LS("Цепь рвётся.", "The chain snaps."),
        LS("Теперь их слышно со всех сторон.", "Now they are heard from every side."),
        MoveKind::Multi, LS("Свора", "The Pack"),
        LS("заходит кругом", "circles in"), 13);

    add(LS("Молчаливый Нотариус", "The Silent Notary"),
        LS("он заверяет всё, к чему прикоснулся", "he notarises all he touches"),
        Archetype::Automaton, ElementId::Aether, 1.30f, 7,
        LS("Печать опускается впервые.", "The seal comes down for the first time."),
        LS("Он заверяет ваше имя.", "He notarises your name."),
        MoveKind::Sunder, LS("Заверение", "The Notarisation"),
        LS("поднимает печать", "raises the seal"), 24);

    add(LS("Ледяная Вдова", "The Ice Widow"),
        LS("она не сняла траур и не снимет", "she has not put off mourning and will not"),
        Archetype::Doll, ElementId::Ice, 1.16f, 8,
        LS("Вуаль покрывается инеем.", "The veil takes on frost."),
        LS("В зале становится нечем дышать.", "The hall becomes hard to breathe in."),
        MoveKind::Heavy, LS("Поминовение", "The Remembrance"),
        LS("складывает руки на груди", "folds her hands at her breast"), 28);

    add(LS("Пепельный Хорал", "The Ashen Chorale"),
        LS("допевает за тех, кто не смог", "it finishes the song for those who could not"),
        Archetype::Choir, ElementId::Fire, 1.25f, 10,
        LS("Пепел поднимается со всех рядов.", "Ash lifts from every row."),
        LS("Поют уже не рты, а пустоты.", "It is no longer mouths that sing, but hollows."),
        MoveKind::Heavy, LS("Финал", "The Finale"),
        LS("набирает воздух в последний раз", "draws breath one final time"), 32);

    return v;
}

static const std::vector<BossDef>& bosses() {
    static const std::vector<BossDef> b = buildBosses();
    return b;
}

int bossCount() { return int(bosses().size()); }

const BossDef& bossDef(int i) {
    const auto& b = bosses();
    if (i < 0 || i >= int(b.size())) return b.front();
    return b[size_t(i)];
}

// Выбор из тех, кто уже «по глубине»: на мелководье это первые имена, глубже
// список расширяется, так что порядок знакомства остаётся осмысленным.
int pickBossIndex(Rng& r, int depth) {
    std::vector<int> pool;
    for (int i = 0; i < bossCount(); ++i)
        if (bosses()[size_t(i)].minDepth <= depth) pool.push_back(i);
    if (pool.empty()) return 0;
    return pool[size_t(r.i(0, int(pool.size()) - 1))];
}

Enemy genNamedBoss(Rng& r, int depth, int index) {
    const BossDef& d = bossDef(index);
    // Тело строим обычным путём, затем надеваем на него личность.
    Enemy e = genEnemy(r, depth, true, d.arche, d.flavour);
    e.name = d.name;
    e.epithet = d.epithet;
    e.maxHp = std::max(1, int(e.maxHp * d.hpMul));
    e.hp = e.maxHp;

    if (e.phases.size() > 1) e.phases[1].enterLine = d.phase2;
    if (e.phases.size() > 2) e.phases[2].enterLine = d.phase3;

    // Фирменный приём кладём в последнюю фазу: бой запоминается концом.
    if (!e.phases.empty()) {
        EnemyMove m = makeMove(r, depth, d.sigKind, d.flavour);
        m.name = d.sigName;
        m.telegraph = d.sigTelegraph;
        m.damage = d.sigDamage + depth;
        m.fromSpot = false;
        m.spotIndex = -1;
        e.phases.back().moves.push_back(std::move(m));
    }
    return e;
}

} // namespace shichoy
