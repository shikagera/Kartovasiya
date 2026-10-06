#include "combat/Combat.h"
#include "core/Merge.h"
#include "audio/ProcAudio.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace shichoy {

// ── Player ────────────────────────────────────────────────────────────────
int Player::statusStacks(StatusId s) const {
    for (const auto& st : statuses) if (st.id == s) return st.stacks;
    return 0;
}
void Player::addStatus(StatusId s, int stacks, int turns) {
    if (s == StatusId::None || stacks <= 0) return;
    for (auto& st : statuses) {
        if (st.id == s) { st.stacks += stacks; st.turns = std::max(st.turns, turns); return; }
    }
    statuses.push_back(StatusInst{ s, stacks, turns });
}
void Player::removeNegative() {
    statuses.erase(std::remove_if(statuses.begin(), statuses.end(), [](const StatusInst& s) {
        return !statusDef(s.id).onPlayer;
    }), statuses.end());
}

Vector2 spotScreenPos(const Enemy& e, int index, Vector2 center, float size) {
    if (index < 0 || index >= int(e.spots.size())) return center;
    const WeakSpot& s = e.spots[size_t(index)];
    return Vector2{ center.x + s.offset.x * size * 1.35f, center.y + s.offset.y * size * 1.70f };
}

// ── Helpers ───────────────────────────────────────────────────────────────
static std::string num(int v) { char b[24]; snprintf(b, sizeof b, "%d", v); return b; }

int Combat::intentHits() const {
    const EnemyMove* m = enemy_.currentMove();
    return m ? m->hits * std::max(1, enemy_.actions) : 0;
}
int Combat::intentDamage() const {
    const EnemyMove* m = enemy_.currentMove();
    if (!m) return 0;
    if (m->kind == MoveKind::Guard || m->kind == MoveKind::Mend ||
        m->kind == MoveKind::Enrage || m->kind == MoveKind::Curse) return 0;
    return std::max(0, m->damage + enemy_.enrage);
}

void Combat::pushFloat(const std::string& s, Vector2 p, Color c, float size) {
    FloatText f;
    f.s = s; f.pos = p; f.color = c; f.maxLife = 1.1f; f.life = f.maxLife; f.size = size;
    floats_.push_back(f);
}
void Combat::pushFx(ElementId a, ElementId b, Color tint, float power, bool reaction) {
    fxEvents_.push_back(FxEvent{ a, b, tint, power, reaction });
}

void Combat::pushLog(const LS& t, Color c) {
    log_.push_back(LogLine{ t, c, 0.0f });
    if (log_.size() > 40) log_.erase(log_.begin());
}

// ── Lifecycle ─────────────────────────────────────────────────────────────
void Combat::begin(Deck* deck, Player* player, Enemy enemy, int depth, Rng* rng) {
    deck_ = deck; player_ = player; rng_ = rng;
    enemy_ = std::move(enemy);
    depth_ = depth;
    hand_.clear(); log_.clear(); floats_.clear();
    rewards_ = CombatRewards{};
    state_ = CombatState::PlayerTurn;
    turn_ = 1;
    target_ = -1;
    mark_ = ElementId::None;
    markTurns_ = 0;
    ampBonus_ = 0.0f;
    ultimaUsed_ = false;
    undyingUsed_ = false;
    firstCardTurn_ = true;
    shake_ = 0.0f;
    reactionFlash_ = 0.0f;
    lastReaction_ = LS("", "");
    player_->shield = 0;

    deck_->beginCombat(*rng_);
    pushLog(LS(std::string("Стол разложен. Против вас — ") + enemy_.name.ru + ".",
               std::string("The table is set. Facing you: ") + enemy_.name.en + "."), hex(0xE0D4B4));
    startPlayerTurn();
}

void Combat::startPlayerTurn() {
    cardLimit_ = player_->baseCardLimit + (player_->has(RelicId::ExtraSlot) ? 1 : 0);
    cardsLeft_ = cardLimit_;
    firstCardTurn_ = true;
    state_ = CombatState::PlayerTurn;

    // Рука между ходами не сбрасывается: всё несыгранное остаётся лежать,
    // сверху приходит фиксированный добор. Потолка у руки нет.
    drawHand(player_->drawPerTurn + (player_->has(RelicId::DeepDraw) ? 2 : 0));

    if (const EnemyMove* m = enemy_.currentMove()) {
        std::string mult = enemy_.actions > 1 ? ("  x" + num(enemy_.actions)) : std::string();
        telegraph_ = LS(std::string(enemy_.name.ru) + " " + m->telegraph.ru + " — " + m->name.ru + mult,
                        std::string(enemy_.name.en) + " " + m->telegraph.en + " — " + m->name.en + mult);
    }
}

void Combat::drawHand(int n) {
    for (int i = 0; i < n; ++i) {
        Card c;
        if (!deck_->draw(c, *rng_)) break;
        hand_.push_back(c);
    }
}

void Combat::discardHand() {
    for (const auto& c : hand_) deck_->discard(c);
    hand_.clear();
}

void Combat::cycleTarget(int dir) {
    int n = int(enemy_.spots.size());
    if (n == 0) { target_ = -1; return; }
    for (int k = 0; k < n + 1; ++k) {
        target_ += dir;
        if (target_ >= n) target_ = -1;
        if (target_ < -1) target_ = n - 1;
        if (target_ == -1) return;
        if (!enemy_.spots[size_t(target_)].destroyed) return;
    }
    target_ = -1;
}

// Место боя усиливает свои стихии — исследование становится подготовкой.
float Combat::biomeMul(const Card& c) const {
    const BiomeMod& m = biomeMod(biome_);
    if (biomeFavours(biome_, c.element)) return 1.0f + m.bonus;
    for (ElementId e : c.chain)
        if (biomeFavours(biome_, e)) return 1.0f + m.bonus;
    return 1.0f;
}

// ── Damage / Урон ─────────────────────────────────────────────────────────
int Combat::dealDamageToEnemy(int dmg, bool precise, Color col) {
    if (dmg <= 0) return 0;

    // Размер числа — от доли снятого здоровья. Мелкий тычок и удар на треть
    // полосы не должны выглядеть одинаково: на этом держится ощущение силы.
    auto hitSize = [this](int d) {
        float f = enemy_.maxHp > 0 ? float(d) / float(enemy_.maxHp) : 0.0f;
        return 26.0f + clampf(f * 3.2f, 0.0f, 1.0f) * 40.0f;
    };

    // Модификаторы: Разряд, Уязвимость, Коррозия, обнажённая цель
    int shock = enemy_.statusStacks(StatusId::Shock);
    if (shock > 0) { dmg += shock; enemy_.removeStatus(StatusId::Shock); }
    if (enemy_.statusStacks(StatusId::Vulnerable) > 0 || enemy_.exposed) dmg = int(dmg * 1.5f);
    int corr = enemy_.statusStacks(StatusId::Corrode);
    if (corr > 0) dmg += corr;
    if (player_->has(RelicId::BloodPact)) dmg = int(dmg * 1.30f);

    // Щит врага
    if (enemy_.guard > 0) {
        int absorbed = std::min(enemy_.guard, dmg);
        enemy_.guard -= absorbed;
        dmg -= absorbed;
        if (absorbed > 0) pushFloat("-" + num(absorbed), Vector2{ 0, -40 }, hex(0x90E0EF), 18.0f);
    }
    if (dmg <= 0) return 0;

    // Выбранная точка
    int spot = target_;
    if (spot >= 0 && spot < int(enemy_.spots.size()) && !enemy_.spots[size_t(spot)].destroyed) {
        bool focused = player_->statusStacks(StatusId::Focus) > 0
                    || player_->has(RelicId::SpotHunter);
        if (!precise && !focused && rng_->chance(0.22f)) {
            audio::play(audio::Sfx::Miss, 0.7f);
            pushFloat(g_lang == Lang::Ru ? "мимо" : "miss", Vector2{ 0, -20 }, hex(0x8A8A94), 18.0f);
            pushLog(LS("Удар прошёл мимо точки.", "The strike misses the weak spot."), hex(0x8A8A94));
            return 0;
        }
        float share = precise ? 1.0f : 0.65f;
        int toSpot = int(dmg * share);
        int toBody = dmg - toSpot;
        WeakSpot& ws = enemy_.spots[size_t(spot)];
        ws.hp -= toSpot;
        ws.flash = 0.4f;
        pushFloat("-" + num(toSpot), Vector2{ 0, 0 }, col, hitSize(toSpot));
        enemy_.hp -= toBody;
        if (ws.hp <= 0) breakSpot(spot);
    } else {
        enemy_.hp -= dmg;
        pushFloat("-" + num(dmg), Vector2{ 0, 0 }, col, hitSize(dmg));
    }

    // Чем крупнее кусок здоровья снят, тем дольше стоит кадр и сильнее тряска.
    float frac = enemy_.maxHp > 0 ? float(dmg) / float(enemy_.maxHp) : 0.0f;
    if (frac > 0.10f) {
        hitstop_ = std::min(0.20f, 0.05f + frac * 0.30f);
        shake_ = std::max(shake_, 0.45f + frac * 0.9f);
    }

    audio::play(audio::Sfx::Hit, 0.55f, 0.9f + 0.2f * rng_->f());
    enemy_.shake = 0.5f;
    shake_ = std::max(shake_, 0.25f);
    if (enemy_.checkPhase()) {
        const EnemyPhase& ph = enemy_.phases[size_t(enemy_.phase)];
        pushLog(ph.enterLine.empty() ? LS("Оно меняет облик.", "It changes shape.") : LS(ph.enterLine),
                hex(0xF2B333));
        audio::play(audio::Sfx::PhaseShift, 0.9f);
        shake_ = 0.9f;
    }
    if (enemy_.hp <= 0) onVictory();
    return dmg;
}

void Combat::breakSpot(int index) {
    WeakSpot& ws = enemy_.spots[size_t(index)];
    ws.destroyed = true;
    ws.hp = 0;
    shake_ = 0.8f;
    audio::play(audio::Sfx::SpotBreak, 0.95f);
    pushLog(LS(std::string("Разрушено: ") + ws.name.ru + ".",
               std::string("Destroyed: ") + ws.name.en + "."), hex(0xF2B333));
    switch (ws.onBreak) {
        case SpotBreak::RemoveMove:
            pushLog(LS("Приём выпал из паттерна.", "A move drops from its pattern."), hex(0x59C772));
            break;
        case SpotBreak::Stagger:
            enemy_.addStatus(StatusId::Stun, 1, 1);
            pushLog(LS("Оно оглушено.", "It is staggered."), hex(0x59C772));
            break;
        case SpotBreak::Expose:
            enemy_.exposed = true;
            pushLog(LS("Оно раскрыто: весь урон +50%.", "It is exposed: all damage +50%."), hex(0x59C772));
            break;
        case SpotBreak::ForcePhase:
            enemy_.hp = std::min(enemy_.hp, int(enemy_.maxHp * enemy_.hpFrac() * 0.9f));
            enemy_.phase = clampi(enemy_.phase + 1, 0, int(enemy_.phases.size()) - 1);
            enemy_.moveIndex = 0;
            pushLog(LS("Оно срывается в следующую фазу.", "It lurches into its next phase."), hex(0xF2B333));
            break;
    }
    if (target_ == index) target_ = -1;
}

void Combat::damagePlayer(int dmg, const LS& src) {
    if (dmg <= 0) return;
    int weak = player_->statusStacks(StatusId::Weaken);
    dmg = std::max(1, dmg - weak);
    if (player_->shield > 0) {
        int absorbed = std::min(player_->shield, dmg);
        player_->shield -= absorbed;
        dmg -= absorbed;
        if (absorbed > 0)
            pushLog(LS("Щит поглотил " + num(absorbed) + ".", "Shield absorbs " + num(absorbed) + "."),
                    hex(0x90E0EF));
    }
    if (dmg <= 0) return;
    audio::play(audio::Sfx::PlayerHurt, 0.8f);
    player_->hp -= dmg;
    shake_ = std::max(shake_, 0.55f);
    pushLog(LS(std::string(src.ru) + ": -" + num(dmg) + " HP",
               std::string(src.en) + ": -" + num(dmg) + " HP"), hex(0xFF6B6B));
    // Восковой двойник: раз за бой смертельный удар оставляет с 1 HP.
    if (player_->hp <= 0 && player_->has(RelicId::Undying) && !undyingUsed_) {
        undyingUsed_ = true;
        player_->hp = 1;
        audio::play(audio::Sfx::Heal, 1.0f, 0.7f);
        pushLog(LS("Восковой двойник трескается вместо вас. 1 HP.",
                   "The waxen double cracks in your place. 1 HP."), hex(0xD8CBB3));
    }
    if (player_->hp <= 0) {
        player_->hp = 0;
        state_ = CombatState::Lost;
        audio::play(audio::Sfx::Defeat, 1.0f);
        pushLog(LS("Свет гаснет. Забег окончен.", "The light goes out. The run ends."), hex(0x8C1A26));
    }
}

// ── Reactions ─────────────────────────────────────────────────────────────
void Combat::applyReaction(const ReactionDef& rc, const Card& card) {
    float amp = (1.0f + ampBonus_) * biomeMul(card);
    ampBonus_ = 0.0f;

    int dmg = int((card.power * rc.mult + rc.bonus) * amp);
    lastReaction_ = rc.name;
    reactionFlash_ = 1.0f;
    pushFx(rc.a, rc.b, rc.tint, 1.5f, true);
    // Реакция звучит поверх голоса стихии: чем сильнее, тем ниже удар.
    audio::play(audio::Sfx::Reaction, 1.0f, clampf(1.25f - rc.mult * 0.28f, 0.6f, 1.3f));
    pushLog(LS(std::string("Реакция: ") + rc.name.ru + " — " + rc.desc.ru,
               std::string("Reaction: ") + rc.name.en + " — " + rc.desc.en), rc.tint);

    // Эхо в пустой зале: реакция срабатывает дважды, по 60% каждая.
    if (player_->has(RelicId::ReactionEcho)) {
        int half = std::max(1, int(dmg * 0.6f));
        dealDamageToEnemy(half, has(card.flags, CardFlags::Precise), rc.tint);
        if (enemy_.alive()) dealDamageToEnemy(half, has(card.flags, CardFlags::Precise), rc.tint);
        pushLog(LS("Эхо повторяет реакцию.", "The echo repeats the reaction."), hex(0x9D4EDD));
    } else {
        dealDamageToEnemy(dmg, has(card.flags, CardFlags::Precise), rc.tint);
    }

    if (rc.applies != StatusId::None) {
        if (statusDef(rc.applies).onPlayer) player_->addStatus(rc.applies, rc.statusStacks, rc.statusTurns);
        else                                enemy_.addStatus(rc.applies, rc.statusStacks, rc.statusTurns);
    }
    if (rc.kind == ReactionKind::Drain) {
        int heal = std::max(1, dmg / 3);
        player_->hp = std::min(player_->maxHp, player_->hp + heal);
        pushLog(LS("Вы вытянули " + num(heal) + " HP.", "You drain " + num(heal) + " HP."), hex(0x95D5B2));
    }
    if (rc.kind == ReactionKind::Risky && rng_->chance(rc.risk)) {
        int back = std::max(2, dmg / 4);
        pushLog(LS("Отдача!", "Backlash!"), hex(0xFF5A40));
        damagePlayer(back, LS("Отдача", "Backlash"));
    }
    mark_ = ElementId::None;
    markTurns_ = 0;
    shake_ = std::max(shake_, 0.5f);
}

// ── Card resolution / Разрешение карт ─────────────────────────────────────
// Составная карта не вешает метку — она разряжается вся сразу.
void Combat::resolveCompound(const Card& c) {
    float amp = (1.0f + ampBonus_) * biomeMul(c); ampBonus_ = 0.0f;
    const int rank = tierRank(c.tier);
    const float mult = tierMult(c.tier);
    int dmg = int(c.power * mult * amp);

    lastReaction_ = c.name;
    reactionFlash_ = 1.0f;
    shake_ = std::max(shake_, 0.6f + rank * 0.12f);

    Color tint = c.chain.size() >= 2
        ? lerpC(elem(c.chain[0]).primary, elem(c.chain[1]).primary, 0.5f)
        : c.tint();

    pushFx(c.chain.empty() ? c.element : c.chain[0],
           c.chain.size() > 1 ? c.chain[1] : ElementId::None,
           tint, 1.2f + rank * 0.45f, true);
    audio::play(audio::Sfx::Reaction, 1.0f, clampf(1.25f - rank * 0.14f, 0.55f, 1.25f));
    if (!c.chain.empty()) audio::playElement(c.chain.front(), 0.75f);

    pushLog(LS(std::string("[") + tierLabel(c.tier) + "] " + c.name.ru + " — урон x"
                   + num(int(mult)) + (mult - int(mult) > 0.01f ? ".5" : ""),
               std::string("[") + tierLabel(c.tier) + "] " + c.name.en + " — x"
                   + num(int(mult)) + (mult - int(mult) > 0.01f ? ".5" : "") + " damage"),
            tint);

    if (c.tier == MergeTier::Ultima) {
        ultimaUsed_ = true;
        enemy_.guard = 0;                                  // игнорирует щит
        dmg += int(enemy_.hp * 0.20f);                     // и откусывает долю HP
        enemy_.addStatus(StatusId::Stun, 1, 1);
        pushLog(LS("Ultima разряжается. Такое бывает раз за бой.",
                   "The Ultima discharges. Once per combat."), hex(0xFF4D73));
    } else if (c.tier == MergeTier::T3plus) {
        enemy_.addStatus(StatusId::Stun, 1, 1);
    } else if (c.tier == MergeTier::T2plus) {
        enemy_.guard = 0;                                  // пробивает щит
    }

    // С третьего уровня удар накрывает и корпус, и все уязвимые точки разом.
    int saved = target_;
    if (rank >= 3) {
        target_ = -1;
        dealDamageToEnemy(dmg, true, tint);
        for (int i = 0; i < int(enemy_.spots.size()) && enemy_.alive(); ++i) {
            if (enemy_.spots[size_t(i)].destroyed) continue;
            target_ = i;
            dealDamageToEnemy(dmg / 2, true, tint);
        }
    } else {
        dealDamageToEnemy(dmg, has(c.flags, CardFlags::Precise), tint);
    }
    target_ = saved;

    // Статусы всей цепочки — то самое накопление из таблицы уровней.
    int stacks = 1 + rank;
    int turns = 2 + rank;
    for (ElementId e : c.chain) enemy_.addStatus(elem(e).mark, stacks, turns);

    mark_ = ElementId::None;
    markTurns_ = 0;
}

void Combat::resolveElement(const Card& c) {
    if (c.tier != MergeTier::Base) { resolveCompound(c); return; }
    ElementId e = c.element;
    // Дикая карта подстраивается: берёт противоположность текущей метки,
    // гарантируя реакцию.
    if (has(c.flags, CardFlags::Wild) && mark_ != ElementId::None) {
        ElementId opp = elem(mark_).opposed;
        if (opp != ElementId::None && opp != mark_) e = opp;
    }

    if (mark_ != ElementId::None && mark_ != e) {
        ReactionDef rc = reactionFor(mark_, e);
        if (rc.valid) { applyReaction(rc, c); return; }
    }

    // Ставим метку
    float amp = (1.0f + ampBonus_) * biomeMul(c); ampBonus_ = 0.0f;
    mark_ = e;
    markTurns_ = 3;
    dealDamageToEnemy(int(c.power * amp), has(c.flags, CardFlags::Precise), elem(e).primary);
    pushFx(e, ElementId::None, elem(e).primary, 1.0f, false);
    enemy_.addStatus(elem(e).mark, 1 + depth_ / 4, 2);
    pushLog(LS(std::string("Метка: ") + elem(e).name.ru, std::string("Mark: ") + elem(e).name.en),
            elem(e).primary);
}

void Combat::resolveSpell(const Card& c) {
    float amp = (1.0f + ampBonus_) * biomeMul(c);
    if (c.element != ElementId::None)
        pushFx(c.element, c.chain.size() > 1 ? c.chain[1] : ElementId::None,
               elem(c.element).primary, 1.0f, false);
    bool precise = has(c.flags, CardFlags::Precise);
    Color col = c.tint();

    switch (c.spell) {
        case SpellEffect::Damage:
            ampBonus_ = 0.0f;
            dealDamageToEnemy(int(c.power * amp), precise, col);
            break;
        case SpellEffect::Pierce:
            ampBonus_ = 0.0f;
            if (target_ < 0 && !enemy_.spots.empty()) {
                // авто-наведение на первую целую точку
                for (int i = 0; i < int(enemy_.spots.size()); ++i)
                    if (!enemy_.spots[size_t(i)].destroyed) { target_ = i; break; }
            }
            dealDamageToEnemy(int(c.power * amp), true, hex(0xF0DC8C));
            break;
        case SpellEffect::Volley: {
            ampBonus_ = 0.0f;
            for (int i = 0; i < c.amount; ++i)
                dealDamageToEnemy(int(c.power * amp), precise, col);
            break;
        }
        case SpellEffect::Drain: {
            ampBonus_ = 0.0f;
            dealDamageToEnemy(int(c.power * amp), precise, hex(0x9B2226));
            int heal = c.amount;
            player_->hp = std::min(player_->maxHp, player_->hp + heal);
            pushLog(LS("Вытянуто " + num(heal) + " HP.", "Drained " + num(heal) + " HP."), hex(0x95D5B2));
            break;
        }
        case SpellEffect::Heal:
            audio::play(audio::Sfx::Heal, 0.85f);
            player_->hp = std::min(player_->maxHp, player_->hp + c.power);
            pushLog(LS("Восстановлено " + num(c.power) + " HP.", "Restored " + num(c.power) + " HP."),
                    hex(0x95D5B2));
            break;
        case SpellEffect::Amplify:
            ampBonus_ += c.amount * 0.1f;
            pushLog(LS("Следующая реакция усилена.", "Next reaction amplified."), hex(0xF0DC8C));
            break;
        case SpellEffect::Detonate: {
            ampBonus_ = 0.0f;
            if (mark_ != ElementId::None) {
                Color mc = elem(mark_).primary;
                pushLog(LS(std::string("Подрыв метки: ") + elem(mark_).name.ru,
                           std::string("Mark detonated: ") + elem(mark_).name.en), mc);
                dealDamageToEnemy(int(c.power * 1.6f * amp), precise, mc);
                enemy_.addStatus(elem(mark_).mark, 2, 2);
                mark_ = ElementId::None;
                markTurns_ = 0;
            } else {
                dealDamageToEnemy(int(c.power * amp), precise, col);
            }
            break;
        }
        default: break;
    }

    if (has(c.flags, CardFlags::Corrupted) && rng_->chance(0.2f)) {
        int back = std::max(2, c.power / 5);
        pushLog(LS("Порча карты отзывается болью.", "The card's corruption bites back."), hex(0xFF5A40));
        damagePlayer(back, LS("Порча", "Corruption"));
    }
}

void Combat::resolveEffect(const Card& c) {
    switch (c.effect) {
        case EffectKind::Shield:
            audio::play(audio::Sfx::Shield, 0.85f);
            player_->shield += c.power;
            pushLog(LS("Щит +" + num(c.power) + ".", "Shield +" + num(c.power) + "."), hex(0x90E0EF));
            break;
        case EffectKind::Draw:
            drawHand(c.amount);
            pushLog(LS("Взято карт: " + num(c.amount) + ".", "Drew " + num(c.amount) + " card(s)."),
                    hex(0xE0D4B4));
            break;
        case EffectKind::LimitUp:
            cardLimit_ += c.amount;
            cardsLeft_ += c.amount;
            pushLog(LS("Лимит карт +" + num(c.amount) + ".", "Card limit +" + num(c.amount) + "."),
                    hex(0xF0DC8C));
            break;
        case EffectKind::Stun:
            enemy_.addStatus(StatusId::Stun, 1, c.amount);
            pushLog(LS("Противник оглушён.", "The enemy is stunned."), hex(0xFFD60A));
            break;
        case EffectKind::Focus:
            player_->addStatus(StatusId::Focus, c.amount, 2);
            pushLog(LS("Сосредоточение.", "Focused."), hex(0xFFF3B0));
            break;
        case EffectKind::Cleanse:
            player_->removeNegative();
            pushLog(LS("Негативные статусы сняты.", "Negative statuses cleared."), hex(0x95D5B2));
            break;
        case EffectKind::Weaken:
            enemy_.addStatus(StatusId::Weaken, c.power, c.amount);
            pushLog(LS("Противник ослаблен.", "The enemy is weakened."), hex(0x6C757D));
            break;
        case EffectKind::Regen:
            audio::play(audio::Sfx::Heal, 0.7f, 0.85f);
            player_->addStatus(StatusId::Regen, c.power, c.amount);
            pushLog(LS("Регенерация.", "Regenerating."), hex(0x95D5B2));
            break;
        default: break;
    }
}

bool Combat::canPlay(int handIndex) const {
    if (state_ != CombatState::PlayerTurn) return false;
    if (handIndex < 0 || handIndex >= int(hand_.size())) return false;
    const Card& c = hand_[size_t(handIndex)];
    if (c.tier == MergeTier::Ultima && ultimaUsed_) return false;   // раз за бой
    return c.cost <= cardsLeft_;
}

void Combat::playCard(int handIndex) {
    if (!canPlay(handIndex)) return;
    Card c = hand_[size_t(handIndex)];
    hand_.erase(hand_.begin() + handIndex);

    // Право первой руки: первая карта за ход не тратит слот.
    bool freeCard = firstCardTurn_ && player_->has(RelicId::FirstFree);
    firstCardTurn_ = false;
    if (!freeCard) cardsLeft_ -= c.cost;

    // Кровный вексель: каждая карта оплачивается кровью.
    if (player_->has(RelicId::BloodPact)) {
        player_->hp = std::max(1, player_->hp - 2);
        pushFloat("-2", Vector2{ -160, 90 }, hex(0x9B2226), 18.0f);
    }

    // Сначала шорох бумаги — карту вытаскивают из руки, — потом голос карты.
    audio::play(audio::Sfx::CardDraw, 0.55f, 0.92f + 0.16f * rng_->f());
    switch (c.type) {
        case CardType::Element:
            audio::playElement(c.element, 0.95f);
            break;
        case CardType::Spell:
            audio::play(audio::Sfx::CardSpell, 0.85f);
            if (c.element != ElementId::None) audio::playElement(c.element, 0.55f);
            break;
        case CardType::Effect:
            audio::play(audio::Sfx::CardEffect, 0.85f);
            break;
    }

    switch (c.type) {
        case CardType::Element: resolveElement(c); break;
        case CardType::Spell:   resolveSpell(c);   break;
        case CardType::Effect:  resolveEffect(c);  break;
    }
    deck_->discard(c);

    if (state_ == CombatState::PlayerTurn && cardsLeft_ <= 0) {
        // лимит исчерпан — ход автоматически завершается
        endTurn();
    }
}

bool Combat::mergeInHand(int from, int onto) {
    int n = int(hand_.size());
    if (from < 0 || from >= n || onto < 0 || onto >= n || from == onto) return false;

    MergeInfo mi = evaluateMerge(hand_[size_t(onto)], hand_[size_t(from)]);
    if (!mi.ok) return false;

    Card merged = applyMerge(hand_[size_t(onto)], hand_[size_t(from)], mi);
    hand_[size_t(onto)] = merged;
    hand_.erase(hand_.begin() + from);

    // Слияние — событие: слышно и видно.
    audio::play(audio::Sfx::Reaction, 0.55f,
                clampf(1.35f - tierRank(mi.tier) * 0.12f, 0.7f, 1.4f));
    if (!merged.chain.empty()) audio::playElement(merged.chain.back(), 0.6f);
    if (mi.tier == MergeTier::Ultima) audio::play(audio::Sfx::CraftPerfect, 1.0f);

    pushLog(LS(std::string("Слияние [") + tierLabel(mi.tier) + "]: " + merged.name.ru,
               std::string("Merge [") + tierLabel(mi.tier) + "]: " + merged.name.en),
            mi.tier == MergeTier::Ultima ? hex(0xFF4D73) : hex(0xF0DC8C));
    return true;
}

void Combat::moveCardInHand(int from, int to) {
    int n = int(hand_.size());
    if (from < 0 || from >= n) return;
    to = clampi(to, 0, n - 1);
    if (from == to) return;
    Card c = hand_[size_t(from)];
    hand_.erase(hand_.begin() + from);
    hand_.insert(hand_.begin() + to, c);
}

void Combat::endTurn() {
    if (state_ != CombatState::PlayerTurn) return;
    state_ = CombatState::Resolving;
    timer_ = 0.55f;
}

void Combat::flee() {
    if (state_ == CombatState::Won || state_ == CombatState::Lost) return;
    state_ = CombatState::Fled;
    discardHand();
    pushLog(LS("Вы складываете стол и уходите.", "You fold the table and walk away."), hex(0xC9B78E));
}

// ── Enemy turn ────────────────────────────────────────────────────────────
void Combat::runEnemyTurn() {
    if (!enemy_.alive()) { onVictory(); return; }

    if (enemy_.statusStacks(StatusId::Freeze) > 0) {
        pushLog(LS("Оно скованно льдом и пропускает ход.", "It is frozen and loses its turn."),
                hex(0x48CAE4));
        enemy_.removeStatus(StatusId::Freeze);
    } else if (enemy_.statusStacks(StatusId::Stun) > 0) {
        pushLog(LS("Оно оглушено и пропускает ход.", "It is stunned and loses its turn."), hex(0xFFD60A));
        enemy_.removeStatus(StatusId::Stun);
    } else if (enemy_.statusStacks(StatusId::Slow) > 0) {
        pushLog(LS("Оно замедлено — шаг паттерна пропущен.", "Slowed — a pattern step is skipped."),
                hex(0x6B705C));
        enemy_.removeStatus(StatusId::Slow);
        enemy_.advanceMove();
    } else for (int act = 0; act < std::max(1, enemy_.actions) && player_->alive() && enemy_.alive(); ++act) {
        const EnemyMove* m = enemy_.currentMove();
        if (!m) break;
        playedMove_ = *m;
        movePlay_ = 1.0f;
        audio::playEnemyMove(int(m->kind), enemy_.flavour, 0.9f);
        int dmg = m->damage + enemy_.enrage;
        switch (m->kind) {
            case MoveKind::Guard:
                enemy_.guard += 6 + depth_ * 2;
                pushLog(LS(std::string(m->name.ru) + ": щит.", std::string(m->name.en) + ": guard."),
                        hex(0x90E0EF));
                break;
            case MoveKind::Mend: {
                int heal = 6 + depth_ * 3;
                enemy_.hp = std::min(enemy_.maxHp, enemy_.hp + heal);
                pushLog(LS(std::string(m->name.ru) + ": +" + num(heal) + " HP",
                           std::string(m->name.en) + ": +" + num(heal) + " HP"), hex(0x95D5B2));
                break;
            }
            case MoveKind::Enrage:
                enemy_.enrage += 2 + depth_ / 2;
                pushLog(LS(std::string(m->name.ru) + ": удары усилены.",
                           std::string(m->name.en) + ": its blows grow."), hex(0xFF5A40));
                break;
            case MoveKind::Sunder:
                if (player_->shield > 0) {
                    pushLog(LS("Щит расколот.", "Your shield is sundered."), hex(0xFF5A40));
                    player_->shield = 0;
                }
                damagePlayer(dmg, m->name);
                break;
            case MoveKind::Multi:
                for (int i = 0; i < m->hits && player_->alive(); ++i) damagePlayer(dmg, m->name);
                break;
            default:
                damagePlayer(dmg, m->name);
                break;
        }
        if (m->applies != StatusId::None && player_->alive())
            player_->addStatus(m->applies, m->stacks, m->turns);
        enemy_.advanceMove();
    }

    tickStatusesEndOfRound();
}

void Combat::tickStatusesEndOfRound() {
    // Враг: DoT
    int burn = enemy_.statusStacks(StatusId::Burn);
    int bleed = enemy_.statusStacks(StatusId::Bleed);
    int pois = enemy_.statusStacks(StatusId::Poisoned);
    if (burn > 0)  { enemy_.hp -= burn;  pushFloat("-" + num(burn),  Vector2{ 30, -30 }, hex(0xE85D04), 18.0f); }
    if (bleed > 0) { enemy_.hp -= bleed; pushFloat("-" + num(bleed), Vector2{ -30, -30 }, hex(0x9B2226), 18.0f); }
    if (pois > 0)  { enemy_.hp -= pois;  pushFloat("-" + num(pois),  Vector2{ 0, -55 }, hex(0x52B788), 18.0f); }

    for (auto& s : enemy_.statuses) if (s.turns > 0) s.turns--;
    enemy_.statuses.erase(std::remove_if(enemy_.statuses.begin(), enemy_.statuses.end(),
        [](const StatusInst& s) { return s.turns == 0; }), enemy_.statuses.end());

    // Игрок
    int pburn = player_->statusStacks(StatusId::Burn);
    int pbleed = player_->statusStacks(StatusId::Bleed);
    int ppois = player_->statusStacks(StatusId::Poisoned);
    if (pburn > 0)  damagePlayer(pburn,  LS("Ожог", "Burn"));
    if (pbleed > 0) damagePlayer(pbleed, LS("Кровотечение", "Bleed"));
    if (ppois > 0)  damagePlayer(ppois,  LS("Яд", "Poison"));
    int regen = player_->statusStacks(StatusId::Regen);
    if (regen > 0 && player_->alive()) {
        player_->hp = std::min(player_->maxHp, player_->hp + regen);
        pushLog(LS("Регенерация: +" + num(regen) + " HP", "Regen: +" + num(regen) + " HP"), hex(0x95D5B2));
    }
    for (auto& s : player_->statuses) if (s.turns > 0) s.turns--;
    player_->statuses.erase(std::remove_if(player_->statuses.begin(), player_->statuses.end(),
        [](const StatusInst& s) { return s.turns == 0; }), player_->statuses.end());

    if (!player_->has(RelicId::LingeringMark) && markTurns_ > 0 && --markTurns_ == 0)
        mark_ = ElementId::None;

    if (enemy_.hp <= 0) { onVictory(); return; }
    if (!player_->alive()) { state_ = CombatState::Lost; return; }

    turn_++;
    startPlayerTurn();
}

// ── Victory ───────────────────────────────────────────────────────────────
void Combat::onVictory() {
    if (state_ == CombatState::Won) return;
    state_ = CombatState::Won;
    audio::play(audio::Sfx::Victory, 0.9f);
    enemy_.hp = 0;

    // Алембик: лучшая составная карта из руки уходит в колоду насовсем.
    if (player_->has(RelicId::Alembic)) {
        int best = -1, bestRank = 0;
        for (int i = 0; i < int(hand_.size()); ++i) {
            int rk = tierRank(hand_[size_t(i)].tier);
            if (rk > bestRank) { bestRank = rk; best = i; }
        }
        if (best >= 0) {
            deck_->addCard(hand_[size_t(best)]);
            pushLog(LS(std::string("Алембик сохраняет: ") + hand_[size_t(best)].name.ru,
                       std::string("The alembic keeps: ") + hand_[size_t(best)].name.en),
                    hex(0xB5179E));
            hand_.erase(hand_.begin() + best);
        }
    }
    // Скупая ладонь: одна случайная карта из руки остаётся в колоде.
    if (player_->has(RelicId::Hoarder) && !hand_.empty()) {
        int i = rng_->i(0, int(hand_.size()) - 1);
        deck_->addCard(hand_[size_t(i)]);
        pushLog(LS(std::string("Скупая ладонь удерживает: ") + hand_[size_t(i)].name.ru,
                   std::string("The miser's palm keeps: ") + hand_[size_t(i)].name.en),
                hex(0xC2A24A));
        hand_.erase(hand_.begin() + i);
    }

    discardHand();

    // Материалы: тема врага + бонус за разрушенные точки и боссов.
    int rolls = 2 + depth_ / 3 + (enemy_.boss ? 3 : 0);
    int spotBonus = player_->has(RelicId::Gravedigger) ? 2 : 1;   // Лопата могильщика
    for (const auto& s : enemy_.spots) if (s.destroyed) rolls += spotBonus;
    for (int i = 0; i < rolls; ++i) {
        int m = rng_->i(0, MATERIAL_COUNT - 1);
        if (rng_->chance(0.45f)) m = eidx(enemy_.flavour) % MATERIAL_COUNT;
        rewards_.materials[m] += rng_->i(1, 2 + depth_ / 4);
    }
    int choices = enemy_.boss ? 3 : 2;
    for (int i = 0; i < choices; ++i) rewards_.cardChoices.push_back(genCard(*rng_, depth_ + (enemy_.boss ? 2 : 0)));

    pushLog(LS("Противник рассыпается. Забирайте трофеи.",
               "The enemy comes apart. Take your spoils."), hex(0xF0DC8C));
}

// ── Update ────────────────────────────────────────────────────────────────
void Combat::update(float dt) {
    // Пока держится хитстоп, не двигается ничего: в этом весь смысл паузы.
    if (hitstop_ > 0.0f) { hitstop_ = std::max(0.0f, hitstop_ - dt); return; }
    shake_ = std::max(0.0f, shake_ - dt * 2.4f);
    reactionFlash_ = std::max(0.0f, reactionFlash_ - dt * 1.6f);
    movePlay_ = std::max(0.0f, movePlay_ - dt * 0.75f);
    enemy_.shake = std::max(0.0f, enemy_.shake - dt * 2.0f);
    for (auto& s : enemy_.spots) s.flash = std::max(0.0f, s.flash - dt * 2.5f);

    for (auto& f : floats_) { f.life -= dt; f.pos.y -= dt * 34.0f; }
    floats_.erase(std::remove_if(floats_.begin(), floats_.end(),
        [](const FloatText& f) { return f.life <= 0.0f; }), floats_.end());
    for (auto& l : log_) l.age += dt;

    if (state_ == CombatState::Resolving) {
        timer_ -= dt;
        if (timer_ <= 0.0f) {
            state_ = CombatState::EnemyTurn;
            runEnemyTurn();
            if (state_ == CombatState::EnemyTurn) state_ = CombatState::PlayerTurn;
        }
    }
}

} // namespace shichoy
