#include "core/Cards.h"
#include <algorithm>
#include <cstdio>

namespace shichoy {

static uint32_t s_uid = 1;

Color Card::tint() const {
    if (element != ElementId::None) return elem(element).primary;
    switch (type) {
        case CardType::Spell:  return hex(0x8A4FBF);
        case CardType::Effect: return hex(0x4E8A6B);
        default:               return hex(0xB08A3E);
    }
}

// ── Text builders / Сборка текста карты ───────────────────────────────────
static LS spellVerb(SpellEffect e) {
    switch (e) {
        case SpellEffect::Damage:   return LS("Урон",            "Damage");
        case SpellEffect::Pierce:   return LS("Точный удар",     "Precise hit");
        case SpellEffect::Drain:    return LS("Вытягивание",     "Drain");
        case SpellEffect::Heal:     return LS("Лечение",         "Heal");
        case SpellEffect::Amplify:  return LS("Усиление реакции","Amplify reaction");
        case SpellEffect::Detonate: return LS("Подрыв метки",    "Detonate mark");
        case SpellEffect::Volley:   return LS("Залп",            "Volley");
        default:                    return LS("", "");
    }
}

static LS effectVerb(EffectKind e) {
    switch (e) {
        case EffectKind::Shield:  return LS("Щит",             "Shield");
        case EffectKind::Draw:    return LS("Добор карт",      "Draw cards");
        case EffectKind::LimitUp: return LS("+лимит карт",     "+card limit");
        case EffectKind::Stun:    return LS("Оглушение",       "Stun");
        case EffectKind::Focus:   return LS("Сосредоточение",  "Focus");
        case EffectKind::Cleanse: return LS("Очищение",        "Cleanse");
        case EffectKind::Weaken:  return LS("Ослабление",      "Weaken");
        case EffectKind::Regen:   return LS("Регенерация",     "Regen");
        default:                  return LS("", "");
    }
}

static std::string fmt(const char* f, int a) { char b[128]; snprintf(b, sizeof b, f, a); return b; }
static std::string fmt2(const char* f, int a, int c) { char b[160]; snprintf(b, sizeof b, f, a, c); return b; }

void Card::refreshText() {
    switch (type) {
        case CardType::Element: {
            const ElementDef& e = elem(element);
            text = LS(fmt("Метка стихии. Урон %d. Вторая стихия — реакция.", power),
                      fmt("Elemental mark. %d damage. A second element reacts.", power));
            (void)e;
            break;
        }
        case CardType::Spell: {
            LS v = spellVerb(spell);
            switch (spell) {
                case SpellEffect::Drain:
                    text = LS(fmt2("Урон %d, лечит на %d.", power, amount),
                              fmt2("%d damage, heals %d.", power, amount));
                    break;
                case SpellEffect::Heal:
                    text = LS(fmt("Восстанавливает %d HP.", power),
                              fmt("Restores %d HP.", power));
                    break;
                case SpellEffect::Amplify:
                    text = LS(fmt("Следующая реакция сильнее на %d%%.", amount * 10),
                              fmt("Next reaction is %d%% stronger.", amount * 10));
                    break;
                case SpellEffect::Detonate:
                    text = LS(fmt("Подрывает метку: урон %d, метка снимается.", power),
                              fmt("Detonates the mark: %d damage, mark consumed.", power));
                    break;
                case SpellEffect::Volley:
                    text = LS(fmt2("%d ударов по %d.", amount, power),
                              fmt2("%d hits of %d.", amount, power));
                    break;
                case SpellEffect::Pierce:
                    text = LS(fmt("Урон %d строго по выбранной точке.", power),
                              fmt("%d damage to the selected spot.", power));
                    break;
                default:
                    text = LS(fmt("Урон %d.", power), fmt("%d damage.", power));
                    break;
            }
            (void)v;
            break;
        }
        case CardType::Effect: {
            switch (effect) {
                case EffectKind::Shield:  text = LS(fmt("Щит %d до конца следующего хода.", power),
                                                    fmt("Shield %d until end of next turn.", power)); break;
                case EffectKind::Draw:    text = LS(fmt("Взять %d карт(ы).", amount),
                                                    fmt("Draw %d card(s).", amount)); break;
                case EffectKind::LimitUp: text = LS(fmt("Лимит карт +%d в этом ходу.", amount),
                                                    fmt("Card limit +%d this turn.", amount)); break;
                case EffectKind::Stun:    text = LS(fmt("Оглушение на %d ход(а).", amount),
                                                    fmt("Stun for %d turn(s).", amount)); break;
                case EffectKind::Focus:   text = LS(fmt("Точность и крит +%d%% на 2 хода.", amount * 10),
                                                    fmt("Accuracy and crit +%d%% for 2 turns.", amount * 10)); break;
                case EffectKind::Cleanse: text = LS("Снимает свои негативные статусы.",
                                                    "Removes your negative statuses."); break;
                case EffectKind::Weaken:  text = LS(fmt2("Ослабление %d на %d ход(а).", power, amount),
                                                    fmt2("Weaken %d for %d turn(s).", power, amount)); break;
                case EffectKind::Regen:   text = LS(fmt2("Регенерация %d на %d ход(а).", power, amount),
                                                    fmt2("Regen %d for %d turn(s).", power, amount)); break;
                default: break;
            }
            break;
        }
    }
    if (has(flags, CardFlags::Corrupted))
        text = LS(text.ru + " Порча: возможна отдача.", text.en + " Corrupted: may backlash.");
    if (has(flags, CardFlags::Wild))
        text = LS(text.ru + " Дикая: принимает стихию метки.", text.en + " Wild: copies the current mark.");
    if (has(flags, CardFlags::Precise))
        text = LS(text.ru + " Точная: без штрафа по точкам.", text.en + " Precise: no penalty on weak spots.");
}

// ── Rarity / Редкость ─────────────────────────────────────────────────────
CardRarity rollRarity(Rng& r, int depth) {
    float roll = r.f() * 100.0f - depth * 1.6f;   // глубже — лучше лут
    if (roll < 2.0f)  return CardRarity::Mythic;
    if (roll < 8.0f)  return CardRarity::Legendary;
    if (roll < 20.0f) return CardRarity::Epic;
    if (roll < 40.0f) return CardRarity::Rare;
    if (roll < 66.0f) return CardRarity::Uncommon;
    return CardRarity::Common;
}
static float rarityScale(CardRarity r) {
    static const float k[RARITY_COUNT] = { 1.0f, 1.18f, 1.4f, 1.7f, 2.05f, 2.5f };
    return k[static_cast<int>(r)];
}

static LS joinName(const LS& pre, const LS& noun) {
    return LS(std::string(pre.ru) + " " + noun.ru, std::string(pre.en) + " " + noun.en);
}

// ── Generators / Генераторы ───────────────────────────────────────────────
Card genElementCard(Rng& r, int depth, ElementId forced) {
    Card c;
    c.uid = s_uid++;
    c.type = CardType::Element;
    c.element = (forced != ElementId::None) ? forced : randomElement(r, depth);
    c.rarity = rollRarity(r, depth);
    const ElementDef& e = elem(c.element);
    c.power = int((5 + depth * 1.6f + r.i(0, 3)) * rarityScale(c.rarity));
    c.amount = 1;
    c.cost = 1;
    c.name = LS(e.name.ru, e.name.en);
    if (c.rarity >= CardRarity::Rare) {
        const LS& p = r.pick(spellPrefixes());
        c.name = joinName(p, c.name);
    }
    if (c.rarity >= CardRarity::Epic && r.chance(0.35f)) c.flags |= CardFlags::Wild;
    c.refreshText();
    return c;
}

Card genSpellCard(Rng& r, int depth) {
    Card c;
    c.uid = s_uid++;
    c.type = CardType::Spell;
    c.rarity = rollRarity(r, depth);
    c.element = r.chance(0.55f) ? randomElement(r, depth) : ElementId::None;

    static const SpellEffect pool[] = {
        SpellEffect::Damage, SpellEffect::Damage, SpellEffect::Damage,
        SpellEffect::Volley, SpellEffect::Pierce, SpellEffect::Drain,
        SpellEffect::Heal,   SpellEffect::Amplify, SpellEffect::Detonate
    };
    c.spell = pool[r.i(0, int(sizeof(pool) / sizeof(pool[0])) - 1)];

    float base = (8 + depth * 2.2f + r.i(0, 4)) * rarityScale(c.rarity);
    switch (c.spell) {
        case SpellEffect::Volley:   c.power = int(base * 0.42f); c.amount = r.i(2, 4); break;
        case SpellEffect::Pierce:   c.power = int(base * 1.15f); c.flags |= CardFlags::Precise; break;
        case SpellEffect::Drain:    c.power = int(base * 0.8f);  c.amount = int(base * 0.35f); break;
        case SpellEffect::Heal:     c.power = int(base * 0.9f);  break;
        case SpellEffect::Amplify:  c.power = 0;                 c.amount = r.i(2, 5); break;
        case SpellEffect::Detonate: c.power = int(base * 1.1f);  break;
        default:                    c.power = int(base);         break;
    }
    if (c.power > 0) c.power = std::max(1, c.power);
    c.cost = (c.rarity >= CardRarity::Epic && r.chance(0.4f)) ? 2 : 1;

    c.name = joinName(r.pick(spellPrefixes()), r.pick(spellNouns()));
    if (c.rarity >= CardRarity::Legendary && r.chance(0.5f)) {
        c.flags |= CardFlags::Corrupted;
        c.power = int(c.power * 1.35f);
    }
    c.refreshText();
    return c;
}

Card genEffectCard(Rng& r, int depth) {
    Card c;
    c.uid = s_uid++;
    c.type = CardType::Effect;
    c.rarity = rollRarity(r, depth);
    c.element = ElementId::None;
    c.effect = static_cast<EffectKind>(r.i(0, int(EffectKind::COUNT) - 1));

    float base = (7 + depth * 1.8f) * rarityScale(c.rarity);
    switch (c.effect) {
        case EffectKind::Shield:  c.power = int(base * 1.2f); c.amount = 1; break;
        case EffectKind::Draw:    c.power = 0; c.amount = r.i(1, 3); break;
        case EffectKind::LimitUp: c.power = 0; c.amount = r.i(1, 2); break;
        case EffectKind::Stun:    c.power = 0; c.amount = 1; break;
        case EffectKind::Focus:   c.power = 0; c.amount = r.i(2, 4); break;
        case EffectKind::Cleanse: c.power = 0; c.amount = 1; break;
        case EffectKind::Weaken:  c.power = r.i(2, 4); c.amount = r.i(2, 3); break;
        case EffectKind::Regen:   c.power = int(base * 0.35f); c.amount = r.i(2, 4); break;
        default: break;
    }
    c.cost = 1;
    c.name = joinName(r.pick(spellPrefixes()), r.pick(effectNouns()));
    c.refreshText();
    return c;
}

Card genCard(Rng& r, int depth) {
    int roll = r.i(0, 99);
    if (roll < 45) return genElementCard(r, depth);
    if (roll < 80) return genSpellCard(r, depth);
    return genEffectCard(r, depth);
}

void mutateCard(Card& c, Rng& r, int depth) {
    c.flags |= CardFlags::Mutated;
    const LS& affix = r.pick(affixNames());
    c.name = LS(affix.ru + std::string(" ") + c.name.ru, affix.en + std::string(" ") + c.name.en);

    switch (r.i(0, 4)) {
        case 0: c.power = int(c.power * r.f(1.15f, 1.55f)) + 1; break;
        case 1: c.amount = std::max(1, c.amount + r.i(1, 2)); break;
        case 2:
            if (c.type == CardType::Element || c.element != ElementId::None)
                c.element = randomElement(r, depth);
            else c.power = int(c.power * 1.25f) + 1;
            break;
        case 3:
            c.flags |= CardFlags::Corrupted;
            c.power = int(c.power * 1.45f) + 2;
            break;
        default:
            c.flags |= CardFlags::Wild;
            break;
    }
    if (c.rarity < CardRarity::Mythic && r.chance(0.45f))
        c.rarity = static_cast<CardRarity>(static_cast<int>(c.rarity) + 1);
    c.refreshText();
}

void upgradeCard(Card& c) {
    c.level++;
    c.flags |= CardFlags::Crafted;
    c.power  = int(c.power * 1.22f) + 1;
    if (c.type == CardType::Effect || c.spell == SpellEffect::Volley) c.amount++;
    c.refreshText();
}

std::vector<Card> starterDeck(Rng& r) {
    std::vector<Card> d;
    // Базовый набор: по две карты четырёх стихий + пара заклинаний и эффектов.
    const ElementId base[] = { ElementId::Fire, ElementId::Water, ElementId::Earth, ElementId::Shadow };
    for (ElementId e : base) {
        for (int i = 0; i < 2; ++i) {
            Card c = genElementCard(r, 0, e);
            c.rarity = CardRarity::Common;
            c.power = 6;
            c.name = LS(elem(e).name.ru, elem(e).name.en);
            c.flags = CardFlags::None;
            c.refreshText();
            d.push_back(c);
        }
    }
    for (int i = 0; i < 3; ++i) {
        Card c = genSpellCard(r, 0);
        c.rarity = CardRarity::Common;
        c.cost = 1;
        c.flags = CardFlags::None;
        c.refreshText();
        d.push_back(c);
    }
    {   // гарантированный «точный удар» — чтобы уязвимые точки были доступны с первого забега
        Card c;
        c.uid = s_uid++;
        c.type = CardType::Spell;
        c.spell = SpellEffect::Pierce;
        c.rarity = CardRarity::Common;
        c.power = 9;
        c.flags = CardFlags::Precise;
        c.name = LS("Трость-шпага", "Cane Sword");
        c.refreshText();
        d.push_back(c);
    }
    for (int i = 0; i < 2; ++i) {
        Card c = genEffectCard(r, 0);
        c.rarity = CardRarity::Common;
        c.flags = CardFlags::None;
        c.refreshText();
        d.push_back(c);
    }
    return d;
}

// ── Deck ──────────────────────────────────────────────────────────────────
void Deck::reset(std::vector<Card> cards) {
    library_ = std::move(cards);
    drawPile_.clear();
    discardPile_.clear();
    exhausted_.clear();
}

void Deck::shuffleDraw(Rng& r) {
    for (int i = int(drawPile_.size()) - 1; i > 0; --i) {
        int j = r.i(0, i);
        std::swap(drawPile_[size_t(i)], drawPile_[size_t(j)]);
    }
}

void Deck::beginCombat(Rng& r) {
    drawPile_ = library_;
    discardPile_.clear();
    exhausted_.clear();
    shuffleDraw(r);
}

bool Deck::draw(Card& out, Rng& r) {
    if (drawPile_.empty()) {
        if (discardPile_.empty()) return false;
        drawPile_ = discardPile_;
        discardPile_.clear();
        shuffleDraw(r);
    }
    out = drawPile_.back();
    drawPile_.pop_back();
    return true;
}

void Deck::discard(const Card& c) { discardPile_.push_back(c); }
void Deck::exhaust(const Card& c) { exhausted_.push_back(c); }

} // namespace shichoy
