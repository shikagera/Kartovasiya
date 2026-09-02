// Combat & reward UI / Интерфейс боя и наград — вид от первого лица.
#include "app/Game.h"
#include "art/ProcArt.h"
#include "audio/ProcAudio.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace shichoy {

// Геометрия боевой сцены
static Vector2 enemyCenter(int sw, int sh) { return Vector2{ sw * 0.5f, sh * 0.34f }; }
static float   enemyScale(int sh) { return sh / 720.0f * 1.30f; }

// ── Update ────────────────────────────────────────────────────────────────
void Game::updateCombat(float dt) {
    combat_.update(dt);

    const Enemy& e = combat_.enemy();
    Vector2 ec = enemyCenter(sw_, sh_);
    float es = 100.0f * enemyScale(sh_);

    // Наведение мышью на уязвимые точки
    hoverSpot_ = -1;
    Vector2 m = GetMousePosition();
    for (int i = 0; i < int(e.spots.size()); ++i) {
        if (e.spots[size_t(i)].destroyed) continue;
        Vector2 p = spotScreenPos(e, i, ec, es);
        if (CheckCollisionPointCircle(m, p, 24.0f)) hoverSpot_ = i;
    }

    if (combat_.state() == CombatState::PlayerTurn) {
        if (IsKeyPressed(KEY_TAB)) combat_.cycleTarget(1);
        if (hoverSpot_ >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) combat_.setTarget(hoverSpot_);
        if (IsKeyPressed(KEY_Q)) combat_.setTarget(-1);

        for (int i = 0; i < 9; ++i)
            if (IsKeyPressed(KEY_ONE + i)) combat_.playCard(i);

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) combat_.endTurn();
        if (IsKeyPressed(KEY_F)) combat_.flee();
    }

    switch (combat_.state()) {
        case CombatState::Won:
            rewardHover_ = -1;
            scene_ = Scene::Reward;
            break;
        case CombatState::Lost:
            endRun(true);
            break;
        case CombatState::Fled: {
            // Бегство: часть материалов теряется, враг отступает.
            int lost = inv_.total() / 4;
            for (int i = 0; i < MATERIAL_COUNT && lost > 0; ++i) {
                int take = std::min(inv_.mats[i], lost);
                inv_.mats[i] -= take; lost -= take;
            }
            if (pendingEnt_ >= 0 && pendingEnt_ < world_.entCount()) world_.removeEnt(pendingEnt_);
            pendingEnt_ = -1;
            world_.clearEncounter();
            toast(LS("Вы ушли, бросив часть добычи.", "You left, dropping part of your haul."));
            scene_ = Scene::Overworld;
            break;
        }
        default: break;
    }
}

// ── Hand / Рука ───────────────────────────────────────────────────────────
void Game::drawHandArea() {
    const auto& hand = combat_.hand();
    if (hand.empty()) return;

    // Рука живёт в полосе между HUD игрока (слева) и журналом/кнопками (справа).
    int n = int(hand.size());
    float cw = 104.0f, ch = 146.0f;
    float lane = std::max(320.0f, sw_ - 700.0f);
    float gap = (n > 1) ? std::min(12.0f, (lane - n * cw) / float(n - 1)) : 0.0f;
    if (gap < -44.0f) gap = -44.0f;
    float total = n * cw + (n - 1) * gap;
    float x0 = sw_ * 0.5f - total * 0.5f;
    float baseY = sh_ - ch - 12.0f;

    Vector2 m = GetMousePosition();
    hoverCard_ = -1;
    for (int i = int(hand.size()) - 1; i >= 0; --i) {
        Rectangle r{ x0 + i * (cw + gap), baseY, cw, ch };
        if (hoverCard_ < 0 && CheckCollisionPointRec(m, r)) hoverCard_ = i;
    }

    for (int i = 0; i < int(hand.size()); ++i) {
        bool hov = (hoverCard_ == i);
        bool playable = combat_.canPlay(i);
        float lift = hov ? 34.0f : 0.0f;
        Rectangle r{ x0 + i * (cw + gap), baseY - lift, cw, ch };
        if (!playable) {
            BeginBlendMode(BLEND_ALPHA);
            art::drawCard(hand[size_t(i)], r, 0.0f, false, time_);
            DrawRectangleRounded(r, 0.10f, 8, Color{ 5, 4, 8, 150 });
            EndBlendMode();
        } else {
            art::drawCard(hand[size_t(i)], r, hov ? 1.0f : 0.0f, false, time_);
        }
        char idx[8]; snprintf(idx, sizeof idx, "%d", i + 1);
        art::textC(idx, r.x + r.width * 0.5f, r.y - 18.0f, 15.0f,
                   playable ? art::pal::GoldLit : hex(0x5A5A62));

        if (hov && playable && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            combat_.playCard(i);
            return;
        }
    }
}

// ── Draw ──────────────────────────────────────────────────────────────────
void Game::drawCombat() {
    const Enemy& e = combat_.enemy();
    float shake = combat_.screenShake();
    float ox = std::sin(time_ * 47.0f) * shake * 9.0f;
    float oy = std::cos(time_ * 41.0f) * shake * 7.0f;

    // ── фон: тёмный зал ───────────────────────────────────────────────────
    ClearBackground(hex(0x07060B));
    DrawRectangleGradientV(0, 0, sw_, int(sh_ * 0.7f), hex(0x140F1E), hex(0x08070D));
    for (int i = 0; i < 6; ++i) {   // колонны
        float px = sw_ * (0.08f + i * 0.17f) + ox * 0.3f;
        float w = 46.0f + (i % 2) * 12.0f;
        DrawRectangleGradientV(int(px - w * 0.5f), 0, int(w), int(sh_ * 0.62f),
                               hex(0x191325), hex(0x0B0912));
        DrawRectangleLinesEx(Rectangle{ px - w * 0.5f, 0, w, sh_ * 0.62f }, 1.0f, withA(hex(0x2A2038), 120));
    }
    DrawRectangleGradientV(0, int(sh_ * 0.58f), sw_, int(sh_ * 0.42f), hex(0x0D0B14), hex(0x050408));
    art::drawStarfield(sw_, int(sh_ * 0.5f), time_ * 0.4f, e.seed);

    // Свечение от стихии врага
    Color fl = elem(e.flavour).primary;
    DrawCircleGradient(int(sw_ * 0.5f + ox), int(sh_ * 0.38f + oy), sh_ * 0.55f, withA(fl, 34), BLANK);

    // ── враг ──────────────────────────────────────────────────────────────
    Vector2 ec = enemyCenter(sw_, sh_);
    ec.x += ox; ec.y += oy;
    float es = 100.0f * enemyScale(sh_);
    if (e.hp > 0)
        art::drawEnemyBody(ec, enemyScale(sh_), e.seed, e.flavour, time_, e.shake * 6.0f, fl, e.phase);

    // Уязвимые точки
    for (int i = 0; i < int(e.spots.size()); ++i) {
        const WeakSpot& ws = e.spots[size_t(i)];
        Vector2 p = spotScreenPos(e, i, ec, es);
        if (ws.destroyed) {
            DrawCircleLines(int(p.x), int(p.y), 14.0f, withA(hex(0x4A4A54), 140));
            DrawLineEx(Vector2{ p.x - 9, p.y - 9 }, Vector2{ p.x + 9, p.y + 9 }, 2.0f, withA(hex(0x6A6A74), 170));
            DrawLineEx(Vector2{ p.x - 9, p.y + 9 }, Vector2{ p.x + 9, p.y - 9 }, 2.0f, withA(hex(0x6A6A74), 170));
            continue;
        }
        bool sel = (combat_.target() == i);
        bool hov = (hoverSpot_ == i);
        Color rc = sel ? art::pal::GoldLit : (hov ? art::pal::Gold : withA(art::pal::Bone, 190));
        float pulse = 1.0f + 0.08f * std::sin(time_ * 4.0f + i);
        DrawCircleV(p, 18.0f, withA(BLACK, 120));
        DrawPolyLinesEx(p, 6, 17.0f * pulse, time_ * 25.0f, sel ? 2.6f : 1.6f, rc);
        if (ws.flash > 0.01f)
            DrawCircleV(p, 16.0f, withA(art::pal::Risk, (unsigned char)(160 * ws.flash)));
        // маленькая шкала прочности
        Rectangle hb{ p.x - 20, p.y + 22, 40, 5 };
        DrawRectangle(int(hb.x), int(hb.y), int(hb.width), int(hb.height), hex(0x1A141C));
        DrawRectangle(int(hb.x), int(hb.y), int(hb.width * (float(ws.hp) / ws.maxHp)), int(hb.height),
                      lerpC(art::pal::Blood, art::pal::GoldLit, 0.4f));
        if (hov || sel)
            art::textC(ws.name.c_str(), p.x, p.y - 40.0f, 13.0f, rc);
    }

    // ── HUD врага ─────────────────────────────────────────────────────────
    Rectangle ehp{ sw_ * 0.5f - 300.0f, 34.0f, 600.0f, 22.0f };
    char nb[192];
    snprintf(nb, sizeof nb, "%s  —  %s", e.name.c_str(), e.epithet.c_str());
    art::textC(nb, sw_ * 0.5f, 8.0f, 19.0f, art::pal::GoldLit);
    drawBar(ehp, e.hpFrac(), art::pal::Blood, hex(0x1A121A),
            TextFormat("%d / %d", std::max(0, e.hp), e.maxHp));
    if (e.guard > 0) {
        char gb[32]; snprintf(gb, sizeof gb, g_lang == Lang::Ru ? "щит %d" : "guard %d", e.guard);
        art::text(gb, ehp.x + ehp.width + 10, ehp.y + 3, 14.0f, hex(0x90E0EF));
    }
    // фазы
    for (int i = 0; i < int(e.phases.size()); ++i) {
        Vector2 p{ ehp.x - 22.0f + i * 14.0f, ehp.y + 11.0f };
        DrawPolyLinesEx(p, 4, 5.0f, 45.0f, 1.4f,
                        i <= e.phase ? art::pal::GoldLit : withA(art::pal::Gold, 90));
    }
    drawStatusRow(e.statuses, ehp.x, ehp.y + 44.0f, 18.0f);

    // Метка стихии
    if (combat_.mark() != ElementId::None) {
        Vector2 mp{ sw_ * 0.5f + 340.0f, 92.0f };
        const ElementDef& me = elem(combat_.mark());
        art::drawSigil(me.id, mp, 22.0f, me.primary, time_);
        char mb[96];
        snprintf(mb, sizeof mb, g_lang == Lang::Ru ? "метка: %s (%d)" : "mark: %s (%d)",
                 me.name.c_str(), combat_.markTurns());
        art::textC(mb, mp.x, mp.y + 30.0f, 13.0f, lerpC(me.primary, WHITE, 0.4f));
    }

    // Вспышка реакции
    if (combat_.reactionFlash() > 0.01f) {
        unsigned char a = (unsigned char)(120 * combat_.reactionFlash());
        DrawRectangle(0, 0, sw_, sh_, withA(WHITE, (unsigned char)(a / 5)));
        art::textC(combat_.lastReaction().c_str(), sw_ * 0.5f, sh_ * 0.20f,
                   32.0f, withA(art::pal::GoldLit, (unsigned char)(255 * combat_.reactionFlash())));
    }

    // Телеграф следующего действия врага
    {
        Rectangle tb{ sw_ * 0.5f - 290.0f, sh_ * 0.56f, 580.0f, 34.0f };
        DrawRectangleRounded(tb, 0.4f, 6, Color{ 10, 8, 14, 200 });
        DrawRectangleRoundedLinesEx(tb, 0.4f, 6, 1.2f, withA(art::pal::Blood, 170));
        art::textC(combat_.telegraph().c_str(), tb.x + tb.width * 0.5f, tb.y + 9.0f, 15.0f,
                   withA(art::pal::Parchment, 235));
    }

    // ── стол на переднем плане ────────────────────────────────────────────
    float tableY = sh_ - 176.0f;
    DrawRectangleGradientV(0, int(tableY), sw_, 176, hex(0x162C1E), hex(0x080F0A));
    DrawLineEx(Vector2{ 0, tableY }, Vector2{ float(sw_), tableY }, 3.0f, hex(0x4A3420));
    DrawLineEx(Vector2{ 0, tableY + 6 }, Vector2{ float(sw_), tableY + 6 }, 1.0f, withA(art::pal::Gold, 90));

    // ── HUD игрока ────────────────────────────────────────────────────────
    Rectangle php{ 24.0f, sh_ - 170.0f, 290.0f, 22.0f };
    drawBar(php, float(player_.hp) / float(player_.maxHp), hex(0x8C1A26), hex(0x241820),
            TextFormat("HP %d / %d", player_.hp, player_.maxHp));
    if (player_.shield > 0) {
        char sb[48]; snprintf(sb, sizeof sb, g_lang == Lang::Ru ? "щит %d" : "shield %d", player_.shield);
        art::text(sb, php.x + php.width + 12, php.y + 3, 15.0f, hex(0x90E0EF));
    }
    drawStatusRow(player_.statuses, php.x, php.y - 30.0f, 17.0f);

    // Лимит карт
    char cl[96];
    snprintf(cl, sizeof cl, g_lang == Lang::Ru ? "Карт в ход: %d / %d" : "Cards this turn: %d / %d",
             combat_.cardsLeft(), combat_.cardLimit());
    art::text(cl, php.x, php.y + 30.0f, 15.0f, art::pal::Parchment);
    for (int i = 0; i < combat_.cardLimit(); ++i) {
        Vector2 p{ php.x + 8.0f + i * 15.0f, php.y + 56.0f };
        if (i < combat_.cardsLeft()) DrawCircleV(p, 5.0f, art::pal::GoldLit);
        else                          DrawCircleLines(int(p.x), int(p.y), 5.0f, withA(art::pal::Gold, 130));
    }

    // Цель
    {
        const char* tgt = combat_.target() < 0
            ? (g_lang == Lang::Ru ? "Цель: корпус (авто)" : "Target: body (auto)")
            : e.spots[size_t(combat_.target())].name.c_str();
        char tb[160];
        snprintf(tb, sizeof tb, g_lang == Lang::Ru ? "%s   [TAB] сменить  [Q] сброс"
                                                   : "%s   [TAB] switch  [Q] clear", tgt);
        art::text(tb, php.x, php.y + 74.0f, 14.0f,
                  combat_.target() < 0 ? withA(art::pal::Vellum, 200) : art::pal::GoldLit);
    }

    // ── журнал боя ────────────────────────────────────────────────────────
    Rectangle lg{ sw_ - 336.0f, sh_ * 0.58f, 312.0f, sh_ * 0.42f - 128.0f };
    DrawRectangleRounded(lg, 0.06f, 6, Color{ 8, 6, 12, 190 });
    DrawRectangleRoundedLinesEx(lg, 0.06f, 6, 1.0f, withA(art::pal::Gold, 110));
    const auto& log = combat_.log();
    int maxLines = int(lg.height / 17.0f);
    int start = std::max(0, int(log.size()) - maxLines);
    float ly = lg.y + 6.0f;
    for (int i = start; i < int(log.size()); ++i) {
        art::textWrapped(log[size_t(i)].text.c_str(),
                         Rectangle{ lg.x + 8, ly, lg.width - 16, 34 }, 12.0f, log[size_t(i)].color, 1.0f);
        ly += 17.0f;
        if (ly > lg.y + lg.height - 14) break;
    }

    // ── кнопки ────────────────────────────────────────────────────────────
    if (combat_.state() == CombatState::PlayerTurn) {
        Rectangle endB{ sw_ - 330.0f, sh_ - 62.0f, 150.0f, 40.0f };
        if (button(endB, g_lang == Lang::Ru ? "Конец хода [ENTER]" : "End turn [ENTER]", true, 14.0f))
            combat_.endTurn();
        Rectangle fleeB{ sw_ - 172.0f, sh_ - 62.0f, 150.0f, 40.0f };
        if (button(fleeB, g_lang == Lang::Ru ? "Сбежать [F]" : "Flee [F]", true, 14.0f))
            combat_.flee();
    } else {
        art::textC(g_lang == Lang::Ru ? "…ход противника…" : "...enemy's turn...",
                   sw_ - 176.0f, sh_ - 54.0f, 16.0f, withA(art::pal::Risk, 220));
    }

    // Рука
    drawHandArea();

    // Всплывающие числа
    for (const auto& f : combat_.floats()) {
        float a = clampf(f.life / f.maxLife, 0.0f, 1.0f);
        Vector2 p{ ec.x + f.pos.x, ec.y + f.pos.y };
        art::textC(f.s.c_str(), p.x, p.y, f.size, withA(f.color, (unsigned char)(255 * a)));
    }

    art::drawVignette(sw_, sh_, 1.0f);
}

// ── Rewards / Награды ─────────────────────────────────────────────────────
void Game::updateReward(float) {
    const CombatRewards& rw = combat_.rewards();
    if (!rewardTaken_) {
        for (int i = 0; i < int(rw.cardChoices.size()); ++i) {
            if (IsKeyPressed(KEY_ONE + i)) {
                deck_.addCard(rw.cardChoices[size_t(i)]);
                for (int m = 0; m < MATERIAL_COUNT; ++m) inv_.add(m, rw.materials[m]);
                rewardTaken_ = true;
                return;
            }
        }
        if (IsKeyPressed(KEY_ESCAPE)) {          // отказаться от карты
            for (int m = 0; m < MATERIAL_COUNT; ++m) inv_.add(m, rw.materials[m]);
            rewardTaken_ = true;
        }
    } else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        afterCombat();
    }
}

void Game::drawReward() {
    drawCombat();
    DrawRectangle(0, 0, sw_, sh_, Color{ 0, 0, 0, 190 });

    const CombatRewards& rw = combat_.rewards();
    art::textC(g_lang == Lang::Ru ? "ТРОФЕИ" : "SPOILS", sw_ * 0.5f, sh_ * 0.06f, 32.0f, art::pal::GoldLit);

    // Материалы
    Rectangle mb{ sw_ * 0.5f - 300.0f, sh_ * 0.14f, 600.0f, 58.0f };
    panel(mb, 0.95f);
    float cx = mb.x + 22.0f;
    bool any = false;
    for (int i = 0; i < MATERIAL_COUNT; ++i) {
        if (rw.materials[i] <= 0) continue;
        any = true;
        DrawCircleV(Vector2{ cx, mb.y + 29.0f }, 6.0f, material(i).tint);
        char b[64]; snprintf(b, sizeof b, "%s x%d", material(i).name.c_str(), rw.materials[i]);
        art::text(b, cx + 12.0f, mb.y + 21.0f, 14.0f, art::pal::Parchment);
        cx += art::textWidth(b, 14.0f) + 40.0f;
    }
    if (!any) art::textC(g_lang == Lang::Ru ? "Пусто." : "Nothing.", mb.x + mb.width * 0.5f,
                         mb.y + 21.0f, 15.0f, withA(art::pal::Vellum, 180));

    if (!rewardTaken_) {
        art::textC(g_lang == Lang::Ru ? "Возьмите одну карту (клик или 1/2/3, ESC — отказаться)"
                                      : "Take one card (click or 1/2/3, ESC to decline)",
                   sw_ * 0.5f, sh_ * 0.28f, 16.0f, withA(art::pal::Vellum, 225));
        int n = int(rw.cardChoices.size());
        float cw = 150.0f, ch = 212.0f, gap = 30.0f;
        float total = n * cw + (n - 1) * gap;
        float x0 = sw_ * 0.5f - total * 0.5f;
        Vector2 m = GetMousePosition();
        for (int i = 0; i < n; ++i) {
            Rectangle r{ x0 + i * (cw + gap), sh_ * 0.36f, cw, ch };
            bool hov = CheckCollisionPointRec(m, r);
            if (hov) r.y -= 12.0f;
            art::drawCard(rw.cardChoices[size_t(i)], r, hov ? 1.0f : 0.0f, hov, time_);
            char k[8]; snprintf(k, sizeof k, "%d", i + 1);
            art::textC(k, r.x + r.width * 0.5f, r.y - 24.0f, 17.0f, art::pal::GoldLit);
            if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                deck_.addCard(rw.cardChoices[size_t(i)]);
                for (int mm = 0; mm < MATERIAL_COUNT; ++mm) inv_.add(mm, rw.materials[mm]);
                rewardTaken_ = true;
            }
        }
    } else {
        art::textC(g_lang == Lang::Ru ? "Трофеи собраны." : "Spoils collected.",
                   sw_ * 0.5f, sh_ * 0.42f, 22.0f, art::pal::Parchment);
        Rectangle b{ sw_ * 0.5f - 120.0f, sh_ * 0.52f, 240.0f, 46.0f };
        if (button(b, g_lang == Lang::Ru ? "Дальше [ENTER]" : "Continue [ENTER]", true, 17.0f))
            afterCombat();
    }
    art::drawVignette(sw_, sh_, 0.9f);
}

} // namespace shichoy

// ── Dev helper / Служебный прыжок в сцену (для проверки кадра) ────────────
namespace shichoy {
void Game::debugForceScene(int which) {
    switch (which) {
        case 0: scene_ = Scene::Menu; break;
        case 1: startRun(); break;
        case 2: {
            startRun();
            pendingBoss_ = false;
            pendingEnt_ = -1;
            Enemy e = genEnemy(rng_, 4, false);
            combat_.begin(&deck_, &player_, std::move(e), 4, &rng_);
            rewardTaken_ = false;
            scene_ = Scene::Combat;
            break;
        }
        case 3: startRun(); openCraft(); break;
        case 4: codexReturn_ = Scene::Menu; codexPage_ = 0; scene_ = Scene::Codex; break;
        case 5: {
            startRun();
            Enemy e = genEnemy(rng_, 4, true);
            combat_.begin(&deck_, &player_, std::move(e), 4, &rng_);
            combat_.enemyMut().hp = 0;
            combat_.update(0.016f);
            for (int i = 0; i < 200 && combat_.state() != CombatState::Won; ++i) combat_.playCard(0);
            rewardTaken_ = false;
            scene_ = Scene::Reward;
            break;
        }
        case 6: {
            startRun();
            enterCombat(false, -1);
            pullT_ = 0.9f;
            break;
        }
        default: break;
    }
}
} // namespace shichoy
