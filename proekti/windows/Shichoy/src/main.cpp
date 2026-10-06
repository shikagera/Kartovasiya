// Shichoy / Шичой — Windows, C++20, raylib. Точка входа.
#include "app/Game.h"
#include "art/ProcArt.h"
#include "audio/ProcAudio.h"
#include "craft/Craft.h"
#include "core/Merge.h"
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

    // ── Лестница слияния: от чистой стихии до Ultima ──────────────────────
    std::printf("--- merge ladder ---\n");
    auto pure = [&](ElementId e) { Card c = genElementCard(rng, 3, e); c.power = 10; return c; };
    auto step = [&](const Card& a, const Card& b, const char* what) -> Card {
        MergeInfo mi = evaluateMerge(a, b);
        std::printf("%-22s -> %-5s x%-5.2f %-34s %s\n", what,
                    mi.ok ? tierLabel(mi.tier) : "нет", mi.ok ? double(mi.mult) : 0.0,
                    mi.ok ? mi.name.c_str() : mi.reason.c_str(),
                    mi.ok ? "" : "  <-- ОТКАЗ");
        return mi.ok ? applyMerge(a, b, mi) : a;
    };
    Card a1 = step(pure(ElementId::Fire), pure(ElementId::Water), "Fire + Water");
    Card a2 = step(a1, pure(ElementId::Earth), "  I + Earth");
    Card a3 = step(a2, pure(ElementId::Air), "  II + Air");
    Card b1 = step(pure(ElementId::Lightning), pure(ElementId::Ice), "Lightning + Ice");
    Card b2 = step(b1, pure(ElementId::Nature), "  I + Nature");
    Card b3 = step(b2, pure(ElementId::Shadow), "  II + Shadow");
    Card ult = step(a3, b3, "  III + III");
    Card cross = step(a1, b1, "  I + I");
    Card conv = step(a2, b2, "  II + II");

    bool ladderOk = a1.tier == MergeTier::T1 && a2.tier == MergeTier::T2
                 && a3.tier == MergeTier::T3 && ult.tier == MergeTier::Ultima
                 && cross.tier == MergeTier::T2plus && conv.tier == MergeTier::T3plus
                 && ult.chain.size() >= 5;

    // Запреты: одна и та же стихия, Ultima дальше, не-стихии
    bool guardsOk = !evaluateMerge(pure(ElementId::Fire), pure(ElementId::Fire)).ok
                 && !evaluateMerge(ult, pure(ElementId::Metal)).ok
                 && !evaluateMerge(genEffectCard(rng, 3), pure(ElementId::Fire)).ok
                 && !evaluateMerge(a3, pure(ElementId::Metal)).ok;   // III + база запрещён
    bool enchantOk = evaluateMerge(genSpellCard(rng, 3), pure(ElementId::Fire)).ok;
    std::printf("ladder       : %s, guards %s, enchant %s, ultima chain %d elements\n",
                ladderOk ? "OK" : "BROKEN", guardsOk ? "OK" : "BROKEN",
                enchantOk ? "OK" : "BROKEN", int(ult.chain.size()));

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

    // ── реликвии: каждая описана, запертые не выпадают без своего бита ───
    bool relicsOk = (relicCount() == int(RelicId::COUNT) - 1);
    for (int i = 1; i < int(RelicId::COUNT) && relicsOk; ++i) {
        const RelicDef& d = relicDef(RelicId(i));
        if (d.id != RelicId(i) || d.name.ru.empty() || d.desc.ru.empty()) relicsOk = false;
    }
    int poolLocked = 0, poolOpen = 0;
    {
        Rng rr(11u); std::vector<RelicId> owned;
        for (;;) {
            RelicId got = rollRelic(rr, owned, 0u);
            if (got == RelicId::None || poolLocked > 300) break;
            if (relicDef(got).unlock >= 0) { relicsOk = false; break; }
            owned.push_back(got); poolLocked++;
        }
    }
    {
        Rng rr(11u); std::vector<RelicId> owned;
        for (;;) {
            RelicId got = rollRelic(rr, owned, ~0u);
            if (got == RelicId::None || poolOpen > 300) break;
            owned.push_back(got); poolOpen++;
        }
    }
    bool unlockGrows = poolOpen > poolLocked;
    std::printf("relics       : %d described, pool %d locked -> %d open\n",
                relicCount(), poolLocked, poolOpen);

    // ── нисхождение: лестница монотонна, у каждой ступени есть правило ────
    bool ascOk = true;
    for (int i = 1; i <= ASCENSION_MAX; ++i) {
        Ascension a = ascensionAt(i), b = ascensionAt(i - 1);
        if (a.enemyDamage < b.enemyDamage - 0.001f) ascOk = false;
        if (a.rewardMul   > b.rewardMul   + 0.001f) ascOk = false;
        if (ascensionRule(i).ru.empty()) ascOk = false;
    }
    std::printf("descent      : %d steps, x%.2f damage at the bottom\n",
                ASCENSION_MAX, ascensionAt(ASCENSION_MAX).enemyDamage);

    bool ok = (stalls == 0) && (pairs == 120) && (maxChance <= 0.7001f) && (wins > 0)
           && (losses > 0) && ladderOk && guardsOk && enchantOk
           && relicsOk && unlockGrows && ascOk;
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
    bool savetest = false;
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--savetest") == 0) savetest = true;

    // Без MSAA: сглаживание краёв — прямая противоположность чёткому пиксель-арту.
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Shichoy / Шичой");
    SetWindowMinSize(1024, 640);
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);          // ESC используется внутри игры

    shichoy::Game game;
    game.init();

    if (savetest) {
        bool ok = game.debugSaveRoundTrip();
        std::printf("save round-trip: %s\n", ok ? "OK" : "FAILED");
        game.shutdown();
        CloseWindow();
        return ok ? 0 : 1;
    }

    if (shotMode) {
        // Служебный режим: пройтись по сценам и сохранить кадры.
        const char* names[] = { "shot_menu.png", "shot_overworld.png", "shot_combat.png",
                                "shot_craft.png", "shot_codex.png", "shot_reward.png",
                                "shot_tablepull.png", "shot_bighand.png",
                                "mg_timing.png", "mg_seal.png", "mg_steady.png", "mg_candles.png",
                                "mg_reflex.png", "mg_sequence.png", "mg_cipher.png", "",
                                "", "", "", "",
                                "en_reaper.png", "en_candelabrum.png", "en_portrait.png",
                                "en_hound.png", "en_automaton.png", "en_choir.png", "en_doll.png",
                                "", "", "",
                                "fx_fire.png", "fx_water.png", "", "", "fx_lightning.png",
                                "fx_ice.png", "", "fx_shadow.png", "", "fx_light.png",
                                "", "", "fx_blood.png", "", "fx_crystal.png", "fx_arcane.png",
                                "", "", "", "", "fx_darkfire.png",
                                "", "", "", "", "", "", "", "", "",
                                "shot_wanderer.png" };
        const int nameCount = int(sizeof(names) / sizeof(names[0]));
        for (int s = 0; s < nameCount; ++s) {
            if (names[s][0] == 0) continue;
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
