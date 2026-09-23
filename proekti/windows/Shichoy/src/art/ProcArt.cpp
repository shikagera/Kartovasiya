#include "art/ProcArt.h"
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace shichoy {
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
    const int extra[] = { 0x00AB, 0x00BB, 0x00B7, 0x2022, 0x2014, 0x2013, 0x2026, 0x2018, 0x2019,
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

    // Уровень слияния и цепочка стихий, из которых карта собрана.
    if (card.tier != MergeTier::Base) {
        const char* lab = tierLabel(card.tier);
        bool ult = (card.tier == MergeTier::Ultima);
        Color tc = ult ? hex(0xFF4D73) : pal::GoldLit;
        float lw = textWidth(lab, 11.0f) + 10.0f;
        Rectangle badge{ r.x + 5, r.y + r.height * 0.60f - 2.0f, lw, 15.0f };
        DrawRectangleRounded(badge, 0.45f, 5, withA(scaleC(tc, 0.35f), 220));
        DrawRectangleRoundedLinesEx(badge, 0.45f, 5, 1.0f, tc);
        textC(lab, badge.x + badge.width * 0.5f, badge.y + 2.0f, 11.0f, tc);

        float dx = r.x + 6.0f;
        for (size_t i = 0; i < card.chain.size() && dx < r.x + r.width - 8.0f; ++i) {
            DrawCircleV(Vector2{ dx + 3.0f, r.y + r.height * 0.60f + 22.0f }, 3.0f,
                        elem(card.chain[i]).primary);
            dx += 8.0f;
        }
        if (ult) {
            float pulse = 0.5f + 0.5f * std::sin(t * 3.5f);
            DrawRectangleRoundedLinesEx(r, 0.10f, 8, 2.2f,
                                        withA(tc, (unsigned char)(140 + 110 * pulse)));
            drawRuneRing(art, ar * 1.5f, 10, withA(tc, 120), t * 1.5f);
        }
    }

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


// ── Enemy bodies / Облики противников ─────────────────────────────────────
// Семь силуэтов, собранных из примитивов. Каждый должен читаться с первого
// взгляда, чтобы по фигуре было ясно, чего ждать.

static void bodyReaper(Vector2 c, float s, uint32_t seed, float t, Color body,
                       Color edge, int phase, float breathe) {
    const int N = 14;
    Vector2 pts[N];
    float w = 150 * s, h = 200 * s;
    for (int i = 0; i < N; ++i) {
        float a = float(i) / N * 2 * PI;
        float rad = 1.0f + (hash01(i, int(seed % 1000), seed) - 0.5f) * 0.45f
                  + std::sin(t * 1.7f + i) * 0.04f;
        pts[i] = Vector2{ c.x + std::cos(a) * w * 0.5f * rad,
                          c.y + std::sin(a) * h * 0.5f * rad + breathe };
    }
    for (int i = 0; i < N; ++i)
        DrawTriangle(Vector2{ c.x, c.y + breathe }, pts[i], pts[(i + 1) % N], body);
    for (int i = 0; i < N; ++i)
        DrawLineEx(pts[i], pts[(i + 1) % N], 2.4f, withA(edge, 200));
    for (int i = 0; i < 5 + phase; ++i) {
        float yy = c.y - h * 0.3f + i * h * 0.11f + breathe;
        float ww = w * (0.42f - i * 0.03f);
        DrawLineEx(Vector2{ c.x - ww, yy }, Vector2{ c.x + ww, yy }, 1.6f, withA(pal::Bone, 110));
    }
    float hy = c.y - h * 0.46f + breathe;
    DrawCircleV(Vector2{ c.x, hy }, 26 * s, lerpC(pal::Bone, body, 0.25f));
    DrawCircleV(Vector2{ c.x - 9 * s, hy - 3 * s }, 5.5f * s, hex(0x07060A));
    DrawCircleV(Vector2{ c.x + 9 * s, hy - 3 * s }, 5.5f * s, hex(0x07060A));
    float glow = 0.5f + 0.5f * std::sin(t * 3.0f);
    unsigned char ga = (unsigned char)(150 + 100 * glow);
    DrawCircleV(Vector2{ c.x - 9 * s, hy - 3 * s }, 2.6f * s, withA(edge, ga));
    DrawCircleV(Vector2{ c.x + 9 * s, hy - 3 * s }, 2.6f * s, withA(edge, ga));
    for (int i = 0; i < 5; ++i)
        DrawLineEx(Vector2{ c.x - 10 * s + i * 5 * s, hy + 9 * s },
                   Vector2{ c.x - 10 * s + i * 5 * s, hy + 16 * s }, 1.4f, hex(0x07060A));
    for (int i = 0; i <= phase; ++i) {
        float dir = (i % 2 == 0) ? -1.0f : 1.0f;
        DrawLineEx(Vector2{ c.x + dir * 18 * s, hy - 14 * s },
                   Vector2{ c.x + dir * (30 + i * 5) * s, hy - (38 + i * 8) * s },
                   3.0f * s, lerpC(pal::Bone, edge, 0.4f));
    }
}

static void bodyCandelabrum(Vector2 c, float s, uint32_t seed, float t, Color body,
                            Color edge, int phase, float breathe) {
    float h = 210 * s;
    // Основание и стойка
    DrawEllipse(int(c.x), int(c.y + h * 0.46f), int(46 * s), int(12 * s), body);
    DrawRectangleRounded(Rectangle{ c.x - 7 * s, c.y - h * 0.34f, 14 * s, h * 0.80f }, 0.5f, 6, body);
    DrawRectangleRoundedLinesEx(Rectangle{ c.x - 7 * s, c.y - h * 0.34f, 14 * s, h * 0.80f },
                                0.5f, 6, 1.6f, withA(edge, 200));
    // Рожки со свечами
    int arms = 3 + phase;
    for (int i = 0; i < arms; ++i) {
        float k = (i % 2 == 0) ? -1.0f : 1.0f;
        float lvl = c.y - h * 0.18f + (i / 2) * 34 * s;
        float len = (34 + (i / 2) * 8) * s;
        DrawLineEx(Vector2{ c.x, lvl }, Vector2{ c.x + k * len, lvl - 26 * s }, 2.4f, edge);
        Vector2 tip{ c.x + k * len, lvl - 30 * s };
        DrawRectangle(int(tip.x - 4 * s), int(tip.y), int(8 * s), int(16 * s), pal::Vellum);
        float fl = 0.7f + 0.3f * std::sin(t * 9.0f + i * 2.1f);
        DrawCircleGradient(int(tip.x), int(tip.y - 6 * s), 18 * s * fl, withA(hex(0xFFBA08), 120), BLANK);
        DrawTriangle(Vector2{ tip.x, tip.y - 14 * s * fl },
                     Vector2{ tip.x - 4 * s, tip.y },
                     Vector2{ tip.x + 4 * s, tip.y }, hex(0xFFD60A));
        // Восковые потёки
        DrawRectangle(int(tip.x - 2 * s), int(tip.y + 14 * s), int(3 * s), int(10 * s * fl), pal::Vellum);
    }
    // Пламя вместо головы
    float top = c.y - h * 0.38f + breathe;
    float big = 0.8f + 0.2f * std::sin(t * 5.0f);
    DrawCircleGradient(int(c.x), int(top), 40 * s * big, withA(edge, 110), BLANK);
    DrawTriangle(Vector2{ c.x, top - 34 * s * big },
                 Vector2{ c.x - 13 * s, top + 8 * s },
                 Vector2{ c.x + 13 * s, top + 8 * s }, hex(0xFFBA08));
    DrawTriangle(Vector2{ c.x, top - 20 * s * big },
                 Vector2{ c.x - 6 * s, top + 6 * s },
                 Vector2{ c.x + 6 * s, top + 6 * s }, hex(0xFFF3B0));
    DrawCircleV(Vector2{ c.x - 5 * s, top - 6 * s }, 2.2f * s, hex(0x07060A));
    DrawCircleV(Vector2{ c.x + 5 * s, top - 6 * s }, 2.2f * s, hex(0x07060A));
    (void)seed;
}

static void bodyPortrait(Vector2 c, float s, uint32_t seed, float t, Color body,
                         Color edge, int phase, float breathe) {
    float w = 170 * s, h = 220 * s;
    Rectangle frame{ c.x - w * 0.5f, c.y - h * 0.5f, w, h };
    // Холст
    DrawRectangleRec(frame, hex(0x0D0A12));
    DrawRectangleGradientV(int(frame.x), int(frame.y), int(frame.width), int(frame.height),
                           withA(body, 200), hex(0x07060A));
    // Тяжёлая золочёная рама
    for (int i = 0; i < 3; ++i) {
        Rectangle f{ frame.x - i * 5 * s, frame.y - i * 5 * s,
                     frame.width + i * 10 * s, frame.height + i * 10 * s };
        DrawRectangleLinesEx(f, 4.0f * s, i == 1 ? pal::Gold : withA(pal::Gold, 160));
    }
    for (int i = 0; i < 4; ++i) {   // угловые завитки
        float cx = frame.x + (i % 2) * frame.width;
        float cy = frame.y + (i / 2) * frame.height;
        DrawCircleLines(int(cx), int(cy), 9 * s, pal::GoldLit);
        DrawCircleV(Vector2{ cx, cy }, 3.5f * s, pal::Gold);
    }
    // Фигура лезет наружу
    float lean = (6.0f + phase * 4.0f) * s * (0.6f + 0.4f * std::sin(t * 1.4f));
    float hy = c.y - h * 0.16f + breathe;
    DrawEllipse(int(c.x), int(hy + 46 * s), int(40 * s), int(52 * s), scaleC(body, 1.25f));
    DrawCircleV(Vector2{ c.x, hy }, 24 * s, lerpC(pal::Vellum, body, 0.35f));
    DrawEllipse(int(c.x - 8 * s), int(hy - 2 * s), int(6 * s), int(4 * s), hex(0x07060A));
    DrawEllipse(int(c.x + 8 * s), int(hy - 2 * s), int(6 * s), int(4 * s), hex(0x07060A));
    DrawEllipse(int(c.x), int(hy + 14 * s), int(5 * s), int(8 * s), hex(0x1A0A0E));   // раскрытый рот
    // Кисти рук на краю рамы
    for (int k = -1; k <= 1; k += 2) {
        float hx = c.x + k * (w * 0.5f - 4 * s);
        DrawEllipse(int(hx), int(c.y + 10 * s - lean), int(11 * s), int(8 * s),
                    lerpC(pal::Vellum, body, 0.3f));
        for (int f = 0; f < 4; ++f)
            DrawLineEx(Vector2{ hx, c.y + 6 * s - lean },
                       Vector2{ hx - k * (4 + f * 3) * s, c.y + 22 * s - lean },
                       2.0f * s, lerpC(pal::Vellum, body, 0.45f));
    }
    DrawRectangleLinesEx(frame, 2.0f, withA(edge, 150));
    (void)seed;
}

static void bodyHound(Vector2 c, float s, uint32_t seed, float t, Color body,
                      Color edge, int phase, float breathe) {
    float step = std::sin(t * 4.0f) * 4.0f * s;
    float bx = c.x, by = c.y + 20 * s + breathe;
    DrawEllipse(int(bx), int(by + 52 * s), int(70 * s), int(11 * s), Color{ 0, 0, 0, 120 });
    // Лапы
    for (int i = 0; i < 4; ++i) {
        float lx = bx - 46 * s + (i % 2) * 92 * s + (i / 2) * 12 * s;
        float sw = (i % 2 == 0) ? step : -step;
        DrawLineEx(Vector2{ lx, by + 10 * s }, Vector2{ lx + sw, by + 50 * s }, 6.5f * s, body);
        DrawCircleV(Vector2{ lx + sw, by + 50 * s }, 5.0f * s, scaleC(body, 0.7f));
    }
    // Туловище и хребет
    DrawEllipse(int(bx), int(by), int(66 * s), int(30 * s), body);
    DrawEllipseLines(int(bx), int(by), 66 * s, 30 * s, withA(edge, 190));
    for (int i = 0; i < 6 + phase; ++i)
        DrawLineEx(Vector2{ bx - 50 * s + i * 16 * s, by - 26 * s },
                   Vector2{ bx - 50 * s + i * 16 * s, by - 36 * s - (i % 2) * 5 * s },
                   2.2f * s, withA(pal::Bone, 200));
    // Хвост
    DrawLineEx(Vector2{ bx - 62 * s, by - 6 * s },
               Vector2{ bx - 96 * s, by - 26 * s + step * 2 }, 4.0f * s, body);
    // Шея и вытянутый череп
    float hx = bx + 74 * s, hy = by - 24 * s;
    DrawLineEx(Vector2{ bx + 48 * s, by - 12 * s }, Vector2{ hx - 8 * s, hy }, 15.0f * s, body);
    DrawEllipse(int(hx), int(hy), int(30 * s), int(15 * s), lerpC(pal::Bone, body, 0.3f));
    DrawCircleV(Vector2{ hx - 4 * s, hy - 5 * s }, 4.0f * s, hex(0x07060A));
    float glow = 0.5f + 0.5f * std::sin(t * 6.0f);
    DrawCircleV(Vector2{ hx - 4 * s, hy - 5 * s }, 2.0f * s,
                withA(edge, (unsigned char)(160 + 90 * glow)));
    // Пасть
    for (int i = 0; i < 6; ++i) {
        float tx = hx + 6 * s + i * 4 * s;
        DrawTriangle(Vector2{ tx, hy + 6 * s }, Vector2{ tx + 3 * s, hy + 6 * s },
                     Vector2{ tx + 1.5f * s, hy + 14 * s }, pal::Bone);
    }
    // Уши
    DrawTriangle(Vector2{ hx - 16 * s, hy - 12 * s }, Vector2{ hx - 6 * s, hy - 12 * s },
                 Vector2{ hx - 14 * s, hy - 30 * s }, body);
    (void)seed;
}

static void bodyAutomaton(Vector2 c, float s, uint32_t seed, float t, Color body,
                          Color edge, int phase, float breathe) {
    Color brass = lerpC(hex(0xB08D57), body, 0.35f);
    float w = 120 * s, h = 190 * s;
    float y = c.y + breathe * 0.3f;
    // Корпус из сегментов
    for (int i = 0; i < 3; ++i) {
        float sw = w * (1.0f - i * 0.18f);
        Rectangle seg{ c.x - sw * 0.5f, y - h * 0.30f + i * h * 0.24f, sw, h * 0.22f };
        DrawRectangleRounded(seg, 0.18f, 6, brass);
        DrawRectangleRoundedLinesEx(seg, 0.18f, 6, 2.0f, withA(edge, 200));
        for (int b = 0; b < 4; ++b)
            DrawCircleV(Vector2{ seg.x + 8 * s + b * (seg.width - 16 * s) / 3, seg.y + 6 * s },
                        2.2f * s, pal::Bone);
    }
    // Шестерни
    for (int g = 0; g < 2 + phase; ++g) {
        float gx = c.x - 26 * s + g * 26 * s, gy = y + h * 0.06f;
        float rot = t * (30.0f + g * 14.0f) * ((g % 2) ? -1.0f : 1.0f);
        DrawPolyLinesEx(Vector2{ gx, gy }, 8, 13 * s, rot, 2.4f * s, pal::GoldLit);
        DrawCircleV(Vector2{ gx, gy }, 4 * s, scaleC(brass, 0.7f));
    }
    // Руки-клещи
    for (int k = -1; k <= 1; k += 2) {
        float ax = c.x + k * w * 0.52f;
        DrawRectangleRounded(Rectangle{ ax - 6 * s, y - h * 0.24f, 12 * s, h * 0.34f }, 0.5f, 5, brass);
        float open = 6 * s + 5 * s * std::sin(t * 2.2f + k);
        DrawLineEx(Vector2{ ax, y + h * 0.10f }, Vector2{ ax - open, y + h * 0.24f }, 4.0f * s, brass);
        DrawLineEx(Vector2{ ax, y + h * 0.10f }, Vector2{ ax + open, y + h * 0.24f }, 4.0f * s, brass);
    }
    // Голова-фонарь с единственной линзой
    float hy = y - h * 0.44f;
    DrawRectangleRounded(Rectangle{ c.x - 26 * s, hy - 22 * s, 52 * s, 44 * s }, 0.25f, 6, brass);
    DrawRectangleRoundedLinesEx(Rectangle{ c.x - 26 * s, hy - 22 * s, 52 * s, 44 * s },
                                0.25f, 6, 2.0f, pal::GoldLit);
    float glow = 0.5f + 0.5f * std::sin(t * 2.5f);
    DrawCircleGradient(int(c.x), int(hy), 30 * s, withA(edge, (unsigned char)(90 + 70 * glow)), BLANK);
    DrawCircleV(Vector2{ c.x, hy }, 13 * s, hex(0x07060A));
    DrawCircleV(Vector2{ c.x, hy }, 8 * s, withA(edge, (unsigned char)(180 + 70 * glow)));
    DrawCircleLines(int(c.x), int(hy), 13 * s, pal::GoldLit);
    (void)seed;
}

static void bodyChoir(Vector2 c, float s, uint32_t seed, float t, Color body,
                      Color edge, int phase, float breathe) {
    float h = 230 * s, w = 74 * s;
    float y = c.y + breathe;
    // Органные трубы за спиной
    for (int i = 0; i < 7; ++i) {
        float px = c.x - 84 * s + i * 28 * s;
        float ph = (60 + std::abs(3 - i) * -12 + 40) * s;
        DrawRectangleRounded(Rectangle{ px - 8 * s, y - h * 0.5f - ph * 0.35f, 16 * s, ph },
                             0.5f, 5, scaleC(body, 0.55f));
        DrawRectangleRoundedLinesEx(Rectangle{ px - 8 * s, y - h * 0.5f - ph * 0.35f, 16 * s, ph },
                                    0.5f, 5, 1.2f, withA(edge, 120));
    }
    // Столб-ряса
    DrawTriangle(Vector2{ c.x - w * 0.9f, y + h * 0.5f },
                 Vector2{ c.x, y - h * 0.46f },
                 Vector2{ c.x + w * 0.9f, y + h * 0.5f }, body);
    DrawTriangleLines(Vector2{ c.x - w * 0.9f, y + h * 0.5f },
                      Vector2{ c.x, y - h * 0.46f },
                      Vector2{ c.x + w * 0.9f, y + h * 0.5f }, withA(edge, 200));
    // Ряд поющих ртов — чем выше фаза, тем их больше
    int mouths = 4 + phase;
    for (int i = 0; i < mouths; ++i) {
        float my = y - h * 0.34f + i * (h * 0.72f / mouths);
        float open = 4 * s + 5 * s * std::abs(std::sin(t * 2.0f + i * 0.9f));
        float mw = 7 * s + i * 1.5f * s;
        DrawEllipse(int(c.x), int(my), int(mw), int(open), hex(0x120A10));
        DrawEllipseLines(int(c.x), int(my), mw, open, withA(pal::Bone, 190));
        for (int k = -1; k <= 1; k += 2)
            DrawCircleV(Vector2{ c.x + k * (mw + 7 * s), my - 3 * s }, 2.4f * s,
                        withA(edge, (unsigned char)(140 + 80 * std::sin(t * 3.0f + i))));
    }
    (void)seed;
}

static void bodyDoll(Vector2 c, float s, uint32_t seed, float t, Color body,
                     Color edge, int phase, float breathe) {
    Color wax = lerpC(hex(0xE8D9BE), body, 0.30f);
    float y = c.y + breathe;
    float tilt = std::sin(t * 1.1f) * 3.0f * s;
    DrawEllipse(int(c.x), int(y + 96 * s), int(46 * s), int(10 * s), Color{ 0, 0, 0, 120 });
    // Ноги и руки — короткие, суставы шариками
    for (int k = -1; k <= 1; k += 2) {
        DrawLineEx(Vector2{ c.x + k * 16 * s, y + 44 * s },
                   Vector2{ c.x + k * 22 * s, y + 92 * s }, 11.0f * s, wax);
        DrawCircleV(Vector2{ c.x + k * 22 * s, y + 92 * s }, 7 * s, scaleC(wax, 0.85f));
        DrawLineEx(Vector2{ c.x + k * 38 * s, y - 6 * s },
                   Vector2{ c.x + k * (52 + phase * 4) * s, y + 40 * s }, 10.0f * s, wax);
        DrawCircleV(Vector2{ c.x + k * 38 * s, y - 6 * s }, 9 * s, scaleC(wax, 0.8f));
    }
    // Туловище
    DrawEllipse(int(c.x), int(y + 18 * s), int(40 * s), int(48 * s), wax);
    DrawEllipseLines(int(c.x), int(y + 18 * s), 40 * s, 48 * s, withA(edge, 170));
    // Шов по центру
    for (int i = 0; i < 8; ++i) {
        float sy = y - 22 * s + i * 10 * s;
        DrawLineEx(Vector2{ c.x - 5 * s, sy }, Vector2{ c.x + 5 * s, sy + 4 * s }, 1.6f * s,
                   hex(0x4A3A30));
    }
    // Круглая голова с трещинами
    float hy = y - 52 * s;
    DrawCircleV(Vector2{ c.x + tilt, hy }, 34 * s, wax);
    DrawCircleLines(int(c.x + tilt), int(hy), 34 * s, withA(edge, 190));
    for (int i = 0; i < 3 + phase; ++i) {
        float a = hash01(i, 7, seed) * 6.28f;
        Vector2 p0{ c.x + tilt + std::cos(a) * 10 * s, hy + std::sin(a) * 10 * s };
        Vector2 p1{ c.x + tilt + std::cos(a) * 33 * s, hy + std::sin(a) * 33 * s };
        DrawLineEx(p0, p1, 1.6f * s, hex(0x6A5A48));
    }
    // Нарисованные глаза и щёки
    DrawCircleV(Vector2{ c.x + tilt - 12 * s, hy - 4 * s }, 6 * s, hex(0x0E0A10));
    DrawCircleV(Vector2{ c.x + tilt + 12 * s, hy - 4 * s }, 6 * s, hex(0x0E0A10));
    float blink = std::sin(t * 1.7f);
    if (blink > 0.94f) {
        DrawRectangle(int(c.x + tilt - 18 * s), int(hy - 8 * s), int(12 * s), int(9 * s), wax);
        DrawRectangle(int(c.x + tilt + 6 * s), int(hy - 8 * s), int(12 * s), int(9 * s), wax);
    }
    DrawCircleV(Vector2{ c.x + tilt - 20 * s, hy + 9 * s }, 5 * s, withA(pal::Blood, 110));
    DrawCircleV(Vector2{ c.x + tilt + 20 * s, hy + 9 * s }, 5 * s, withA(pal::Blood, 110));
    DrawLineEx(Vector2{ c.x + tilt - 7 * s, hy + 15 * s },
               Vector2{ c.x + tilt + 7 * s, hy + 15 * s }, 1.8f * s, hex(0x5A2A2A));
}

void drawEnemyBody(Vector2 center, float scale, uint32_t seed, ElementId flavour,
                   float t, float shake, Color tint, int phase, Archetype arche) {
    Vector2 c{ center.x + std::sin(t * 23.0f) * shake, center.y + std::cos(t * 19.0f) * shake };
    Color body = lerpC(hex(0x241C33), tint, 0.34f + phase * 0.09f);
    Color edge = lerpC(elem(flavour).primary, WHITE, 0.28f);
    float breathe = std::sin(t * 1.3f) * 3.0f * scale;

    DrawCircleGradient(int(c.x), int(c.y), 150 * scale * 1.6f, withA(edge, 40), BLANK);

    switch (arche) {
        case Archetype::Candelabrum: bodyCandelabrum(c, scale, seed, t, body, edge, phase, breathe); break;
        case Archetype::Portrait:    bodyPortrait(c, scale, seed, t, body, edge, phase, breathe); break;
        case Archetype::Hound:       bodyHound(c, scale, seed, t, body, edge, phase, breathe); break;
        case Archetype::Automaton:   bodyAutomaton(c, scale, seed, t, body, edge, phase, breathe); break;
        case Archetype::Choir:       bodyChoir(c, scale, seed, t, body, edge, phase, breathe); break;
        case Archetype::Doll:        bodyDoll(c, scale, seed, t, body, edge, phase, breathe); break;
        default:                     bodyReaper(c, scale, seed, t, body, edge, phase, breathe); break;
    }
    drawRuneRing(Vector2{ c.x, c.y + breathe }, 150 * scale * 0.78f,
                 6 + phase * 2, withA(edge, 110), t);
}

// ── Enemy cards / Карты противника ────────────────────────────────────────
// Противник кладёт на стол свои карты — костяные пластины с процарапанным
// знаком. Ни рамок Таро, ни позолоты: он играет не по вашим правилам.

static void moveSigil(MoveKind k, Vector2 c, float r, Color col) {
    switch (k) {
        case MoveKind::Attack:
            DrawLineEx(Vector2{ c.x - r * 0.7f, c.y + r * 0.6f },
                       Vector2{ c.x + r * 0.7f, c.y - r * 0.6f }, 2.6f, col);
            break;
        case MoveKind::Heavy:
            for (int i = 0; i < 2; ++i) {
                float y = c.y - r * 0.4f + i * r * 0.6f;
                DrawLineEx(Vector2{ c.x - r * 0.7f, y }, Vector2{ c.x, y + r * 0.5f }, 3.0f, col);
                DrawLineEx(Vector2{ c.x, y + r * 0.5f }, Vector2{ c.x + r * 0.7f, y }, 3.0f, col);
            }
            break;
        case MoveKind::Multi:
            for (int i = -1; i <= 1; ++i)
                DrawLineEx(Vector2{ c.x + i * r * 0.45f - r * 0.25f, c.y + r * 0.6f },
                           Vector2{ c.x + i * r * 0.45f + r * 0.25f, c.y - r * 0.6f }, 2.0f, col);
            break;
        case MoveKind::Curse:
            DrawCircleLines(int(c.x), int(c.y), r * 0.72f, col);
            DrawEllipse(int(c.x), int(c.y), int(r * 0.7f), int(r * 0.32f), withA(col, 90));
            DrawCircleV(c, r * 0.22f, col);
            break;
        case MoveKind::Guard:
            DrawLineEx(Vector2{ c.x - r * 0.6f, c.y - r * 0.6f },
                       Vector2{ c.x - r * 0.6f, c.y + r * 0.1f }, 2.4f, col);
            DrawLineEx(Vector2{ c.x + r * 0.6f, c.y - r * 0.6f },
                       Vector2{ c.x + r * 0.6f, c.y + r * 0.1f }, 2.4f, col);
            DrawLineEx(Vector2{ c.x - r * 0.6f, c.y + r * 0.1f }, Vector2{ c.x, c.y + r * 0.7f }, 2.4f, col);
            DrawLineEx(Vector2{ c.x + r * 0.6f, c.y + r * 0.1f }, Vector2{ c.x, c.y + r * 0.7f }, 2.4f, col);
            DrawLineEx(Vector2{ c.x - r * 0.6f, c.y - r * 0.6f },
                       Vector2{ c.x + r * 0.6f, c.y - r * 0.6f }, 2.4f, col);
            break;
        case MoveKind::Mend:
            DrawLineEx(Vector2{ c.x, c.y - r * 0.7f }, Vector2{ c.x, c.y + r * 0.7f }, 2.6f, col);
            for (int i = -1; i <= 1; ++i)
                DrawLineEx(Vector2{ c.x - r * 0.5f, c.y + i * r * 0.45f },
                           Vector2{ c.x + r * 0.5f, c.y + i * r * 0.45f }, 1.8f, col);
            break;
        case MoveKind::Enrage:
            for (int i = -1; i <= 1; ++i) {
                float x = c.x + i * r * 0.45f;
                DrawLineEx(Vector2{ x, c.y + r * 0.7f }, Vector2{ x, c.y - r * 0.5f }, 2.2f, col);
                DrawTriangle(Vector2{ x, c.y - r * 0.75f }, Vector2{ x - r * 0.2f, c.y - r * 0.4f },
                             Vector2{ x + r * 0.2f, c.y - r * 0.4f }, col);
            }
            break;
        case MoveKind::Sunder:
            DrawLineEx(Vector2{ c.x - r * 0.7f, c.y - r * 0.5f }, Vector2{ c.x - r * 0.1f, c.y }, 2.8f, col);
            DrawLineEx(Vector2{ c.x + r * 0.1f, c.y }, Vector2{ c.x + r * 0.7f, c.y + r * 0.5f }, 2.8f, col);
            DrawLineEx(Vector2{ c.x - r * 0.3f, c.y + r * 0.6f },
                       Vector2{ c.x + r * 0.3f, c.y - r * 0.6f }, 1.6f, withA(col, 130));
            break;
    }
}

void drawEnemyCard(const EnemyMove& m, Rectangle r, float t, Color flavour,
                   bool active, bool faceDown, bool dead) {
    Color bone = dead ? hex(0x3A3A42) : hex(0xC8BBA0);
    Color ink = hex(0x1A1216);
    Color seal = dead ? hex(0x4A4A52) : lerpC(pal::Blood, flavour, 0.35f);

    DrawRectangleRounded(Rectangle{ r.x + 2, r.y + 4, r.width, r.height }, 0.05f, 4,
                         Color{ 0, 0, 0, 120 });
    // Пластина с обломанным краем — никакой аккуратности
    DrawRectangleRec(r, dead ? scaleC(bone, 0.55f) : bone);
    for (int i = 0; i < 6; ++i) {
        float y = r.y + r.height * (i / 6.0f);
        float bite = 2.0f + 3.0f * hash01(i, int(r.x), 7u);
        DrawTriangle(Vector2{ r.x + r.width, y },
                     Vector2{ r.x + r.width - bite, y + r.height / 12.0f },
                     Vector2{ r.x + r.width, y + r.height / 6.0f },
                     Color{ 12, 10, 16, 255 });
    }
    DrawRectangleLinesEx(r, 1.6f, withA(ink, 200));
    // Процарапанные штрихи
    for (int i = 0; i < 4; ++i) {
        float y = r.y + 8 + i * (r.height - 16) / 4.0f + hash01(i, 3, 11u) * 4.0f;
        DrawLineEx(Vector2{ r.x + 5, y }, Vector2{ r.x + r.width - 5, y + 1.5f },
                   1.0f, withA(ink, 45));
    }

    if (faceDown) {
        // Рубашка: сургучная печать поверх пустой пластины
        Vector2 c{ r.x + r.width * 0.5f, r.y + r.height * 0.5f };
        DrawCircleV(c, r.width * 0.26f, withA(seal, 210));
        DrawPolyLinesEx(c, 5, r.width * 0.26f, t * 8.0f, 1.6f, withA(ink, 170));
        DrawCircleLines(int(c.x), int(c.y), r.width * 0.14f, withA(ink, 190));
        return;
    }

    // Знак приёма
    Vector2 sc{ r.x + r.width * 0.5f, r.y + r.height * 0.40f };
    float rad = r.width * 0.26f;
    DrawCircleV(sc, rad * 1.35f, withA(seal, dead ? 70 : 130));
    moveSigil(m.kind, sc, rad, ink);

    // Имя приёма
    {
        std::string nm = m.name.str();
        float fs = 11.0f;
        while (fs > 7.0f && textWidth(nm.c_str(), fs) > r.width - 8) fs -= 0.5f;
        text(nm.c_str(), r.x + (r.width - textWidth(nm.c_str(), fs)) * 0.5f,
             r.y + r.height - 30.0f, fs, dead ? hex(0x6A6A72) : ink);
    }
    // Урон крупно, чтобы читалось с одного взгляда
    if (m.damage > 0) {
        char d[32];
        if (m.hits > 1) snprintf(d, sizeof d, "%d x%d", m.damage, m.hits);
        else            snprintf(d, sizeof d, "%d", m.damage);
        float fs = 15.0f;
        text(d, r.x + (r.width - textWidth(d, fs)) * 0.5f, r.y + r.height - 18.0f, fs,
             dead ? hex(0x6A6A72) : pal::Blood);
    }

    if (dead) {   // разрушенная точка выбила приём из паттерна
        DrawLineEx(Vector2{ r.x + 6, r.y + 6 }, Vector2{ r.x + r.width - 6, r.y + r.height - 6 },
                   2.5f, withA(hex(0x8A8A94), 200));
        DrawLineEx(Vector2{ r.x + 6, r.y + r.height - 6 }, Vector2{ r.x + r.width - 6, r.y + 6 },
                   2.5f, withA(hex(0x8A8A94), 200));
    }
    if (active) {
        float pulse = 0.5f + 0.5f * std::sin(t * 5.0f);
        DrawRectangleLinesEx(Rectangle{ r.x - 3, r.y - 3, r.width + 6, r.height + 6 }, 2.2f,
                             withA(pal::Blood, (unsigned char)(150 + 105 * pulse)));
    }
}

} // namespace art
} // namespace shichoy
