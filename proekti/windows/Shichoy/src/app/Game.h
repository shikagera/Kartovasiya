// Game / Игра: сцены, переходы, UI. Windows, raylib, без внешних ассетов.
#pragma once
#include "world/Overworld.h"
#include "combat/Combat.h"
#include "craft/Craft.h"

namespace shichoy {

enum class Scene : int {
    Menu = 0,
    Overworld,
    TablePull,     // анимация «ГГ достаёт стол»
    Combat,
    Reward,
    Dialog,
    Craft,
    RunEnd,
    Codex          // справочник стихий и реакций
};

struct DialogOption {
    LS text;
    int action = 0;
    bool enabled = true;
};

struct MetaProgress {
    int runs = 0;
    int bestDepth = 0;
    int bankedMaterials = 0;
    int permHp = 0;          // постоянная прибавка к максимуму HP
    int unlockedCards = 0;
};

class Game {
public:
    void init();
    void shutdown();
    void frame();                 // один кадр: update + draw
    bool wantsExit() const { return exit_; }
    void debugForceScene(int which);   // dev: прыжок в сцену для скриншотов

private:
    // ── сцены ─────────────────────────────────────────────────────────────
    void updateMenu(float dt);      void drawMenu();
    void updateOverworld(float dt); void drawOverworld();
    void updateTablePull(float dt); void drawTablePull();
    void updateCombat(float dt);    void drawCombat();
    void updateReward(float dt);    void drawReward();
    void updateDialog(float dt);    void drawDialog();
    void updateCraft(float dt);     void drawCraft();
    void updateRunEnd(float dt);    void drawRunEnd();
    void updateCodex(float dt);     void drawCodex();

    // ── поток игры ────────────────────────────────────────────────────────
    void startRun();
    void endRun(bool died);
    void enterCombat(bool boss, int worldEntIndex);
    void afterCombat();
    void nextRegion();
    void interact(int entIndex);
    void openDialog(const LS& title, const LS& body, std::vector<DialogOption> opts, int context);
    void resolveDialog(int action);
    void openCraft();

    // ── UI-помощники ──────────────────────────────────────────────────────
    bool button(Rectangle r, const char* label, bool enabled = true, float fontSize = 18.0f);
    void panel(Rectangle r, float alpha = 0.92f);
    void drawStatusRow(const std::vector<StatusInst>& st, float x, float y, float size);
    void drawBar(Rectangle r, float frac, Color fill, Color back, const char* label);
    void drawHandArea();
    // Раскладка руки: карты ужимаются и наезжают друг на друга по мере роста.
    struct HandLayout {
        float cw = 104, ch = 146, step = 110, x0 = 0, y = 0;
        int   count = 0;
        Rectangle rect(int i) const { return Rectangle{ x0 + i * step, y, cw, ch }; }
    };
    HandLayout handLayout(int count) const;
    // Прямоугольник карты в руке; под курсором она всплывает в полный размер.
    Rectangle handCardRect(const HandLayout& L, int slot, bool hovered) const;
    float tableLineY() const { return sh_ - 176.0f; }
    void drawCodexEntry(int i, Rectangle r);
    void toast(const LS& t);

    // ── состояние ─────────────────────────────────────────────────────────
    Scene scene_ = Scene::Menu;
    bool  exit_ = false;
    float time_ = 0.0f;
    int   sw_ = 1280, sh_ = 720;

    Rng   rng_{ 0xA11CE };
    MetaProgress meta_;

    Overworld world_;
    Deck   deck_;
    Player player_;
    Inventory inv_;
    Combat combat_;
    TimingMiniGame mini_;

    int   depth_ = 1;
    int   pendingEnt_ = -1;
    bool  pendingBoss_ = false;
    bool  diedLastRun_ = false;

    // анимация «достаёт стол»
    float pullT_ = 0.0f;

    // рука/наведение
    int   hoverCard_ = -1;
    int   hoverSpot_ = -1;

    // перетаскивание карт мышью
    int     pressCard_ = -1;      // карта под зажатой кнопкой (ещё не тащим)
    int     dragCard_ = -1;       // карта в руке
    bool    dragging_ = false;    // курсор ушёл дальше порога — это перетаскивание
    Vector2 pressPos_{ 0, 0 };    // где нажали
    Vector2 dragGrab_{ 0, 0 };    // смещение курсора внутри карты
    int     dropSlot_ = -1;       // куда карта встанет в руке
    bool    dropPlay_ = false;    // курсор в зоне розыгрыша (над столом)
    float   dragLift_ = 0.0f;     // анимация подъёма

    // награды
    int   rewardHover_ = -1;
    bool  rewardTaken_ = false;

    // диалог
    LS    dlgTitle_, dlgBody_;
    std::vector<DialogOption> dlgOpts_;
    int   dlgContext_ = 0;

    // крафт
    int   craftAction_ = 0;
    int   craftTarget_ = -1;
    int   craftScroll_ = 0;
    CraftPlan plan_;
    bool  planActive_ = false;
    LS    craftResult_;

    // кодекс
    int   codexPage_ = 0;
    Scene codexReturn_ = Scene::Menu;

    // тосты
    LS    toast_;
    float toastT_ = 0.0f;

    // звук
    unsigned hoverKey_ = 0;      // по какой кнопке уже звучало наведение
    bool  thudPlayed_ = false;   // стол уже стукнул об землю

    // меню
    int   menuIndex_ = 0;
};

} // namespace shichoy
