#include "app/Game.h"
#include "art/ProcArt.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace kv {

static const char* SAVE_FILE = "kartovasiya_meta.bin";

// ── Meta save / Сохранение мета-прогресса ─────────────────────────────────
static void saveMeta(const MetaProgress& m) {
    SaveFileData(SAVE_FILE, (void*)&m, int(sizeof(MetaProgress)));
}
static MetaProgress loadMeta() {
    MetaProgress m{};
    if (!FileExists(SAVE_FILE)) return m;
    int size = 0;
    unsigned char* data = LoadFileData(SAVE_FILE, &size);
    if (data && size == int(sizeof(MetaProgress))) std::memcpy(&m, data, sizeof(MetaProgress));
    if (data) UnloadFileData(data);
    return m;
}

// ── UI helpers ────────────────────────────────────────────────────────────
void Game::panel(Rectangle r, float alpha) {
    DrawRectangleRounded(r, 0.06f, 8, Color{ 12, 9, 16, (unsigned char)(235 * alpha) });
    DrawRectangleRoundedLinesEx(r, 0.06f, 8, 1.5f, withA(art::pal::Gold, 150));
    art::drawFiligree(r, withA(art::pal::Gold, 120), time_);
}

bool Game::button(Rectangle r, const char* label, bool enabled, float fontSize) {
    Vector2 m = GetMousePosition();
    bool over = enabled && CheckCollisionPointRec(m, r);
    Color fill = enabled ? (over ? hex(0x2A2038) : hex(0x191324)) : hex(0x141218);
    Color line = enabled ? (over ? art::pal::GoldLit : art::pal::Gold) : hex(0x3A3A42);
    DrawRectangleRounded(r, 0.25f, 6, fill);
    DrawRectangleRoundedLinesEx(r, 0.25f, 6, over ? 2.0f : 1.2f, line);
    Color tc = enabled ? (over ? art::pal::GoldLit : art::pal::Parchment) : hex(0x5A5A62);
    art::textC(label, r.x + r.width * 0.5f, r.y + (r.height - fontSize) * 0.5f, fontSize, tc);
    return over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void Game::drawBar(Rectangle r, float frac, Color fill, Color back, const char* label) {
    frac = clampf(frac, 0.0f, 1.0f);
    DrawRectangleRounded(r, 0.5f, 6, back);
    if (frac > 0.001f)
        DrawRectangleRounded(Rectangle{ r.x + 2, r.y + 2, (r.width - 4) * frac, r.height - 4 }, 0.5f, 6, fill);
    DrawRectangleRoundedLinesEx(r, 0.5f, 6, 1.2f, withA(art::pal::Gold, 170));
    if (label && label[0])
        art::textC(label, r.x + r.width * 0.5f, r.y + (r.height - 13.0f) * 0.5f, 13.0f, art::pal::Parchment);
}

void Game::drawStatusRow(const std::vector<StatusInst>& st, float x, float y, float size) {
    float cx = x;
    for (const auto& s : st) {
        const StatusDef& d = statusDef(s.id);
        DrawRectangleRounded(Rectangle{ cx, y, size * 2.6f, size }, 0.4f, 4, withA(d.tint, 60));
        DrawRectangleRoundedLinesEx(Rectangle{ cx, y, size * 2.6f, size }, 0.4f, 4, 1.0f, d.tint);
        char buf[48];
        snprintf(buf, sizeof buf, "%d/%d", s.stacks, s.turns);
        art::textC(buf, cx + size * 1.3f, y + (size - size * 0.62f) * 0.5f, size * 0.62f, art::pal::Parchment);
        // подпись
        art::text(d.name.c_str(), cx, y - size * 0.72f, size * 0.6f, withA(d.tint, 230));
        cx += size * 3.0f;
    }
}

void Game::toast(const LS& t) { toast_ = t; toastT_ = 3.0f; }

// ── Lifecycle ─────────────────────────────────────────────────────────────
void Game::init() {
    art::init();
    meta_ = loadMeta();
    rng_.reseed(uint64_t(GetTime() * 1000000.0) ^ 0x9E3779B97F4A7C15ull);
    g_rng.reseed(rng_.next());
    scene_ = Scene::Menu;
}

void Game::shutdown() {
    saveMeta(meta_);
    art::shutdown();
}

void Game::startRun() {
    depth_ = 1;
    diedLastRun_ = false;
    player_ = Player{};
    player_.maxHp = 60 + meta_.permHp;
    player_.hp = player_.maxHp;
    inv_ = Inventory{};
    deck_.reset(starterDeck(rng_));
    world_.generate(depth_, rng_);
    scene_ = Scene::Overworld;
    toast(LS("Забег начат. WASD — идти, E — взаимодействие.",
             "Run started. WASD to move, E to interact."));
}

void Game::endRun(bool died) {
    diedLastRun_ = died;
    meta_.runs++;
    meta_.bestDepth = std::max(meta_.bestDepth, depth_);
    if (!died) {
        meta_.bankedMaterials += inv_.total();
        meta_.permHp += inv_.total() / 12;
    } else {
        meta_.bankedMaterials += inv_.total() / 4;
        meta_.permHp += inv_.total() / 40;
    }
    meta_.unlockedCards = std::max(meta_.unlockedCards, int(deck_.library().size()));
    saveMeta(meta_);
    scene_ = Scene::RunEnd;
}

void Game::nextRegion() {
    depth_++;
    world_.generate(depth_, rng_);
    scene_ = Scene::Overworld;
    char buf[128];
    snprintf(buf, sizeof buf, g_lang == Lang::Ru ? "Глубина %d. Земля становится темнее."
                                                 : "Depth %d. The land grows darker.", depth_);
    toast(LS(buf, buf));
}

void Game::enterCombat(bool boss, int worldEntIndex) {
    pendingBoss_ = boss;
    pendingEnt_ = worldEntIndex;
    pullT_ = 0.0f;
    scene_ = Scene::TablePull;
}

void Game::afterCombat() {
    // Удаляем побеждённую сущность из мира
    if (pendingEnt_ >= 0 && pendingEnt_ < world_.entCount()) world_.removeEnt(pendingEnt_);
    pendingEnt_ = -1;
    world_.clearEncounter();
    if (pendingBoss_) {
        openDialog(LS("Руины проседают", "The Ruins Give Way"),
                   LS("Пол уходит вниз. Дальше — темнее и богаче. Спуститься?",
                      "The floor sinks away. Below is darker, and richer. Descend?"),
                   { { LS("Спуститься глубже", "Descend deeper"), 1 },
                     { LS("Остаться здесь", "Stay here"), 2 } },
                   100);
    } else {
        scene_ = Scene::Overworld;
    }
}

// ── Menu ──────────────────────────────────────────────────────────────────
void Game::updateMenu(float) {
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) menuIndex_ = (menuIndex_ + 1) % 4;
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))   menuIndex_ = (menuIndex_ + 3) % 4;
    bool go = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    if (go) {
        switch (menuIndex_) {
            case 0: startRun(); break;
            case 1: codexPage_ = 0; codexReturn_ = Scene::Menu; scene_ = Scene::Codex; break;
            case 2: if (art::hasCyrillic()) g_lang = (g_lang == Lang::Ru ? Lang::En : Lang::Ru); break;
            case 3: exit_ = true; break;
        }
    }
}

void Game::drawMenu() {
    ClearBackground(art::pal::Night);
    art::drawStarfield(sw_, sh_, time_, 7u);
    DrawCircleGradient(sw_ / 2, int(sh_ * 0.34f), sh_ * 0.5f, withA(hex(0x2A1840), 90), BLANK);

    float cx = sw_ * 0.5f;
    art::drawRuneRing(Vector2{ cx, sh_ * 0.30f }, sh_ * 0.20f, 16, withA(art::pal::Gold, 90), time_);
    art::drawRuneRing(Vector2{ cx, sh_ * 0.30f }, sh_ * 0.26f, 24, withA(art::pal::Blood, 60), -time_);

    art::drawAristocratSide(Vector2{ cx, sh_ * 0.36f }, 2.1f, time_,
                            0.5f + 0.5f * std::sin(time_ * 0.8f));

    const char* title = g_lang == Lang::Ru ? "КАРТОВАСИЯ" : "KARTOVASIYA";
    const char* sub = g_lang == Lang::Ru ? "бесконечный аркадный рогалик на картах Таро"
                                         : "an endless arcade roguelike of tarot cards";
    DrawTextEx(art::fontTitle(), title,
               Vector2{ cx - MeasureTextEx(art::fontTitle(), title, 54.0f, 6.0f).x * 0.5f, sh_ * 0.06f },
               54.0f, 6.0f, art::pal::GoldLit);
    art::textC(sub, cx, sh_ * 0.06f + 72.0f, 15.0f, withA(art::pal::Vellum, 200));

    const LS items[4] = {
        LS("Новый забег", "New Run"),
        LS("Кодекс стихий", "Codex of Elements"),
        LS("Язык: Русский / English", "Language: English / Русский"),
        LS("Выход", "Quit")
    };
    for (int i = 0; i < 4; ++i) {
        Rectangle r{ cx - 170.0f, sh_ * 0.56f + i * 52.0f, 340.0f, 42.0f };
        bool sel = (menuIndex_ == i);
        if (sel) DrawRectangleRoundedLinesEx(Rectangle{ r.x - 6, r.y - 4, r.width + 12, r.height + 8 },
                                             0.3f, 6, 1.5f, withA(art::pal::GoldLit, 200));
        if (button(r, items[i].c_str(), true, 19.0f)) {
            menuIndex_ = i;
            switch (i) {
                case 0: startRun(); break;
                case 1: codexPage_ = 0; codexReturn_ = Scene::Menu; scene_ = Scene::Codex; break;
                case 2: if (art::hasCyrillic()) g_lang = (g_lang == Lang::Ru ? Lang::En : Lang::Ru); break;
                case 3: exit_ = true; break;
            }
        }
        if (CheckCollisionPointRec(GetMousePosition(), r)) menuIndex_ = i;
    }

    char buf[160];
    snprintf(buf, sizeof buf,
             g_lang == Lang::Ru ? "Забегов: %d   Лучшая глубина: %d   Банк материалов: %d   +%d HP навсегда"
                                : "Runs: %d   Best depth: %d   Banked materials: %d   +%d permanent HP",
             meta_.runs, meta_.bestDepth, meta_.bankedMaterials, meta_.permHp);
    art::textC(buf, cx, sh_ - 40.0f, 14.0f, withA(art::pal::Vellum, 170));
    if (!art::hasCyrillic())
        art::textC("Cyrillic font not found - running in English", cx, sh_ - 62.0f, 13.0f,
                   withA(art::pal::Risk, 200));
    art::drawVignette(sw_, sh_, 1.0f);
}

// ── Overworld ─────────────────────────────────────────────────────────────
void Game::updateOverworld(float dt) {
    world_.update(dt, true);

    int enc = world_.pendingEncounter();
    if (enc >= 0) { enterCombat(false, enc); return; }

    if (IsKeyPressed(KEY_E)) {
        int i = world_.nearestInteractable(46.0f);
        if (i >= 0) interact(i);
    }
    if (IsKeyPressed(KEY_C)) openCraft();
    if (IsKeyPressed(KEY_K)) { codexPage_ = 0; codexReturn_ = Scene::Overworld; scene_ = Scene::Codex; }
    if (IsKeyPressed(KEY_ESCAPE)) {
        openDialog(LS("Покинуть забег?", "Leave the run?"),
                   LS("Карета ждёт. Всё собранное отправится в банк, забег закончится.",
                      "The carriage waits. Everything gathered is banked and the run ends."),
                   { { LS("Уехать (закончить забег)", "Ride out (end the run)"), 10 },
                     { LS("Остаться", "Stay"), 11 } },
                   200);
    }
}

void Game::drawOverworld() {
    world_.draw(time_);

    // ── HUD ───────────────────────────────────────────────────────────────
    panel(Rectangle{ 12, 12, 268, 84 }, 0.85f);
    char buf[128];
    snprintf(buf, sizeof buf, g_lang == Lang::Ru ? "Глубина %d" : "Depth %d", depth_);
    art::text(buf, 26, 22, 17.0f, art::pal::GoldLit);
    drawBar(Rectangle{ 26, 46, 240, 18 }, float(player_.hp) / float(player_.maxHp),
            hex(0x8C1A26), hex(0x241820), TextFormat("HP %d / %d", player_.hp, player_.maxHp));
    snprintf(buf, sizeof buf, g_lang == Lang::Ru ? "Материалы: %d   Карты: %d"
                                                 : "Materials: %d   Cards: %d",
             inv_.total(), int(deck_.library().size()));
    art::text(buf, 26, 70, 13.0f, withA(art::pal::Vellum, 210));

    // Подсказки
    int near_ = world_.nearestInteractable(46.0f);
    const char* hint = g_lang == Lang::Ru
        ? "WASD — идти   E — взаимодействие   C — крафт   K — кодекс   ESC — уехать"
        : "WASD move   E interact   C craft   K codex   ESC leave";
    art::textC(hint, sw_ * 0.5f, float(sh_ - 30), 14.0f, withA(art::pal::Vellum, 160));

    if (near_ >= 0) {
        const WorldEnt& e = world_.ent(near_);
        Vector2 sp = GetWorldToScreen2D(e.pos, world_.camera());
        std::string label = e.name.str();
        if (label.empty()) label = g_lang == Lang::Ru ? "Осмотреть" : "Examine";
        float w = art::textWidth(label.c_str(), 15.0f) + 26.0f;
        Rectangle box{ sp.x - w * 0.5f, sp.y - 58.0f, w, 26.0f };
        panel(box, 0.9f);
        art::textC(label.c_str(), sp.x, sp.y - 52.0f, 15.0f, art::pal::GoldLit);
        art::textC(g_lang == Lang::Ru ? "[E]" : "[E]", sp.x, sp.y - 28.0f, 13.0f, art::pal::Gold);
    }

    // Индикатор опасности
    float danger = world_.dangerAt(world_.playerPos());
    if (danger < 260.0f) {
        float k = 1.0f - danger / 260.0f;
        DrawRectangleGradientV(0, 0, sw_, 90, withA(art::pal::Blood, (unsigned char)(90 * k)), BLANK);
        if (danger < 130.0f)
            art::textC(g_lang == Lang::Ru ? "Что-то идёт за вами" : "Something is following you",
                       sw_ * 0.5f, 26.0f, 16.0f, withA(art::pal::Risk, (unsigned char)(200 * k)));
    }

    if (toastT_ > 0.0f) {
        float a = clampf(toastT_, 0.0f, 1.0f);
        Rectangle box{ sw_ * 0.5f - 300.0f, sh_ - 92.0f, 600.0f, 34.0f };
        DrawRectangleRounded(box, 0.4f, 6, Color{ 10, 8, 14, (unsigned char)(210 * a) });
        DrawRectangleRoundedLinesEx(box, 0.4f, 6, 1.2f, withA(art::pal::Gold, (unsigned char)(180 * a)));
        art::textC(toast_.c_str(), sw_ * 0.5f, sh_ - 83.0f, 15.0f,
                   withA(art::pal::Parchment, (unsigned char)(240 * a)));
    }
}

// ── Interaction / Взаимодействие ──────────────────────────────────────────
void Game::interact(int entIndex) {
    WorldEnt& e = world_.ent(entIndex);
    pendingEnt_ = entIndex;

    switch (e.kind) {
        case EntKind::Resource: {
            int n = rng_.i(1, 2 + depth_ / 3);
            inv_.add(e.data, n);
            char buf[160];
            snprintf(buf, sizeof buf, g_lang == Lang::Ru ? "Собрано: %s x%d" : "Gathered: %s x%d",
                     material(e.data).name.c_str(), n);
            toast(LS(buf, buf));
            world_.removeEnt(entIndex);
            pendingEnt_ = -1;
            break;
        }
        case EntKind::Npc: {
            if (e.data == 0) {
                openDialog(e.name,
                    LS("«Сядь. Отвар горький, но кровь остановит.»",
                       "\"Sit. The brew is bitter, but it stops the bleeding.\""),
                    { { LS("Выпить отвар (5 материалов, +30% HP)", "Drink (5 materials, +30% HP)"), 20,
                        inv_.total() >= 5 },
                      { LS("Попросить оберег (8 материалов, +8 макс. HP)", "Ask for a charm (8 materials, +8 max HP)"), 21,
                        inv_.total() >= 8 },
                      { LS("Уйти", "Leave"), 0 } }, 1);
            } else if (e.data == 1) {
                openDialog(e.name,
                    LS("«Приноси материал — вырежу тебе новую карту. Резец скользит, рука дрожит.»",
                       "\"Bring me stock and I will cut you a new card. The burin slips, the hand shakes.\""),
                    { { LS("Открыть верстак", "Open the workbench"), 22 },
                      { LS("Уйти", "Leave"), 0 } }, 1);
            } else {
                openDialog(e.name,
                    LS("«Реликвии? Беру всё. Плачу картами — своими, ворованными, чужими.»",
                       "\"Relics? I take everything. I pay in cards — mine, stolen, someone else's.\""),
                    { { LS("Обменять 10 материалов на карту", "Trade 10 materials for a card"), 23,
                        inv_.total() >= 10 },
                      { LS("Переплавить материалы", "Transmute materials"), 24 },
                      { LS("Уйти", "Leave"), 0 } }, 1);
            }
            break;
        }
        case EntKind::EventSite: {
            static const LS bodies[4] = {
                LS("Алтарь без святого. На камне — свежая зарубка и след ладони.",
                   "An altar with no saint. A fresh notch on the stone, and a handprint."),
                LS("Колодец отвечает вашим голосом, но говорит не то, что вы сказали.",
                   "The well answers in your voice, but not with your words."),
                LS("Карета лежит на боку. Внутри — чужая колода, разбухшая от сырости.",
                   "The carriage lies on its side. Inside, someone's deck, swollen with damp."),
                LS("Камни стоят кругом. В центре — выжженная трава и очень тихо.",
                   "Stones in a ring. In the middle, scorched grass and a great quiet.")
            };
            openDialog(e.name, bodies[clampi(e.data, 0, 3)],
                { { LS("Рискнуть (мини-игра)", "Take the risk (mini-game)"), 30 },
                  { LS("Обыскать спокойно (материалы)", "Search calmly (materials)"), 31 },
                  { LS("Пройти мимо", "Walk on"), 0 } }, 2);
            break;
        }
        case EntKind::RuinGate:
            openDialog(e.name,
                LS("За воротами что-то очень старое и очень терпеливое. Оно ждёт стола.",
                   "Beyond the gate is something very old and very patient. It waits for the table."),
                { { LS("Войти (бой с боссом)", "Enter (boss fight)"), 40 },
                  { LS("Не сейчас", "Not now"), 0 } }, 3);
            break;
        case EntKind::Carriage:
            openDialog(e.name,
                LS("Лошади стоят смирно. Уехать — значит сохранить добытое.",
                   "The horses stand still. To ride out is to keep what you have."),
                { { LS("Уехать (закончить забег)", "Ride out (end the run)"), 10 },
                  { LS("Остаться", "Stay"), 0 } }, 4);
            break;
        default: break;
    }
}

void Game::openDialog(const LS& title, const LS& body, std::vector<DialogOption> opts, int context) {
    dlgTitle_ = title;
    dlgBody_ = body;
    dlgOpts_ = std::move(opts);
    dlgContext_ = context;
    scene_ = Scene::Dialog;
}

void Game::resolveDialog(int action) {
    switch (action) {
        case 0:   scene_ = Scene::Overworld; pendingEnt_ = -1; break;
        case 1:   nextRegion(); break;                       // после босса — глубже
        case 2:   scene_ = Scene::Overworld; break;
        case 10:  endRun(false); break;
        case 11:  scene_ = Scene::Overworld; break;
        case 20: {                                            // отвар
            int cost = 5, paid = 0;
            for (int i = 0; i < MATERIAL_COUNT && paid < cost; ++i) {
                int take = std::min(inv_.mats[i], cost - paid);
                inv_.mats[i] -= take; paid += take;
            }
            int heal = int(player_.maxHp * 0.30f);
            player_.hp = std::min(player_.maxHp, player_.hp + heal);
            toast(LS("Отвар горчит. Раны затягиваются.", "The brew is bitter. Wounds close."));
            scene_ = Scene::Overworld;
            break;
        }
        case 21: {                                            // оберег
            int cost = 8, paid = 0;
            for (int i = 0; i < MATERIAL_COUNT && paid < cost; ++i) {
                int take = std::min(inv_.mats[i], cost - paid);
                inv_.mats[i] -= take; paid += take;
            }
            player_.maxHp += 8;
            player_.hp += 8;
            toast(LS("Оберег на шее. +8 к максимуму HP.", "A charm at your throat. +8 max HP."));
            scene_ = Scene::Overworld;
            break;
        }
        case 22:  openCraft(); break;
        case 23: {                                            // карта за материалы
            int cost = 10, paid = 0;
            for (int i = 0; i < MATERIAL_COUNT && paid < cost; ++i) {
                int take = std::min(inv_.mats[i], cost - paid);
                inv_.mats[i] -= take; paid += take;
            }
            Card c = genCard(rng_, depth_ + 1);
            deck_.addCard(c);
            toast(LS(std::string("Получена карта: ") + c.name.ru,
                     std::string("Card acquired: ") + c.name.en));
            scene_ = Scene::Overworld;
            break;
        }
        case 24:
            craftAction_ = int(CraftAction::Transmute);
            craftTarget_ = -1;
            plan_ = makePlan(CraftAction::Transmute, depth_, deck_, -1, rng_);
            planActive_ = false;
            craftResult_ = LS("", "");
            scene_ = Scene::Craft;
            break;
        case 30:                                              // событие: риск
            craftAction_ = int(CraftAction::Mutate);
            craftTarget_ = deck_.library().empty() ? -1 : rng_.i(0, int(deck_.library().size()) - 1);
            plan_ = makePlan(CraftAction::Mutate, depth_, deck_, craftTarget_, rng_);
            for (int i = 0; i < MATERIAL_COUNT; ++i) plan_.cost[i] = 0;   // событие бесплатное
            plan_.title = LS("Событие: рискнуть", "Event: take the risk");
            planActive_ = false;
            craftResult_ = LS("", "");
            scene_ = Scene::Craft;
            if (pendingEnt_ >= 0) { world_.removeEnt(pendingEnt_); pendingEnt_ = -1; }
            break;
        case 31: {                                            // событие: обыск
            int rolls = 2 + depth_ / 2;
            for (int i = 0; i < rolls; ++i) inv_.add(rng_.i(0, MATERIAL_COUNT - 1), rng_.i(1, 2));
            toast(LS("Вы обыскали место без спешки.", "You searched the place without haste."));
            if (pendingEnt_ >= 0) { world_.removeEnt(pendingEnt_); pendingEnt_ = -1; }
            scene_ = Scene::Overworld;
            break;
        }
        case 40:  enterCombat(true, pendingEnt_); break;
        case 100: nextRegion(); break;
        case 101: scene_ = Scene::Overworld; break;
        default:  scene_ = Scene::Overworld; break;
    }
}

void Game::updateDialog(float) {
    for (int i = 0; i < int(dlgOpts_.size()); ++i) {
        if (IsKeyPressed(KEY_ONE + i) && dlgOpts_[size_t(i)].enabled) {
            int a = dlgOpts_[size_t(i)].action;
            if (dlgContext_ == 100) a = (i == 0 ? 100 : 101);
            resolveDialog(a);
            return;
        }
    }
    if (IsKeyPressed(KEY_ESCAPE)) resolveDialog(dlgContext_ == 100 ? 101 : 0);
}

void Game::drawDialog() {
    drawOverworld();
    DrawRectangle(0, 0, sw_, sh_, Color{ 0, 0, 0, 170 });

    Rectangle box{ sw_ * 0.5f - 340.0f, sh_ * 0.5f - 190.0f, 680.0f, 380.0f };
    panel(box, 1.0f);
    art::textC(dlgTitle_.c_str(), box.x + box.width * 0.5f, box.y + 22.0f, 24.0f, art::pal::GoldLit);
    DrawLineEx(Vector2{ box.x + 40, box.y + 56 }, Vector2{ box.x + box.width - 40, box.y + 56 },
               1.2f, withA(art::pal::Gold, 130));
    art::textWrapped(dlgBody_.c_str(), Rectangle{ box.x + 36, box.y + 72, box.width - 72, 110 },
                     16.0f, withA(art::pal::Parchment, 235), 5.0f);

    for (int i = 0; i < int(dlgOpts_.size()); ++i) {
        const DialogOption& o = dlgOpts_[size_t(i)];
        Rectangle r{ box.x + 40, box.y + 196.0f + i * 48.0f, box.width - 80, 40.0f };
        char label[256];
        snprintf(label, sizeof label, "%d.  %s", i + 1, o.text.c_str());
        if (button(r, label, o.enabled, 16.0f)) {
            int a = o.action;
            if (dlgContext_ == 100) a = (i == 0 ? 100 : 101);
            resolveDialog(a);
            return;
        }
    }
}

// ── Table pull / «ГГ достаёт стол» ────────────────────────────────────────
void Game::updateTablePull(float dt) {
    pullT_ += dt;
    if (pullT_ >= 1.55f) {
        Enemy e = genEnemy(rng_, depth_ + (pendingBoss_ ? 2 : 0), pendingBoss_);
        combat_.begin(&deck_, &player_, std::move(e), depth_ + (pendingBoss_ ? 2 : 0), &rng_);
        hoverCard_ = -1;
        rewardTaken_ = false;
        scene_ = Scene::Combat;
    }
}

void Game::drawTablePull() {
    world_.draw(time_);
    float k = clampf(pullT_ / 1.55f, 0.0f, 1.0f);

    // Затемнение и «схлопывание» камеры
    DrawRectangle(0, 0, sw_, sh_, Color{ 0, 0, 0, (unsigned char)(180 + 75 * k) });
    float bar = k * sh_ * 0.14f;
    DrawRectangle(0, 0, sw_, int(bar), BLACK);
    DrawRectangle(0, sh_ - int(bar), sw_, int(bar), BLACK);

    float scale = 2.2f + k * 1.6f;
    Vector2 p{ sw_ * 0.42f, sh_ * 0.62f + (1.0f - k) * 40.0f };
    float pull = clampf((pullT_ - 0.35f) / 0.9f, 0.0f, 1.0f);
    art::drawAristocratSide(p, scale, time_, pull);

    art::drawRuneRing(Vector2{ sw_ * 0.5f, sh_ * 0.5f }, sh_ * (0.42f - k * 0.10f),
                      18, withA(art::pal::Blood, (unsigned char)(160 * k)), time_ * 2.0f);

    const char* line = g_lang == Lang::Ru ? "Он достаёт стол." : "He takes out the table.";
    unsigned char a = (unsigned char)(255 * clampf((k - 0.25f) / 0.4f, 0.0f, 1.0f));
    art::textC(line, sw_ * 0.5f, sh_ * 0.18f, 30.0f, withA(art::pal::GoldLit, a));
    art::drawVignette(sw_, sh_, 1.0f);
}

// ── Craft ─────────────────────────────────────────────────────────────────
void Game::openCraft() {
    craftAction_ = 0;
    craftTarget_ = deck_.library().empty() ? -1 : 0;
    craftScroll_ = 0;
    planActive_ = false;
    craftResult_ = LS("", "");
    plan_ = makePlan(CraftAction::Forge, depth_, deck_, craftTarget_, rng_);
    mini_.reset();
    scene_ = Scene::Craft;
}

void Game::updateCraft(float dt) {
    mini_.update(dt);

    if (mini_.finished() && planActive_) {
        planActive_ = false;
        craftResult_ = applyCraft(plan_, mini_.success(), mini_.perfect(), deck_, inv_, depth_, rng_);
    }
    if (mini_.running() && (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)))
        mini_.commit();

    if (IsKeyPressed(KEY_ESCAPE) && !mini_.running()) {
        scene_ = Scene::Overworld;
        pendingEnt_ = -1;
    }
}

void Game::drawCraft() {
    ClearBackground(art::pal::Night);
    art::drawStarfield(sw_, sh_, time_, 21u);
    DrawCircleGradient(sw_ / 2, int(sh_ * 0.4f), sh_ * 0.6f, withA(hex(0x2A1030), 110), BLANK);

    art::textC(g_lang == Lang::Ru ? "ВЕРСТАК" : "WORKBENCH", sw_ * 0.5f, 24.0f, 28.0f, art::pal::GoldLit);
    art::textC(g_lang == Lang::Ru ? "Шанс успеха ограничен 70%. Остальное решает рука."
                                  : "Success is capped at 70%. The rest is in your hand.",
               sw_ * 0.5f, 60.0f, 14.0f, withA(art::pal::Vellum, 190));

    // Материалы
    Rectangle matBox{ 24, 96, 250, 250 };
    panel(matBox, 0.95f);
    art::text(g_lang == Lang::Ru ? "Материалы" : "Materials", matBox.x + 16, matBox.y + 12, 17.0f,
              art::pal::GoldLit);
    for (int i = 0; i < MATERIAL_COUNT; ++i) {
        float y = matBox.y + 44.0f + i * 25.0f;
        DrawCircleV(Vector2{ matBox.x + 26, y + 7 }, 5.0f, material(i).tint);
        art::text(material(i).name.c_str(), matBox.x + 40, y, 14.0f, art::pal::Parchment);
        char n[16]; snprintf(n, sizeof n, "%d", inv_.get(i));
        art::text(n, matBox.x + matBox.width - 44, y, 14.0f, art::pal::GoldLit);
    }

    // Действия
    Rectangle actBox{ 292, 96, 300, 250 };
    panel(actBox, 0.95f);
    art::text(g_lang == Lang::Ru ? "Работа" : "Work", actBox.x + 16, actBox.y + 12, 17.0f, art::pal::GoldLit);
    static const LS actNames[4] = {
        LS("Выковать карту", "Forge a Card"), LS("Усилить карту", "Upgrade a Card"),
        LS("Мутировать карту", "Mutate a Card"), LS("Переплавить", "Transmute")
    };
    for (int i = 0; i < 4; ++i) {
        Rectangle r{ actBox.x + 16, actBox.y + 46.0f + i * 46.0f, actBox.width - 32, 38.0f };
        bool sel = (craftAction_ == i);
        if (sel) DrawRectangleRounded(Rectangle{ r.x - 4, r.y - 3, r.width + 8, r.height + 6 }, 0.3f, 6,
                                      withA(art::pal::Gold, 40));
        if (button(r, actNames[i].c_str(), !mini_.running(), 15.0f)) {
            craftAction_ = i;
            plan_ = makePlan(CraftAction(i), depth_, deck_, craftTarget_, rng_);
            craftResult_ = LS("", "");
        }
    }

    // План
    Rectangle planBox{ 610, 96, sw_ - 634.0f, 250 };
    panel(planBox, 0.95f);
    art::text(plan_.title.c_str(), planBox.x + 16, planBox.y + 12, 18.0f, art::pal::GoldLit);
    art::textWrapped(plan_.desc.c_str(), Rectangle{ planBox.x + 16, planBox.y + 42, planBox.width - 32, 60 },
                     14.0f, withA(art::pal::Parchment, 225), 3.0f);
    art::text(g_lang == Lang::Ru ? "Цена:" : "Cost:", planBox.x + 16, planBox.y + 108, 15.0f, art::pal::Vellum);
    float cx = planBox.x + 80;
    bool affordable = inv_.canPay(plan_.cost);
    for (int i = 0; i < MATERIAL_COUNT; ++i) {
        if (plan_.cost[i] <= 0) continue;
        DrawCircleV(Vector2{ cx, planBox.y + 116 }, 5.0f, material(i).tint);
        char n[32]; snprintf(n, sizeof n, "%s x%d", material(i).name.c_str(), plan_.cost[i]);
        Color c = inv_.get(i) >= plan_.cost[i] ? art::pal::Parchment : art::pal::Risk;
        art::text(n, cx + 10, planBox.y + 108, 14.0f, c);
        cx += art::textWidth(n, 14.0f) + 34.0f;
    }
    char pc[64];
    snprintf(pc, sizeof pc, g_lang == Lang::Ru ? "Шанс: %d%%" : "Chance: %d%%", int(plan_.chance * 100));
    art::text(pc, planBox.x + 16, planBox.y + 140, 17.0f, art::pal::GoldLit);

    bool needTarget = (craftAction_ == 1 || craftAction_ == 2);
    bool ok = affordable && (!needTarget || craftTarget_ >= 0) && !mini_.running();
    Rectangle startBtn{ planBox.x + planBox.width - 214.0f, planBox.y + 186.0f, 198.0f, 44.0f };
    if (button(startBtn, g_lang == Lang::Ru ? "Начать работу [SPACE]" : "Begin work [SPACE]", ok, 16.0f)) {
        plan_.targetCard = craftTarget_;
        mini_.start(plan_.chance, rng_);
        planActive_ = true;
        craftResult_ = LS("", "");
    }

    // Мини-игра
    Rectangle bar{ 24, 372, sw_ - 48.0f, 54.0f };
    panel(bar, 0.95f);
    Rectangle track{ bar.x + 20, bar.y + 18, bar.width - 40, 20 };
    DrawRectangleRounded(track, 0.5f, 6, hex(0x141018));
    if (mini_.running() || mini_.finished()) {
        Rectangle zone{ track.x + track.width * mini_.zoneStart(), track.y,
                        track.width * (mini_.zoneEnd() - mini_.zoneStart()), track.height };
        DrawRectangleRounded(zone, 0.5f, 6, withA(hex(0x2D6A4F), 190));
        Rectangle pz{ track.x + track.width * mini_.perfectStart(), track.y,
                      track.width * (mini_.perfectEnd() - mini_.perfectStart()), track.height };
        DrawRectangleRounded(pz, 0.5f, 6, withA(art::pal::GoldLit, 210));
        float mx = track.x + track.width * mini_.marker();
        DrawRectangle(int(mx) - 2, int(track.y) - 6, 4, int(track.height) + 12, art::pal::Parchment);
    }
    DrawRectangleRoundedLinesEx(track, 0.5f, 6, 1.2f, withA(art::pal::Gold, 150));
    if (mini_.running())
        art::textC(g_lang == Lang::Ru ? "SPACE / клик — остановить" : "SPACE / click to stop",
                   sw_ * 0.5f, bar.y + 42.0f, 13.0f, withA(art::pal::Vellum, 200));
    if (!craftResult_.empty())
        art::textC(craftResult_.c_str(), sw_ * 0.5f, bar.y + 42.0f, 15.0f,
                   mini_.success() ? hex(0x59C772) : art::pal::Risk);

    // Библиотека карт (цель для усиления/мутации)
    Rectangle lib{ 24, 440, sw_ - 48.0f, sh_ - 480.0f };
    panel(lib, 0.95f);
    art::text(g_lang == Lang::Ru ? "Ваши карты (клик — выбрать цель)"
                                 : "Your cards (click to target)",
              lib.x + 16, lib.y + 10, 15.0f, art::pal::GoldLit);
    const auto& cards = deck_.library();
    float cw = 92.0f, ch = 128.0f, gap = 10.0f;
    int perRow = std::max(1, int((lib.width - 32) / (cw + gap)));
    int maxRows = std::max(1, int((lib.height - 40) / (ch + gap)));
    int shown = perRow * maxRows;
    int maxScroll = std::max(0, (int(cards.size()) - shown + perRow - 1) / perRow);
    craftScroll_ = clampi(craftScroll_ + int(-GetMouseWheelMove()), 0, maxScroll);
    for (int i = 0; i < shown; ++i) {
        int idx = i + craftScroll_ * perRow;
        if (idx >= int(cards.size())) break;
        Rectangle r{ lib.x + 16 + (i % perRow) * (cw + gap),
                     lib.y + 34 + (i / perRow) * (ch + gap), cw, ch };
        Vector2 m = GetMousePosition();
        bool over = CheckCollisionPointRec(m, r);
        art::drawCard(cards[size_t(idx)], r, over ? 1.0f : 0.0f, craftTarget_ == idx, time_);
        if (over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !mini_.running()) {
            craftTarget_ = idx;
            plan_ = makePlan(CraftAction(craftAction_), depth_, deck_, craftTarget_, rng_);
        }
    }

    art::textC(g_lang == Lang::Ru ? "ESC — закрыть верстак" : "ESC — close the workbench",
               sw_ * 0.5f, float(sh_ - 22), 13.0f, withA(art::pal::Vellum, 170));
    art::drawVignette(sw_, sh_, 0.85f);
}

// ── Run end ───────────────────────────────────────────────────────────────
void Game::updateRunEnd(float) {
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE))
        scene_ = Scene::Menu;
}

void Game::drawRunEnd() {
    ClearBackground(art::pal::Night);
    art::drawStarfield(sw_, sh_, time_, 33u);
    Rectangle box{ sw_ * 0.5f - 320.0f, sh_ * 0.5f - 190.0f, 640.0f, 380.0f };
    panel(box, 1.0f);
    const char* title = diedLastRun_
        ? (g_lang == Lang::Ru ? "ЗАБЕГ ОБОРВАН" : "THE RUN IS CUT SHORT")
        : (g_lang == Lang::Ru ? "ВЫ УЕХАЛИ" : "YOU RODE OUT");
    art::textC(title, box.x + box.width * 0.5f, box.y + 28.0f, 30.0f,
               diedLastRun_ ? art::pal::Blood : art::pal::GoldLit);

    char buf[256];
    snprintf(buf, sizeof buf, g_lang == Lang::Ru ? "Глубина: %d" : "Depth reached: %d", depth_);
    art::textC(buf, box.x + box.width * 0.5f, box.y + 92.0f, 18.0f, art::pal::Parchment);
    snprintf(buf, sizeof buf, g_lang == Lang::Ru ? "Карт в колоде: %d" : "Cards in deck: %d",
             int(deck_.library().size()));
    art::textC(buf, box.x + box.width * 0.5f, box.y + 124.0f, 18.0f, art::pal::Parchment);
    snprintf(buf, sizeof buf, g_lang == Lang::Ru ? "В банк отправлено материалов: %d"
                                                 : "Materials banked: %d", meta_.bankedMaterials);
    art::textC(buf, box.x + box.width * 0.5f, box.y + 156.0f, 18.0f, art::pal::Vellum);
    snprintf(buf, sizeof buf, g_lang == Lang::Ru ? "Постоянный бонус: +%d HP" : "Permanent bonus: +%d HP",
             meta_.permHp);
    art::textC(buf, box.x + box.width * 0.5f, box.y + 188.0f, 18.0f, hex(0x95D5B2));

    art::textWrapped(diedLastRun_
        ? (g_lang == Lang::Ru ? "Стол остался в траве. Кто-то поднимет его следующей ночью."
                              : "The table stays in the grass. Someone will pick it up tomorrow night.")
        : (g_lang == Lang::Ru ? "Колёса стучат по гнилым доскам. Вы увозите добытое."
                              : "Wheels rattle over rotten boards. You carry your spoils home."),
        Rectangle{ box.x + 40, box.y + 232, box.width - 80, 60 }, 15.0f,
        withA(art::pal::Vellum, 210), 4.0f);

    Rectangle b{ box.x + box.width * 0.5f - 110.0f, box.y + box.height - 66.0f, 220.0f, 42.0f };
    if (button(b, g_lang == Lang::Ru ? "В меню [ENTER]" : "To menu [ENTER]", true, 17.0f))
        scene_ = Scene::Menu;
    art::drawVignette(sw_, sh_, 1.0f);
}

// ── Codex ─────────────────────────────────────────────────────────────────
void Game::updateCodex(float) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_K)) scene_ = codexReturn_;
    if (IsKeyPressed(KEY_RIGHT)) codexPage_ = (codexPage_ + 1) % 2;
    if (IsKeyPressed(KEY_LEFT))  codexPage_ = (codexPage_ + 1) % 2;
}

void Game::drawCodex() {
    ClearBackground(art::pal::Night);
    art::drawStarfield(sw_, sh_, time_, 11u);
    art::textC(g_lang == Lang::Ru ? "КОДЕКС" : "CODEX", sw_ * 0.5f, 20.0f, 28.0f, art::pal::GoldLit);
    art::textC(codexPage_ == 0
        ? (g_lang == Lang::Ru ? "16 стихий — метка от первой карты, реакция от второй"
                              : "16 elements — the first card marks, the second reacts")
        : (g_lang == Lang::Ru ? "Реакции: пары стихий" : "Reactions: element pairs"),
        sw_ * 0.5f, 56.0f, 14.0f, withA(art::pal::Vellum, 200));

    if (codexPage_ == 0) {
        const auto& els = allElements();
        int cols = 4;
        float w = (sw_ - 80.0f) / cols, h = 120.0f;
        for (int i = 1; i < int(els.size()); ++i) {
            int k = i - 1;
            Rectangle r{ 40.0f + (k % cols) * w, 90.0f + (k / cols) * h, w - 12.0f, h - 12.0f };
            panel(r, 0.9f);
            const ElementDef& e = els[size_t(i)];
            art::drawSigil(e.id, Vector2{ r.x + 40, r.y + r.height * 0.5f }, 24.0f, e.primary, time_);
            art::text(e.name.c_str(), r.x + 76, r.y + 16, 17.0f, lerpC(e.primary, WHITE, 0.4f));
            art::textWrapped(e.desc.c_str(), Rectangle{ r.x + 76, r.y + 40, r.width - 88, 44 }, 12.0f,
                             withA(art::pal::Parchment, 210), 2.0f);
            char b[96];
            snprintf(b, sizeof b, g_lang == Lang::Ru ? "метка: %s" : "mark: %s",
                     statusDef(e.mark).name.c_str());
            art::text(b, r.x + 76, r.y + r.height - 24, 12.0f, withA(art::pal::Gold, 210));
        }
    } else {
        // Список известных пар — берём подмножество для читаемости
        int cols = 3, shown = 0;
        float w = (sw_ - 80.0f) / cols;
        for (int a = 1; a <= ELEMENT_COUNT && shown < 27; ++a) {
            for (int b = a + 1; b <= ELEMENT_COUNT && shown < 27; ++b) {
                ReactionDef rc = reactionFor(ElementId(a), ElementId(b));
                if (!rc.valid) continue;
                if (rc.name.ru.rfind("Смесь", 0) == 0 || rc.name.ru.rfind("Разлад", 0) == 0) continue;
                Rectangle r{ 40.0f + (shown % cols) * w, 90.0f + (shown / cols) * 66.0f, w - 12.0f, 58.0f };
                panel(r, 0.9f);
                DrawCircleV(Vector2{ r.x + 18, r.y + 20 }, 7.0f, elem(ElementId(a)).primary);
                DrawCircleV(Vector2{ r.x + 34, r.y + 20 }, 7.0f, elem(ElementId(b)).primary);
                art::text(rc.name.c_str(), r.x + 50, r.y + 10, 14.0f, rc.tint);
                art::text(rc.desc.c_str(), r.x + 50, r.y + 30, 11.0f, withA(art::pal::Parchment, 200));
                shown++;
            }
        }
    }
    art::textC(g_lang == Lang::Ru ? "← → — страницы    ESC — назад" : "← → pages    ESC back",
               sw_ * 0.5f, float(sh_ - 26), 13.0f, withA(art::pal::Vellum, 180));
    art::drawVignette(sw_, sh_, 0.9f);
}

// ── Frame ─────────────────────────────────────────────────────────────────
void Game::frame() {
    float dt = GetFrameTime();
    if (dt > 0.1f) dt = 0.1f;
    time_ += dt;
    sw_ = GetScreenWidth();
    sh_ = GetScreenHeight();
    if (toastT_ > 0.0f) toastT_ -= dt;

    switch (scene_) {
        case Scene::Menu:      updateMenu(dt); break;
        case Scene::Overworld: updateOverworld(dt); break;
        case Scene::TablePull: updateTablePull(dt); break;
        case Scene::Combat:    updateCombat(dt); break;
        case Scene::Reward:    updateReward(dt); break;
        case Scene::Dialog:    updateDialog(dt); break;
        case Scene::Craft:     updateCraft(dt); break;
        case Scene::RunEnd:    updateRunEnd(dt); break;
        case Scene::Codex:     updateCodex(dt); break;
    }

    BeginDrawing();
    switch (scene_) {
        case Scene::Menu:      drawMenu(); break;
        case Scene::Overworld: drawOverworld(); break;
        case Scene::TablePull: drawTablePull(); break;
        case Scene::Combat:    drawCombat(); break;
        case Scene::Reward:    drawReward(); break;
        case Scene::Dialog:    drawDialog(); break;
        case Scene::Craft:     drawCraft(); break;
        case Scene::RunEnd:    drawRunEnd(); break;
        case Scene::Codex:     drawCodex(); break;
    }
    EndDrawing();
}

} // namespace kv
