// Shichoy / Шичой — Windows, C++20, raylib. Точка входа.
#include "app/Game.h"
#include "art/ProcArt.h"
#include "audio/ProcAudio.h"
#include "craft/Craft.h"
#include <cstring>
#include <cstdio>

// ── Selftest / Прогон боевой математики без окна ──────────────────────────
// Гоняет бои на разных глубинах со случайной игрой, ловит зависания и
// показывает, во что складывается баланс.
static int runSelftest() {
    using namespace shichoy;
    Rng rng(0xBEEF1234);
    int wins = 0, losses = 0, stalls = 0, reactions = 0, spotsBroken = 0, maxHand = 0;

    for (int depth = 1; depth <= 15; ++depth) {
        for (int trial = 0; trial < 12; ++trial) {
            Deck deck;
            deck.reset(starterDeck(rng));
            // Немного процедурных карт, как после нескольких боёв
            for (int i = 0; i < depth; ++i) deck.addCard(genCard(rng, depth));

            Player p;
            p.maxHp = 60 + depth * 2;
            p.hp = p.maxHp;

            Combat c;
            c.begin(&deck, &p, genEnemy(rng, depth, trial % 4 == 0), depth, &rng);

            int guard = 0;
            while (c.state() != CombatState::Won && c.state() != CombatState::Lost && guard++ < 4000) {
                if (c.state() == CombatState::PlayerTurn) {
                    if (rng.chance(0.25f) && !c.enemy().spots.empty()) c.cycleTarget(1);
                    int n = int(c.hand().size());
                    int idx = n > 0 ? rng.i(0, n - 1) : -1;
                    if (idx >= 0 && c.canPlay(idx)) c.playCard(idx);
                    else c.endTurn();
                } else {
                    c.update(0.6f);
                }
                maxHand = std::max(maxHand, int(c.hand().size()));
            }
            if (guard >= 4000) { stalls++; continue; }
            if (c.state() == CombatState::Won) wins++; else losses++;
            for (const auto& s : c.enemy().spots) if (s.destroyed) spotsBroken++;
            for (const auto& l : c.log()) if (l.text.ru.rfind("Реакция", 0) == 0) reactions++;
        }
    }

    // Проверка крафта: потолок шанса и применение результата
    Deck deck; deck.reset(starterDeck(rng));
    Inventory inv;
    for (int i = 0; i < MATERIAL_COUNT; ++i) inv.add(i, 40);
    float maxChance = 0.0f;
    int crafted = 0;
    for (int i = 0; i < 200; ++i) {
        CraftAction a = CraftAction(i % 4);
        CraftPlan pl = makePlan(a, 1 + i % 20, deck, i % std::max<int>(1, int(deck.library().size())), rng);
        maxChance = std::max(maxChance, pl.chance);
        TimingMiniGame mg;
        mg.start(pl.chance, rng);
        for (int f = 0; f < 400 && mg.running(); ++f) { mg.update(0.016f); if (f == 200) mg.commit(); }
        if (!mg.finished()) mg.commit();
        applyCraft(pl, mg.success(), mg.perfect(), deck, inv, 1 + i % 20, rng);
        crafted++;
    }

    // Проверка реакций: все 120 пар обязаны что-то возвращать
    int pairs = 0;
    for (int a = 1; a <= ELEMENT_COUNT; ++a)
        for (int b = a + 1; b <= ELEMENT_COUNT; ++b)
            if (reactionFor(ElementId(a), ElementId(b)).valid) pairs++;

    std::printf("--- selftest ---\n");
    std::printf("combats      : %d wins / %d losses / %d stalls\n", wins, losses, stalls);
    std::printf("reactions    : %d fired\n", reactions);
    std::printf("weak spots   : %d destroyed\n", spotsBroken);
    std::printf("max hand     : %d cards (no cap)\n", maxHand);
    std::printf("craft ops    : %d, max chance %.0f%% (cap 70%%)\n", crafted, maxChance * 100.0f);
    std::printf("element pairs: %d / 120 resolve\n", pairs);
    std::printf("deck size    : %d cards after crafting\n", int(deck.library().size()));

    bool ok = (stalls == 0) && (pairs == 120) && (maxChance <= 0.7001f) && (wins > 0) && (losses > 0);
    std::printf("result       : %s\n", ok ? "OK" : "FAILED");
    return ok ? 0 : 1;
}

int main(int argc, char** argv) {
    bool shotMode = false, selftest = false, audiodump = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--shot") == 0) shotMode = true;
        if (std::strcmp(argv[i], "--selftest") == 0) selftest = true;
        if (std::strcmp(argv[i], "--audiodump") == 0) audiodump = true;
    }
    if (selftest) return runSelftest();
    if (audiodump) { shichoy::audio::debugDump("."); return 0; }

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Shichoy / Шичой");
    SetWindowMinSize(1024, 640);
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);          // ESC используется внутри игры

    shichoy::Game game;
    game.init();

    if (shotMode) {
        // Служебный режим: пройтись по сценам и сохранить кадры.
        const char* names[] = { "shot_menu.png", "shot_overworld.png", "shot_combat.png",
                                "shot_craft.png", "shot_codex.png", "shot_reward.png",
                                "shot_tablepull.png", "shot_bighand.png" };
        for (int s = 0; s < 8; ++s) {
            game.debugForceScene(s);
            for (int f = 0; f < 25; ++f) game.frame();
            TakeScreenshot(names[s]);
            game.frame();
        }
        game.shutdown();
        CloseWindow();
        return 0;
    }

    while (!WindowShouldClose() && !game.wantsExit())
        game.frame();

    game.shutdown();
    CloseWindow();
    return 0;
}
