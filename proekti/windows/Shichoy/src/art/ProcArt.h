// Procedural art / Процедурная графика — ни одного внешнего ассета.
// Всё рисуется примитивами raylib: рамки Таро, сигилы, персонаж, враги, фон.
#pragma once
#include "core/Merge.h"
#include "combat/Enemy.h"

namespace shichoy {
namespace art {

// ── Palette / Палитра (тёмное готическое Таро) ────────────────────────────
namespace pal {
    inline const Color Night     = hex(0x0B0910);
    inline const Color Deep      = hex(0x141021);
    inline const Color VoidPurp  = hex(0x241638);
    inline const Color Parchment = hex(0xE0D4B4);
    inline const Color Vellum    = hex(0xC9B78E);
    inline const Color Ink       = hex(0x1A1418);
    inline const Color Gold      = hex(0xC2A24A);
    inline const Color GoldLit   = hex(0xF0DC8C);
    inline const Color Blood     = hex(0x8C1A26);
    inline const Color Bone      = hex(0xD8CBB3);
    inline const Color Moss      = hex(0x2A3A2C);
    inline const Color Risk      = hex(0xFF5A40);
    inline const Color Fog       = hex(0x6E7A8A);
}

Color rarityColor(CardRarity r);

// ── Font / Шрифт ──────────────────────────────────────────────────────────
// Ищет системный шрифт с кириллицей (Windows Fonts). Если не нашёл —
// откатывается на встроенный шрифт raylib и переключает язык на английский.
void  init();
void  shutdown();
const Font& fontUI();
const Font& fontTitle();
bool  hasCyrillic();

// ── Text helpers / Текст ──────────────────────────────────────────────────
void  text(const char* s, float x, float y, float size, Color c);
void  textC(const char* s, float cx, float y, float size, Color c);   // по центру
void  textShadow(const char* s, float x, float y, float size, Color c);
float textWidth(const char* s, float size);
// Перенос по словам. Возвращает высоту блока.
float textWrapped(const char* s, Rectangle box, float size, Color c, float lineGap = 3.0f);

// ── Noise / Шум ───────────────────────────────────────────────────────────
float hash01(int x, int y, uint32_t seed);
float valueNoise(float x, float y, uint32_t seed);
float fbm(float x, float y, uint32_t seed, int octaves = 4);

// ── Ornaments / Орнаменты ─────────────────────────────────────────────────
void drawOrnateFrame(Rectangle r, Color fill, Color border, float thick, float corner);
void drawFiligree(Rectangle r, Color c, float t);           // витиеватые углы
void drawSigil(ElementId e, Vector2 c, float radius, Color col, float t);
void drawRuneRing(Vector2 c, float radius, int count, Color col, float t);
void drawVignette(int w, int h, float strength);
void drawStarfield(int w, int h, float t, uint32_t seed);
// Объёмное пятно и фактура — общие кирпичи светотени для всей игры.
void blob(Vector2 c, float rx, float ry, Color deep, Color lit, int layers = 6);
void grit(Vector2 c, float rx, float ry, uint32_t seed, Color col, int count);

// ── Cards / Карты ─────────────────────────────────────────────────────────
void drawCard(const Card& card, Rectangle r, float hover, bool selected, float t,
              bool showBack = false);
void drawCardBack(Rectangle r, float t);
// Сюжетное окно карты: небо, горизонт, мотив стихии и арка сверху.
void drawCardScene(Rectangle win, const Card& card, float t);

// ── Characters / Персонажи ────────────────────────────────────────────────
// Мрачный аристократ-маг: чёрный фрак, цилиндр, трость.
void drawAristocratTopDown(Vector2 pos, float scale, int facing, float walkPhase);
void drawAristocratSide(Vector2 pos, float scale, float t, float tablePull);

// Силуэт врага для боевой сцены (вид от первого лица).
// Семь обликов: у каждого свой силуэт и свои повадки.
void drawEnemyBody(Vector2 center, float scale, uint32_t seed, ElementId flavour,
                   float t, float shake, Color tint, int phase, Archetype arche);

// Карта противника — грубая, костяная, ничего общего с Таро игрока.
void drawEnemyCard(const EnemyMove& m, Rectangle r, float t, Color flavour,
                   bool active, bool faceDown, bool dead);

} // namespace art
} // namespace shichoy
