#include "core/Merge.h"
#include <algorithm>
#include <cstdio>

namespace shichoy {

// ── Ступени лестницы ──────────────────────────────────────────────────────
int tierRank(MergeTier t) {
    switch (t) {
        case MergeTier::Base:   return 0;
        case MergeTier::T1:     return 1;
        case MergeTier::T2:
        case MergeTier::T2plus: return 2;
        case MergeTier::T3:
        case MergeTier::T3plus: return 3;
        case MergeTier::Ultima: return 4;
    }
    return 0;
}

float tierMult(MergeTier t) {
    switch (t) {
        case MergeTier::Base:   return 1.00f;
        case MergeTier::T1:     return 2.00f;
        case MergeTier::T2:     return 3.00f;
        case MergeTier::T2plus: return 3.75f;
        case MergeTier::T3:     return 5.00f;
        case MergeTier::T3plus: return 6.50f;
        case MergeTier::Ultima: return 10.0f;
    }
    return 1.0f;
}

const char* tierLabel(MergeTier t) {
    switch (t) {
        case MergeTier::Base:   return "";
        case MergeTier::T1:     return "I";
        case MergeTier::T2:     return "II";
        case MergeTier::T2plus: return "II+";
        case MergeTier::T3:     return "III";
        case MergeTier::T3plus: return "III+";
        case MergeTier::Ultima: return "ULT";
    }
    return "";
}

LS tierName(MergeTier t) {
    switch (t) {
        case MergeTier::Base:   return LS("Чистая стихия", "Pure element");
        case MergeTier::T1:     return LS("Реакция", "Reaction");
        case MergeTier::T2:     return LS("Составная", "Compound");
        case MergeTier::T2plus: return LS("Столкновение", "Collision");
        case MergeTier::T3:     return LS("Каскад", "Cascade");
        case MergeTier::T3plus: return LS("Схождение", "Convergence");
        case MergeTier::Ultima: return LS("УЛЬТИМА", "ULTIMA");
    }
    return LS("", "");
}

// ── Вспомогательное ───────────────────────────────────────────────────────
static bool hasElem(const std::vector<ElementId>& v, ElementId e) {
    return std::find(v.begin(), v.end(), e) != v.end();
}

static std::vector<ElementId> unite(const std::vector<ElementId>& a,
                                    const std::vector<ElementId>& b) {
    std::vector<ElementId> out = a;
    for (ElementId e : b) if (!hasElem(out, e)) out.push_back(e);
    return out;
}

static int mergedPower(const Card& a, const Card& b) {
    // Растёт подлинейно: иначе цепочка из восьми карт улетала бы в космос.
    int hi = std::max(a.power, b.power), lo = std::min(a.power, b.power);
    return hi + lo / 2;
}

static int tierCost(MergeTier t) {
    switch (t) {
        case MergeTier::T2:
        case MergeTier::T2plus:
        case MergeTier::T3:
        case MergeTier::T3plus: return 2;
        case MergeTier::Ultima: return 3;
        default:                return 1;
    }
}

// Имена растут склейкой, но с потолком: длинные заменяем на «Каскад из N стихий».
static LS buildName(MergeTier tier, const Card& a, const Card& b,
                    const std::vector<ElementId>& chain) {
    if (tier == MergeTier::Ultima) {
        static const LS pool[] = {
            LS("Последний Аркан", "The Last Arcanum"),
            LS("Двенадцатый Час", "The Twelfth Hour"),
            LS("Реквием по Дому", "Requiem for the House"),
            LS("Приговор Всем Стихиям", "Verdict Upon All Elements"),
            LS("Полночь Поместья", "Midnight of the Manor"),
        };
        const LS& p = pool[chain.size() % 5];
        char ru[160], en[160];
        snprintf(ru, sizeof ru, "%s (%d стихий)", p.ru.c_str(), int(chain.size()));
        snprintf(en, sizeof en, "%s (%d elements)", p.en.c_str(), int(chain.size()));
        return LS(ru, en);
    }

    if (tier == MergeTier::T1) {
        ReactionDef rc = reactionFor(a.element, b.element);
        if (rc.valid) return rc.name;
    }

    auto part = [](const Card& c) -> LS {
        if (c.tier == MergeTier::Base) return LS(elem(c.element).name.ru, elem(c.element).name.en);
        return c.name;
    };
    LS pa = part(a), pb = part(b);
    std::string ru = pa.ru + " · " + pb.ru;
    std::string en = pa.en + " · " + pb.en;
    // Пороги в байтах: кириллица в UTF-8 занимает по два на символ.
    if (ru.size() > 62 || en.size() > 38) {
        LS tn = tierName(tier);
        char bru[128], ben[128];
        snprintf(bru, sizeof bru, "%s из %d стихий", tn.ru.c_str(), int(chain.size()));
        snprintf(ben, sizeof ben, "%s of %d elements", tn.en.c_str(), int(chain.size()));
        return LS(bru, ben);
    }
    return LS(ru, en);
}

static LS buildEffect(MergeTier tier, const std::vector<ElementId>& chain) {
    int n = int(chain.size());
    char ru[320], en[320];
    switch (tier) {
        case MergeTier::T1:
            snprintf(ru, sizeof ru, "Урон x2. Вешает метку реакции.");
            snprintf(en, sizeof en, "x2 damage. Applies the reaction's status.");
            break;
        case MergeTier::T2:
            snprintf(ru, sizeof ru, "Урон x3. Копит статусы всех %d стихий цепи.", n);
            snprintf(en, sizeof en, "x3 damage. Stacks statuses of all %d chained elements.", n);
            break;
        case MergeTier::T2plus:
            snprintf(ru, sizeof ru, "Урон x3.75. Пробивает щит, статусы цепи усилены.");
            snprintf(en, sizeof en, "x3.75 damage. Shreds guard, chain statuses amplified.");
            break;
        case MergeTier::T3:
            snprintf(ru, sizeof ru, "Урон x5 по корпусу и по каждой уязвимой точке разом.");
            snprintf(en, sizeof en, "x5 to the body and to every weak spot at once.");
            break;
        case MergeTier::T3plus:
            snprintf(ru, sizeof ru, "Урон x6.5 по всему, все статусы цепи и оглушение на ход.");
            snprintf(en, sizeof en, "x6.5 to everything, all chain statuses, stun for a turn.");
            break;
        case MergeTier::Ultima:
            snprintf(ru, sizeof ru,
                     "Урон x10 и ещё 20%% текущего HP врага. Игнорирует щит, "
                     "оглушает, вешает все %d статусов. Один раз за бой.", n);
            snprintf(en, sizeof en,
                     "x10 damage plus 20%% of the enemy's current HP. Ignores guard, "
                     "stuns, applies all %d statuses. Once per combat.", n);
            break;
        default:
            return LS("", "");
    }
    return LS(ru, en);
}

// ── Правила лестницы ──────────────────────────────────────────────────────
MergeInfo evaluateMerge(const Card& a, const Card& b) {
    MergeInfo r;

    // Зачарование: заклинание принимает чистую стихию и начинает вешать её метку.
    if (a.type == CardType::Spell && b.type == CardType::Element && b.tier == MergeTier::Base) {
        if (a.chain.size() >= 2) {
            r.reason = LS("Заклинание уже несёт две стихии.",
                          "The spell already carries two elements.");
            return r;
        }
        if (hasElem(a.chain, b.element)) {
            r.reason = LS("Эта стихия уже вплетена в заклинание.",
                          "That element is already woven in.");
            return r;
        }
        r.ok = true;
        r.enchant = true;
        r.tier = MergeTier::Base;
        r.mult = 1.0f;
        r.chain = unite(a.chain, b.chain);
        r.power = int(a.power * 1.25f) + 1;
        r.cost = a.cost;
        r.name = LS(std::string(elem(b.element).name.ru) + " " + a.name.ru,
                    std::string(elem(b.element).name.en) + " " + a.name.en);
        r.effect = LS(std::string("Заклинание принимает стихию «") + elem(b.element).name.ru
                          + "»: сила +25%, удар вешает её метку.",
                      std::string("The spell takes on ") + elem(b.element).name.en
                          + ": +25% power, its strike applies that mark.");
        return r;
    }

    if (a.type != CardType::Element || b.type != CardType::Element) {
        r.reason = a.type == CardType::Effect || b.type == CardType::Effect
            ? LS("Карты-эффекты не смешиваются.", "Effect cards do not merge.")
            : LS("Стихию можно вплести в заклинание, но не наоборот.",
                 "An element weaves into a spell, not the other way round.");
        return r;
    }
    if (a.tier == MergeTier::Ultima || b.tier == MergeTier::Ultima) {
        r.reason = LS("Ultima дальше не смешивается — её разыгрывают.",
                      "An Ultima merges no further — play it.");
        return r;
    }

    std::vector<ElementId> chain = unite(a.chain, b.chain);
    if (chain.size() <= a.chain.size()) {
        r.reason = LS("Нужна стихия, которой ещё нет в цепочке.",
                      "Needs an element the chain does not have yet.");
        return r;
    }

    const int ra = tierRank(a.tier), rb = tierRank(b.tier);
    const int lo = std::min(ra, rb), hi = std::max(ra, rb);

    MergeTier tier;
    if (hi == 0)                        tier = MergeTier::T1;        // база + база
    else if (hi == 1 && lo == 0)        tier = MergeTier::T2;        // ур.1 + стихия
    else if (hi == 1 && lo == 1)        tier = MergeTier::T2plus;    // ур.1 + ур.1
    else if (hi == 2 && lo <= 1)        tier = MergeTier::T3;        // ур.2 + стихия/ур.1
    else if (hi == 2 && lo == 2)        tier = MergeTier::T3plus;    // ур.2 + ур.2
    else if (hi == 3 && lo == 3) {                                   // ур.3 + ур.3
        if (chain.size() < 5) {
            char ru[160], en[160];
            snprintf(ru, sizeof ru, "Для Ultima нужно 5 разных стихий, набрано %d.",
                     int(chain.size()));
            snprintf(en, sizeof en, "Ultima needs 5 distinct elements, you have %d.",
                     int(chain.size()));
            r.reason = LS(ru, en);
            return r;
        }
        tier = MergeTier::Ultima;
    } else {
        r.reason = LS("Уровень III смешивается только с другим III.",
                      "Tier III only merges with another III.");
        return r;
    }

    r.ok = true;
    r.tier = tier;
    r.mult = tierMult(tier);
    r.chain = chain;
    r.power = mergedPower(a, b);
    r.cost = tierCost(tier);
    r.name = buildName(tier, a, b, chain);
    r.effect = buildEffect(tier, chain);
    return r;
}

Card applyMerge(const Card& a, const Card& b, const MergeInfo& info) {
    Card c = a;

    if (info.enchant) {                       // заклинание остаётся заклинанием
        c.element = info.chain.empty() ? a.element : info.chain.back();
        c.chain = info.chain;
        c.power = info.power;
        c.name = info.name;
        c.refreshText();
        c.text = LS(c.text.ru + " Стихия: " + elem(c.element).name.ru + ".",
                    c.text.en + " Element: " + elem(c.element).name.en + ".");
        if (c.rarity < CardRarity::Mythic)
            c.rarity = CardRarity(int(c.rarity) + 1);
        return c;
    }

    c.tier = info.tier;
    c.chain = info.chain;
    c.power = info.power;
    c.cost = info.cost;
    c.name = info.name;
    c.text = info.effect;
    c.element = info.chain.empty() ? a.element : info.chain.front();
    c.flags |= b.flags;                       // порча и дикость передаются дальше
    c.level = std::max(a.level, b.level);

    int rank = tierRank(info.tier);
    c.rarity = CardRarity(clampi(int(a.rarity) + rank, 0, RARITY_COUNT - 1));
    if (info.tier == MergeTier::Ultima) c.rarity = CardRarity::Mythic;
    return c;
}

} // namespace shichoy
