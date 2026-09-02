#include "art/ProcArt.h"
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace kv {
namespace art {

static Font s_ui   = { 0 };
static Font s_title = { 0 };
static bool s_cyr  = false;
static bool s_ready = false;

Color rarityColor(CardRarity r) {
    switch (r) {
        case CardRarity::Common:    return hex(0xA6A6AC);
        case CardRarity::Uncommon:  return hex(0x59C772);
        case CardRarity::Rare:      return hex(0x598CF2);
        case CardRarity::Epic:      return hex(0xB359E6);
        case CardRarity::Legendary: return hex(0xF2B333);
        case CardRarity::Mythic:    return hex(0xFF4D73);
        default:                    return pal::Gold;
    }
}

// ── Font ──────────────────────────────────────────────────────────────────
static std::vector<int> buildCodepoints() {
    std::vector<int> cp;
    for (int c = 32; c <= 126; ++c) cp.push_back(c);        // ASCII
    for (int c = 0x0400; c <= 0x045F; ++c) cp.push_back(c); // Кириллица
    cp.push_back(0x0490); cp.push_back(0x0491);             // Ґ ґ
    const int extra[] = { 0x00AB, 0x00BB, 0x2014, 0x2013, 0x2026, 0x2018, 0x2019,
                          0x201C, 0x201D, 0x2192, 0x2190, 0x00B0, 0x00D7, 0x2020,
                          0x25C6, 0x2605, 0x2660, 0x2663, 0x2665, 0x2666 };
    for (int c : extra) cp.push_back(c);
    return cp;
}

static Font tryLoad(const char* path, int size, std::vector<int>& cp) {
    if (!FileExists(path)) return Font{ 0 };
    Font f = LoadFontEx(path, size, cp.data(), int(cp.size()));
    if (f.texture.id == 0) return Font{ 0 };
    SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
    return f;
}

void init() {
    if (s_ready) return;
    auto cp = buildCodepoints();

    // Кандидаты — системные шрифты Windows с кириллицей (бесплатны и уже есть).
    const char* uiPaths[] = {
        "C:/Windows/Fonts/segoeui.ttf", "C:/Windows/Fonts/calibri.ttf",
        "C:/Windows/Fonts/tahoma.ttf",  "C:/Windows/Fonts/arial.ttf"
    };
    const char* titlePaths[] = {
        "C:/Windows/Fonts/georgiab.ttf", "C:/Windows/Fonts/georgia.ttf",
        "C:/Windows/Fonts/timesbd.ttf",  "C:/Windows/Fonts/times.ttf",
        "C:/Windows/Fonts/constanb.ttf", "C:/Windows/Fonts/segoeuib.ttf"
    };

    for (const char* p : uiPaths)    { s_ui    = tryLoad(p, 48, cp); if (s_ui.texture.id)    break; }
    for (const char* p : titlePaths) { s_title = tryLoad(p, 56, cp); if (s_title.texture.id) break; }

    if (s_ui.texture.id == 0)    { s_ui = GetFontDefault(); s_cyr = false; }
    else                          s_cyr = true;
    if (s_title.texture.id == 0)   s_title = s_ui;

    if (!s_cyr) g_lang = Lang::En;   // без кириллицы русский текст был бы кашей
    s_ready = true;
}

void shutdown() {
    if (!s_ready) return;
    if (s_cyr) {
        if (s_title.texture.id != s_ui.texture.id) UnloadFont(s_title);
        UnloadFont(s_ui);
    }
    s_ready = false;
}

const Font& fontUI()    { return s_ui; }
const Font& fontTitle() { return s_title; }
bool hasCyrillic()      { return s_cyr; }

// ── Text ──────────────────────────────────────────────────────────────────
void text(const char* s, float x, float y, float size, Color c) {
    DrawTextEx(s_ui, s, Vector2{ x, y }, size, size * 0.06f, c);
}
float textWidth(const char* s, float size) {
    return MeasureTextEx(s_ui, s, size, size * 0.06f).x;
}
void textC(const char* s, float cx, float y, float size, Color c) {
    text(s, cx - textWidth(s, size) * 0.5f, y, size, c);
}
void textShadow(const char* s, float x, float y, float size, Color c) {
    text(s, x + 1.5f, y + 1.5f, size, Color{ 0, 0, 0, (unsigned char)(c.a * 0.7f) });
    text(s, x, y, size, c);
}

float textWrapped(const char* s, Rectangle box, float size, Color c, float lineGap) {
    std::string str(s ? s : "");
    std::vector<std::string> words;
    size_t start = 0;
    while (start <= str.size()) {
        size_t sp = str.find(' ', start);
        if (sp == std::string::npos) { words.push_back(str.substr(start)); break; }
        words.push_back(str.substr(start, sp - start));
        start = sp + 1;
    }
    float y = box.y;
    std::string line;
    auto flush = [&]() {
        if (line.empty()) return;
        text(line.c_str(), box.x, y, size, c);
        y += size + lineGap;
        line.clear();
    };
    for (const auto& w : words) {
        if (w.empty()) continue;
        std::string cand = line.empty() ? w : line + " " + w;
        if (textWidth(cand.c_str(), size) > box.width && !line.empty()) flush();
        line = line.empty() ? w : line + " " + w;
        if (y + size > box.y + box.height) break;
    }
    flush();
    return y - box.y;
}

// ── Noise ─────────────────────────────────────────────────────────────────
float hash01(int x, int y, uint32_t seed) {
    uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= (h >> 16);
    return float(h & 0xFFFFFF) / float(0xFFFFFF);
}
float valueNoise(float x, float y, uint32_t seed) {
    int xi = int(std::floor(x)), yi = int(std::floor(y));
    float xf = x - xi, yf = y - yi;
    float u = xf * xf * (3 - 2 * xf), v = yf * yf * (3 - 2 * yf);
    float a = hash01(xi, yi, seed),     b = hash01(xi + 1, yi, seed);
    float c = hash01(xi, yi + 1, seed), d = hash01(xi + 1, yi + 1, seed);
    return (a * (1 - u) + b * u) * (1 - v) + (c * (1 - u) + d * u) * v;
}
float fbm(float x, float y, uint32_t seed, int octaves) {
    float sum = 0, amp = 0.5f, freq = 1.0f, norm = 0;
    for (int i = 0; i < octaves; ++i) {
        sum  += valueNoise(x * freq, y * freq, seed + uint32_t(i) * 7919u) * amp;
        norm += amp;
        amp  *= 0.5f; freq *= 2.0f;
    }
    return norm > 0 ? sum / norm : 0;
}

// ── Ornaments ─────────────────────────────────────────────────────────────
void drawOrnateFrame(Rectangle r, Color fill, Color border, float thick, float corner) {
    float round = clampf(corner / (r.width < r.height ? r.width : r.height) * 2.0f, 0.0f, 1.0f);
    DrawRectangleRounded(r, round, 8, fill);
    DrawRectangleRoundedLinesEx(r, round, 8, thick, border);
    Rectangle inner = { r.x + thick + 3, r.y + thick + 3, r.width - (thick + 3) * 2, r.height - (thick + 3) * 2 };
    if (inner.width > 4 && inner.height > 4)
        DrawRectangleRoundedLinesEx(inner, round * 0.9f, 8, 1.0f, withA(border, 120));
}

void drawFiligree(Rectangle r, Color c, float t) {
    const float s = 12.0f;
    float pulse = 0.75f + 0.25f * std::sin(t * 1.6f);
    Color cc = withA(c, (unsigned char)(c.a * pulse));
    struct P { float x, y, dx, dy; };
    P corners[4] = {
        { r.x + 4,             r.y + 4,              1,  1 },
        { r.x + r.width - 4,   r.y + 4,             -1,  1 },
        { r.x + 4,             r.y + r.height - 4,   1, -1 },
        { r.x + r.width - 4,   r.y + r.height - 4,  -1, -1 },
    };
    for (const P& p : corners) {
        DrawLineEx(Vector2{ p.x, p.y }, Vector2{ p.x + p.dx * s, p.y }, 1.4f, cc);
        DrawLineEx(Vector2{ p.x, p.y }, Vector2{ p.x, p.y + p.dy * s }, 1.4f, cc);
        // мягкий завиток внутрь — намёк на витиеватую рамку Таро
        DrawCircleSectorLines(Vector2{ p.x + p.dx * s * 0.75f, p.y + p.dy * s * 0.75f },
                              s * 0.30f, 0, 210, 6, withA(cc, 70));
        DrawCircleV(Vector2{ p.x + p.dx * 2.5f, p.y + p.dy * 2.5f }, 1.2f, withA(cc, 200));
    }
}

void drawRuneRing(Vector2 c, float radius, int count, Color col, float t) {
    for (int i = 0; i < count; ++i) {
        float a = t * 0.35f + float(i) / count * 2 * PI;
        Vector2 p = { c.x + std::cos(a) * radius, c.y + std::sin(a) * radius };
        float w = 2.0f + 1.5f * std::sin(t * 2.0f + i);
        DrawLineEx(Vector2{ p.x - w, p.y - w }, Vector2{ p.x + w, p.y + w }, 1.2f, col);
        DrawLineEx(Vector2{ p.x - w, p.y + w }, Vector2{ p.x + w, p.y - w }, 1.2f, withA(col, 140));
    }
    DrawCircleLines(int(c.x), int(c.y), radius, withA(col, 70));
}

// Каждая стихия получает узнаваемый геометрический сигил — без спрайтов.
void drawSigil(ElementId e, Vector2 c, float radius, Color col, float t) {
    int idx = eidx(e);
    if (idx <= 0) { DrawCircleLines(int(c.x), int(c.y), radius, col); return; }
    int sides = 3 + (idx % 6);
    float rot = t * 8.0f + idx * 23.0f;

    DrawPolyLinesEx(c, sides, radius, rot, 2.0f, col);
    DrawPolyLinesEx(c, sides, radius * 0.62f, -rot * 0.7f, 1.2f, withA(col, 160));
    DrawCircleLines(int(c.x), int(c.y), radius * 0.30f, withA(col, 200));

    int rays = 4 + (idx % 5);
    for (int i = 0; i < rays; ++i) {
        float a = float(i) / rays * 2 * PI + rot * DEG2RAD;
        float r0 = radius * 0.34f, r1 = radius * (0.78f + 0.18f * ((idx + i) % 2));
        DrawLineEx(Vector2{ c.x + std::cos(a) * r0, c.y + std::sin(a) * r0 },
                   Vector2{ c.x + std::cos(a) * r1, c.y + std::sin(a) * r1 },
                   1.4f, withA(col, 190));
    }
    // Внутренний знак: пересечения по «алхимическому» правилу
    if (idx % 3 == 0) DrawLineEx(Vector2{ c.x - radius * 0.4f, c.y }, Vector2{ c.x + radius * 0.4f, c.y }, 1.6f, col);
    if (idx % 3 == 1) DrawTriangleLines(Vector2{ c.x, c.y - radius * 0.42f },
                                        Vector2{ c.x - radius * 0.36f, c.y + radius * 0.26f },
                                        Vector2{ c.x + radius * 0.36f, c.y + radius * 0.26f }, col);
    if (idx % 3 == 2) DrawCircleLines(int(c.x), int(c.y), radius * 0.46f, col);
}

void drawVignette(int w, int h, float strength) {
    int steps = 40;
    for (int i = 0; i < steps; ++i) {
        float f = 1.0f - float(i) / steps;
        unsigned char a = (unsigned char)(strength * 70.0f * f * f);
        DrawRectangleLinesEx(Rectangle{ float(i), float(i), float(w - i * 2), float(h - i * 2) },
                             1.0f, Color{ 0, 0, 0, a });
    }
    DrawRectangleGradientV(0, 0, w, h / 5, Color{ 0, 0, 0, (unsigned char)(90 * strength) }, BLANK);
    DrawRectangleGradientV(0, h - h / 5, w, h / 5, BLANK, Color{ 0, 0, 0, (unsigned char)(110 * strength) });
}

void drawStarfield(int w, int h, float t, uint32_t seed) {
    for (int i = 0; i < 90; ++i) {
        float x = hash01(i, 1, seed) * w;
        float y = hash01(i, 2, seed) * h * 0.7f;
        float tw = 0.5f + 0.5f * std::sin(t * (0.5f + hash01(i, 3, seed)) + i);
        DrawCircleV(Vector2{ x, y }, 0.8f + tw, withA(pal::Bone, (unsigned char)(40 + 90 * tw)));
    }
}

// ── Cards ─────────────────────────────────────────────────────────────────
void drawCardBack(Rectangle r, float t) {
    drawOrnateFrame(r, pal::Deep, pal::Gold, 2.0f, 10.0f);
    Vector2 c = { r.x + r.width * 0.5f, r.y + r.height * 0.5f };
    drawRuneRing(c, r.width * 0.28f, 8, withA(pal::Gold, 150), t);
    DrawPolyLinesEx(c, 6, r.width * 0.18f, t * 12.0f, 1.5f, withA(pal::GoldLit, 180));
    drawFiligree(r, withA(pal::Gold, 200), t);
}

static void drawRarityGems(Rectangle r, CardRarity rar, Color col) {
    int n = static_cast<int>(rar) + 1;
    float y = r.y + r.height - 9.0f;
    float step = 7.0f;
    float x = r.x + r.width * 0.5f - (n - 1) * step * 0.5f;
    for (int i = 0; i < n; ++i) {
        Vector2 p = { x + i * step, y };
        DrawPolyLinesEx(p, 4, 3.2f, 45.0f, 1.4f, col);
    }
}

void drawCard(const Card& card, Rectangle r, float hover, bool selected, float t, bool showBack) {
    if (showBack) { drawCardBack(r, t); return; }

    Color accent = card.tint();
    Color rare   = rarityColor(card.rarity);
    Color paper  = lerpC(pal::Vellum, accent, 0.14f);
    paper = lerpC(paper, pal::Night, 0.55f - hover * 0.15f);

    // Тень + подсветка выбора
    DrawRectangleRounded(Rectangle{ r.x + 3, r.y + 5, r.width, r.height }, 0.10f, 8, Color{ 0, 0, 0, 110 });
    if (selected)
        DrawRectangleRoundedLinesEx(Rectangle{ r.x - 4, r.y - 4, r.width + 8, r.height + 8 },
                                    0.12f, 8, 2.0f, withA(pal::GoldLit, 220));

    drawOrnateFrame(r, paper, lerpC(pal::Gold, rare, 0.5f), 2.0f, 9.0f);

    // Шапка: имя
    Rectangle head = { r.x + 7, r.y + 6, r.width - 14, 26 };
    DrawRectangleRounded(head, 0.4f, 6, withA(scaleC(accent, 0.55f), 190));
    DrawRectangleRoundedLinesEx(head, 0.4f, 6, 1.0f, withA(pal::Gold, 160));
    {
        std::string nm = card.name.str();
        float fs = 13.0f;
        while (fs > 8.0f && textWidth(nm.c_str(), fs) > head.width - 8) fs -= 0.5f;
        textC(nm.c_str(), head.x + head.width * 0.5f, head.y + (head.height - fs) * 0.5f, fs, pal::Parchment);
    }

    // Иллюстрация: сигил стихии / знак типа
    Vector2 art = { r.x + r.width * 0.5f, r.y + r.height * 0.44f };
    float ar = r.width * 0.28f;
    DrawCircleGradient(int(art.x), int(art.y), ar * 1.7f, withA(accent, 55), BLANK);
    if (card.element != ElementId::None) {
        drawSigil(card.element, art, ar, lerpC(accent, WHITE, 0.35f), t);
    } else if (card.type == CardType::Spell) {
        drawRuneRing(art, ar * 0.9f, 6, withA(pal::GoldLit, 200), t);
        DrawPolyLinesEx(art, 5, ar * 0.5f, t * 10.0f, 1.8f, pal::GoldLit);
    } else {
        DrawRectangleRoundedLinesEx(Rectangle{ art.x - ar * 0.7f, art.y - ar * 0.7f, ar * 1.4f, ar * 1.4f },
                                    0.25f, 6, 1.8f, withA(pal::Bone, 210));
        DrawLineEx(Vector2{ art.x - ar * 0.45f, art.y }, Vector2{ art.x + ar * 0.45f, art.y }, 1.6f, pal::Bone);
    }

    // Тип карты
    const char* typeStr =
        card.type == CardType::Element ? (g_lang == Lang::Ru ? "СТИХИЯ" : "ELEMENT")
      : card.type == CardType::Spell   ? (g_lang == Lang::Ru ? "ЗАКЛИНАНИЕ" : "SPELL")
                                       : (g_lang == Lang::Ru ? "ЭФФЕКТ" : "EFFECT");
    textC(typeStr, r.x + r.width * 0.5f, r.y + r.height * 0.60f, 9.0f, withA(pal::Gold, 220));

    // Текст правил
    Rectangle body = { r.x + 9, r.y + r.height * 0.66f, r.width - 18, r.height * 0.24f };
    textWrapped(card.text.c_str(), body, 10.0f, withA(pal::Parchment, 235), 2.0f);

    // Цена в слотах хода
    for (int i = 0; i < card.cost; ++i) {
        Vector2 p = { r.x + 12.0f + i * 9.0f, r.y + r.height - 10.0f };
        DrawCircleV(p, 3.2f, withA(pal::GoldLit, 230));
        DrawCircleLines(int(p.x), int(p.y), 3.2f, pal::Ink);
    }
    // Уровень апгрейда
    if (card.level > 0) {
        std::string lv = "+" + std::to_string(card.level);
        text(lv.c_str(), r.x + r.width - 22.0f, r.y + r.height - 17.0f, 11.0f, pal::GoldLit);
    }
    drawRarityGems(r, card.rarity, rare);

    if (has(card.flags, CardFlags::Corrupted))
        DrawRectangleRoundedLinesEx(r, 0.10f, 8, 1.0f, withA(pal::Risk, (unsigned char)(120 + 80 * std::sin(t * 4))));
    if (has(card.flags, CardFlags::Mutated))
        DrawCircleV(Vector2{ r.x + 10, r.y + 10 }, 3.0f, pal::Risk);
    if (has(card.flags, CardFlags::Wild))
        DrawCircleV(Vector2{ r.x + r.width - 10, r.y + 10 }, 3.0f, hex(0x9D4EDD));

    drawFiligree(r, withA(pal::Gold, 190), t);
}

// ── Characters ────────────────────────────────────────────────────────────
void drawAristocratTopDown(Vector2 pos, float s, int facing, float walkPhase) {
    float bob = std::sin(walkPhase) * 1.2f * s;
    Color coat = hex(0x14111C);
    Color coatLit = hex(0x241F32);
    Color skin = hex(0xC9AE96);

    // Тень
    DrawEllipse(int(pos.x), int(pos.y + 10 * s), 9 * s, 4 * s, Color{ 0, 0, 0, 110 });

    float y = pos.y + bob;
    // Фалды фрака
    DrawTriangle(Vector2{ pos.x - 7 * s, y + 2 * s },
                 Vector2{ pos.x, y + 12 * s },
                 Vector2{ pos.x + 7 * s, y + 2 * s }, coat);
    // Корпус
    DrawEllipse(int(pos.x), int(y), int(7 * s), int(9 * s), coat);
    DrawEllipseLines(int(pos.x), int(y), 7 * s, 9 * s, withA(coatLit, 180));
    // Манишка
    DrawTriangle(Vector2{ pos.x - 2.2f * s, y - 5 * s },
                 Vector2{ pos.x, y + 3 * s },
                 Vector2{ pos.x + 2.2f * s, y - 5 * s }, hex(0xD8CBB3));
    // Голова + цилиндр
    float hx = pos.x + (facing == 1 ? 1.2f * s : facing == 3 ? -1.2f * s : 0);
    float hy = y - 8.5f * s;
    DrawCircleV(Vector2{ hx, hy }, 4.2f * s, skin);
    // лицо в тени полей — читаются только глаза
    DrawCircleV(Vector2{ hx, hy - 0.8f * s }, 4.2f * s, Color{ 8, 6, 12, 170 });
    if (facing != 0) {
        DrawCircleV(Vector2{ hx - 1.6f * s, hy - 0.4f * s }, 0.7f * s, hex(0xFFE9A8));
        DrawCircleV(Vector2{ hx + 1.6f * s, hy - 0.4f * s }, 0.7f * s, hex(0xFFE9A8));
    }
    DrawEllipse(int(hx), int(hy - 2.0f * s), int(7.2f * s), int(2.0f * s), hex(0x0C0A12));  // поля
    DrawRectangle(int(hx - 4.0f * s), int(hy - 10.0f * s), int(8.0f * s), int(8.0f * s), hex(0x0C0A12));
    DrawRectangle(int(hx - 4.0f * s), int(hy - 4.4f * s), int(8.0f * s), int(1.4f * s), pal::Blood);
    // Трость
    float cx = pos.x + (facing == 3 ? -8.5f : 8.5f) * s;
    DrawLineEx(Vector2{ cx, y - 3 * s }, Vector2{ cx, y + 11 * s }, 1.6f * s, hex(0x3A2A20));
    DrawCircleV(Vector2{ cx, y - 4 * s }, 1.8f * s, pal::Gold);
}

void drawAristocratSide(Vector2 pos, float s, float t, float tablePull) {
    Color coat = hex(0x14111C);
    DrawEllipse(int(pos.x), int(pos.y + 26 * s), 16 * s, 5 * s, Color{ 0, 0, 0, 120 });
    // Ноги
    DrawRectangle(int(pos.x - 6 * s), int(pos.y + 6 * s), int(4 * s), int(20 * s), coat);
    DrawRectangle(int(pos.x + 2 * s), int(pos.y + 6 * s), int(4 * s), int(20 * s), coat);
    // Фрак
    DrawTriangle(Vector2{ pos.x - 11 * s, pos.y + 4 * s },
                 Vector2{ pos.x - 3 * s,  pos.y + 24 * s },
                 Vector2{ pos.x + 2 * s,  pos.y + 4 * s }, coat);
    DrawRectangleRounded(Rectangle{ pos.x - 10 * s, pos.y - 14 * s, 20 * s, 22 * s }, 0.35f, 6, coat);
    DrawTriangle(Vector2{ pos.x - 3 * s, pos.y - 12 * s },
                 Vector2{ pos.x,         pos.y + 2 * s },
                 Vector2{ pos.x + 3 * s, pos.y - 12 * s }, hex(0xD8CBB3));
    // Голова и цилиндр
    float hy = pos.y - 20 * s;
    DrawCircleV(Vector2{ pos.x, hy }, 6 * s, hex(0xC9AE96));
    // тень от полей цилиндра: видно только челюсть и два блика глаз
    DrawRectangleRounded(Rectangle{ pos.x - 6 * s, hy - 6 * s, 12 * s, 7.5f * s }, 0.5f, 6,
                         Color{ 8, 6, 12, 200 });
    DrawCircleV(Vector2{ pos.x - 2.4f * s, hy - 1.6f * s }, 0.9f * s, hex(0xFFE9A8));
    DrawCircleV(Vector2{ pos.x + 2.4f * s, hy - 1.6f * s }, 0.9f * s, hex(0xFFE9A8));
    DrawLineEx(Vector2{ pos.x - 2.0f * s, hy + 3.2f * s }, Vector2{ pos.x + 2.0f * s, hy + 3.2f * s },
               1.0f * s, hex(0x8A6A58));
    DrawEllipse(int(pos.x), int(hy - 5 * s), int(11 * s), int(2.4f * s), hex(0x0C0A12));
    DrawRectangle(int(pos.x - 6 * s), int(hy - 16 * s), int(12 * s), int(11 * s), hex(0x0C0A12));
    DrawRectangle(int(pos.x - 6 * s), int(hy - 8 * s), int(12 * s), int(2 * s), pal::Blood);

    // Рука тянет стол из-под фрака / The table being pulled out
    if (tablePull > 0.001f) {
        float k = clampf(tablePull, 0.0f, 1.0f);
        float tx = pos.x + 14 * s + k * 26 * s;
        float ty = pos.y - 2 * s + (1.0f - k) * 16 * s;
        DrawLineEx(Vector2{ pos.x + 8 * s, pos.y - 6 * s }, Vector2{ tx - 6 * s, ty }, 3.0f * s, hex(0x1B1626));
        DrawRectangleRounded(Rectangle{ tx - 16 * s, ty - 4 * s, 34 * s, 5 * s }, 0.6f, 6,
                             lerpC(hex(0x4A3420), pal::Gold, 0.2f));
        DrawRectangle(int(tx - 12 * s), int(ty + 1 * s), int(2.4f * s), int(14 * s * k), hex(0x33231A));
        DrawRectangle(int(tx + 14 * s), int(ty + 1 * s), int(2.4f * s), int(14 * s * k), hex(0x33231A));
        // Зелёное сукно и карты на нём
        DrawRectangleRounded(Rectangle{ tx - 15 * s, ty - 6 * s, 32 * s, 3 * s }, 0.8f, 6, hex(0x1E3A2A));
        for (int i = 0; i < 3; ++i) {
            float cx = tx - 8 * s + i * 7 * s;
            DrawRectangleRounded(Rectangle{ cx, ty - 12 * s * k, 5 * s, 8 * s * k }, 0.3f, 4,
                                 withA(pal::Vellum, (unsigned char)(255 * k)));
        }
    }
    (void)t;
}

// ── Enemy / Враг ──────────────────────────────────────────────────────────
void drawEnemyBody(Vector2 center, float scale, uint32_t seed, ElementId flavour,
                   float t, float shake, Color tint, int phase) {
    float sx = center.x + std::sin(t * 23.0f) * shake;
    float sy = center.y + std::cos(t * 19.0f) * shake;
    Color body = lerpC(hex(0x241C33), tint, 0.34f + phase * 0.09f);
    Color bodyDeep = scaleC(body, 0.45f);
    Color edge = lerpC(elem(flavour).primary, WHITE, 0.28f);

    float breathe = std::sin(t * 1.3f) * 3.0f * scale;
    float w = 150 * scale, h = 200 * scale;

    // Аура
    DrawCircleGradient(int(sx), int(sy), w * 1.6f, withA(edge, 40), BLANK);

    // Плащ/туловище — рваный многоугольник, детерминированный по seed
    const int N = 14;
    Vector2 pts[N];
    for (int i = 0; i < N; ++i) {
        float a = float(i) / N * 2 * PI;
        float rad = 1.0f + (hash01(i, int(seed % 1000), seed) - 0.5f) * 0.45f;
        rad += std::sin(t * 1.7f + i) * 0.04f;
        pts[i] = Vector2{ sx + std::cos(a) * w * 0.5f * rad,
                          sy + std::sin(a) * h * 0.5f * rad + breathe };
    }
    for (int i = 0; i < N; ++i)
        DrawTriangle(Vector2{ sx, sy + breathe }, pts[i], pts[(i + 1) % N], body);
    // объём: тёмный низ плаща
    for (int i = 0; i < N; ++i) {
        if (pts[i].y < sy + breathe) continue;
        DrawTriangle(Vector2{ sx, sy + breathe }, pts[i], pts[(i + 1) % N], withA(bodyDeep, 190));
    }
    for (int i = 0; i < N; ++i)
        DrawLineEx(pts[i], pts[(i + 1) % N], 2.4f, withA(edge, 200));

    // «Рёбра» / внутренняя структура
    for (int i = 0; i < 5 + phase; ++i) {
        float yy = sy - h * 0.3f + i * h * 0.11f + breathe;
        float ww = w * (0.42f - i * 0.03f);
        DrawLineEx(Vector2{ sx - ww, yy }, Vector2{ sx + ww, yy }, 1.6f, withA(pal::Bone, 110));
    }

    // Голова — череп в маске
    float hy = sy - h * 0.46f + breathe;
    DrawCircleV(Vector2{ sx, hy }, 26 * scale, lerpC(pal::Bone, body, 0.25f));
    DrawCircleV(Vector2{ sx - 9 * scale, hy - 3 * scale }, 5.5f * scale, hex(0x07060A));
    DrawCircleV(Vector2{ sx + 9 * scale, hy - 3 * scale }, 5.5f * scale, hex(0x07060A));
    float glow = 0.5f + 0.5f * std::sin(t * 3.0f);
    DrawCircleV(Vector2{ sx - 9 * scale, hy - 3 * scale }, 2.6f * scale, withA(edge, (unsigned char)(150 + 100 * glow)));
    DrawCircleV(Vector2{ sx + 9 * scale, hy - 3 * scale }, 2.6f * scale, withA(edge, (unsigned char)(150 + 100 * glow)));
    for (int i = 0; i < 5; ++i)
        DrawLineEx(Vector2{ sx - 10 * scale + i * 5 * scale, hy + 9 * scale },
                   Vector2{ sx - 10 * scale + i * 5 * scale, hy + 16 * scale }, 1.4f, hex(0x07060A));

    // Рога/корона, растут с фазой
    for (int i = 0; i <= phase; ++i) {
        float dir = (i % 2 == 0) ? -1.0f : 1.0f;
        DrawLineEx(Vector2{ sx + dir * 18 * scale, hy - 14 * scale },
                   Vector2{ sx + dir * (30 + i * 5) * scale, hy - (38 + i * 8) * scale },
                   3.0f * scale, lerpC(pal::Bone, edge, 0.4f));
    }
    drawRuneRing(Vector2{ sx, sy + breathe }, w * 0.78f, 6 + phase * 2, withA(edge, 110), t);
}

} // namespace art
} // namespace kv
