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

    // Иллюстрация: сюжетное окно в арке, поверх него — бледный сигил стихии
    Rectangle win{ r.x + 9, r.y + 36, r.width - 18, r.height * 0.36f };
    drawCardScene(win, card, t);
    Vector2 art = { win.x + win.width * 0.5f, win.y + win.height * 0.52f };
    float ar = win.height * 0.34f;
    if (card.element != ElementId::None)
        drawSigil(card.element, art, ar, withA(lerpC(accent, WHITE, 0.55f), 120), t);

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
    float sway = std::sin(walkPhase * 0.5f) * 1.6f * s;

    // Плащ развевается позади — читается со спины и в движении
    DrawTriangle(Vector2{ pos.x - 9 * s + sway, y - 4 * s },
                 Vector2{ pos.x + sway * 1.6f, y + 15 * s },
                 Vector2{ pos.x + 9 * s + sway, y - 4 * s }, hex(0x0E0B16));
    DrawTriangle(Vector2{ pos.x - 5 * s + sway, y - 2 * s },
                 Vector2{ pos.x + sway * 1.4f, y + 12 * s },
                 Vector2{ pos.x + 5 * s + sway, y - 2 * s }, hex(0x1A1228));

    // Фалды фрака
    DrawTriangle(Vector2{ pos.x - 7 * s, y + 2 * s },
                 Vector2{ pos.x, y + 12 * s },
                 Vector2{ pos.x + 7 * s, y + 2 * s }, coat);
    // Корпус
    DrawEllipse(int(pos.x), int(y), int(7 * s), int(9 * s), coat);
    DrawEllipseLines(int(pos.x), int(y), 7 * s, 9 * s, withA(coatLit, 180));
    // Плечи
    DrawEllipse(int(pos.x - 6 * s), int(y - 3 * s), int(3 * s), int(4 * s), coatLit);
    DrawEllipse(int(pos.x + 6 * s), int(y - 3 * s), int(3 * s), int(4 * s), coatLit);
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
    // Трость с набалдашником
    float cx = pos.x + (facing == 3 ? -8.5f : 8.5f) * s;
    float lean = std::sin(walkPhase) * 0.8f * s;
    DrawLineEx(Vector2{ cx + lean, y - 3 * s }, Vector2{ cx, y + 11 * s }, 1.6f * s, hex(0x3A2A20));
    DrawCircleV(Vector2{ cx + lean, y - 4 * s }, 1.8f * s, pal::Gold);

    // Фонарь в другой руке: маленький круг света вокруг героя
    float lx = pos.x + (facing == 3 ? 7.0f : -7.0f) * s;
    float ly = y + 2 * s;
    float flick = 0.88f + 0.12f * std::sin(walkPhase * 2.3f + pos.x * 0.05f);
    DrawCircleGradient(int(lx), int(ly), 34 * s * flick, withA(hex(0xFFBA08), 46), BLANK);
    DrawRectangleRounded(Rectangle{ lx - 1.8f * s, ly - 2.4f * s, 3.6f * s, 4.8f * s }, 0.4f, 4,
                         hex(0x2A2118));
    DrawCircleV(Vector2{ lx, ly }, 1.3f * s, withA(hex(0xFFD60A), (unsigned char)(220 * flick)));
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
// Плоская заливка читается как рисунок в Paint, поэтому каждая форма здесь
// набирается слоями: глубокая тень снизу, средний тон, подсветка сверху и
// тонкий контровой край со стороны источника света. Сверху — фактура:
// складки ткани, царапины, трещины, заклёпки.

// Объёмное пятно: концентрические эллипсы от тёмного к светлому со сдвигом
// вверх — получается шар, а не блин.
void blob(Vector2 c, float rx, float ry, Color deep, Color lit, int layers) {
    for (int i = layers - 1; i >= 0; --i) {
        float k = float(i) / float(layers - 1);          // 1 снаружи … 0 в центре
        DrawEllipse(int(c.x), int(c.y + ry * 0.20f * k),
                    int(rx * (0.52f + 0.48f * k)), int(ry * (0.52f + 0.48f * k)),
                    lerpC(lit, deep, k));
    }
}
static void volumeEllipse(Vector2 c, float rx, float ry, Color deep, Color lit, int layers = 7) {
    blob(c, rx, ry, deep, lit, layers);
}

// То же для многоугольника: форму рисуем несколько раз, поджимая к центру.
static void volumePoly(const Vector2* pts, int n, Vector2 c, Color deep, Color lit,
                       float liftY, int layers = 6) {
    for (int L = layers - 1; L >= 0; --L) {
        float k = float(L) / float(layers - 1);
        float shrink = 0.55f + 0.45f * k;
        Color col = lerpC(lit, deep, k);
        for (int i = 0; i < n; ++i) {
            Vector2 a{ c.x + (pts[i].x - c.x) * shrink,
                       c.y + (pts[i].y - c.y) * shrink + liftY * k };
            int j = (i + 1) % n;
            Vector2 b{ c.x + (pts[j].x - c.x) * shrink,
                       c.y + (pts[j].y - c.y) * shrink + liftY * k };
            DrawTriangle(Vector2{ c.x, c.y + liftY * k }, a, b, col);
        }
    }
}

// Контровой край: подсвечиваем только ту часть контура, что смотрит на свет.
static void rimLight(const Vector2* pts, int n, Vector2 c, Color rim, float w) {
    for (int i = 0; i < n; ++i) {
        Vector2 a = pts[i], b = pts[(i + 1) % n];
        Vector2 mid{ (a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f };
        float up = (c.y - mid.y);                        // выше центра — светлее
        float k = clampf(up / (std::fabs(up) + 60.0f) * 1.4f, -1.0f, 1.0f);
        if (k <= 0.05f) continue;
        DrawLineEx(a, b, w * (0.5f + k), withA(rim, (unsigned char)(70 + 150 * k)));
    }
}

// Фактура: мелкие точки и штрихи по форме, чтобы поверхность не была пустой.
void grit(Vector2 c, float rx, float ry, uint32_t seed, Color col, int count) {
    for (int i = 0; i < count; ++i) {
        float a = hash01(i, 1, seed) * 6.2832f;
        float r = std::sqrt(hash01(i, 2, seed));
        float x = c.x + std::cos(a) * rx * r * 0.9f;
        float y = c.y + std::sin(a) * ry * r * 0.9f;
        float sz = 0.6f + hash01(i, 3, seed) * 1.5f;
        DrawCircleV(Vector2{ x, y }, sz, withA(col, (unsigned char)(40 + 60 * hash01(i, 4, seed))));
    }
}
static void speckle(Vector2 c, float rx, float ry, uint32_t seed, Color col, int count) {
    grit(c, rx, ry, seed, col, count);
}

// Складки ткани: линии, сходящиеся к точке подвеса.
static void folds(Vector2 top, const Vector2* hem, int n, Color dark, Color lite, float t) {
    for (int i = 0; i < n; ++i) {
        float wob = std::sin(t * 1.3f + i * 1.7f) * 3.0f;
        Vector2 h{ hem[i].x + wob, hem[i].y };
        Vector2 midp{ (top.x + h.x) * 0.5f + wob * 0.6f, (top.y + h.y) * 0.5f };
        DrawLineEx(top, midp, 2.0f, withA(dark, 130));
        DrawLineEx(midp, h, 1.4f, withA(dark, 100));
        if (i % 2 == 0)
            DrawLineEx(Vector2{ midp.x + 2.0f, midp.y }, Vector2{ h.x + 2.0f, h.y }, 1.0f,
                       withA(lite, 70));
    }
}

// Череп с глазницами, скулами и зубами — вместо круга с двумя точками.
static void skull(Vector2 c, float s, Color bone, Color shade, Color glow, float t) {
    volumeEllipse(c, 26 * s, 30 * s, scaleC(bone, 0.45f), bone, 8);
    // Челюсть отдельной формой
    volumeEllipse(Vector2{ c.x, c.y + 20 * s }, 17 * s, 13 * s, scaleC(bone, 0.40f),
                  scaleC(bone, 0.92f), 5);
    // Скулы
    DrawEllipse(int(c.x - 18 * s), int(c.y + 2 * s), int(7 * s), int(9 * s), scaleC(bone, 0.72f));
    DrawEllipse(int(c.x + 18 * s), int(c.y + 2 * s), int(7 * s), int(9 * s), scaleC(bone, 0.72f));
    // Глазницы: тёмная впадина и огонёк внутри
    for (int k = -1; k <= 1; k += 2) {
        Vector2 e{ c.x + k * 10 * s, c.y - 4 * s };
        volumeEllipse(e, 7.5f * s, 8.5f * s, hex(0x050408), hex(0x14101A), 4);
        float g = 0.55f + 0.45f * std::sin(t * 2.6f + k);
        DrawCircleGradient(int(e.x), int(e.y + 1 * s), 9 * s, withA(glow, (unsigned char)(110 * g)), BLANK);
        DrawCircleV(Vector2{ e.x, e.y + 1 * s }, 2.6f * s, withA(glow, (unsigned char)(170 + 80 * g)));
    }
    // Носовая впадина
    DrawTriangle(Vector2{ c.x, c.y + 4 * s }, Vector2{ c.x - 3.2f * s, c.y + 12 * s },
                 Vector2{ c.x + 3.2f * s, c.y + 12 * s }, hex(0x08060C));
    // Зубы
    for (int i = 0; i < 6; ++i) {
        float x = c.x - 12 * s + i * 4.8f * s;
        DrawRectangleRounded(Rectangle{ x, c.y + 15 * s, 3.4f * s, 7 * s }, 0.3f, 3,
                             scaleC(bone, 1.05f));
        DrawLineEx(Vector2{ x, c.y + 15 * s }, Vector2{ x, c.y + 22 * s }, 0.8f, shade);
    }
    // Трещины
    for (int i = 0; i < 3; ++i) {
        float a = 2.2f + i * 1.1f;
        DrawLineEx(Vector2{ c.x + std::cos(a) * 8 * s, c.y + std::sin(a) * 8 * s },
                   Vector2{ c.x + std::cos(a) * 25 * s, c.y + std::sin(a) * 25 * s },
                   1.1f, withA(shade, 150));
    }
    speckle(c, 24 * s, 26 * s, 991u, shade, 18);
}

static void bodyReaper(Vector2 c, float s, uint32_t seed, float t, Color body,
                       Color edge, int phase, float breathe) {
    const int N = 18;
    Vector2 pts[N], hem[6];
    float w = 160 * s, h = 210 * s;
    Vector2 core{ c.x, c.y + breathe };

    for (int i = 0; i < N; ++i) {
        float a = float(i) / N * 2 * PI - PI * 0.5f;
        float rad = 1.0f + (hash01(i, int(seed % 977), seed) - 0.5f) * 0.38f;
        // Низ плаща рвётся сильнее, чем плечи
        if (std::sin(a) > 0.2f) rad += 0.10f + hash01(i, 7, seed) * 0.22f;
        pts[i] = Vector2{ core.x + std::cos(a) * w * 0.5f * rad,
                          core.y + std::sin(a) * h * 0.5f * rad + std::sin(t * 1.5f + i) * 2.0f };
    }
    // Контактная тень под фигурой
    DrawEllipse(int(c.x), int(c.y + h * 0.48f), int(w * 0.46f), int(12 * s), Color{ 0, 0, 0, 130 });

    Color deep = scaleC(body, 0.48f), lit = scaleC(body, 1.95f);
    volumePoly(pts, N, core, deep, lit, -h * 0.06f, 7);
    rimLight(pts, N, core, edge, 2.6f);

    for (int i = 0; i < 6; ++i) hem[i] = pts[(N / 2 - 2 + i + N) % N];
    folds(Vector2{ core.x, core.y - h * 0.34f }, hem, 6, scaleC(body, 0.25f), lit, t);
    speckle(core, w * 0.36f, h * 0.36f, seed, scaleC(body, 0.2f), 40);

    // Костяные рёбра проступают сквозь ткань
    for (int i = 0; i < 4 + phase; ++i) {
        float yy = core.y - h * 0.24f + i * h * 0.095f;
        float ww = w * (0.30f - i * 0.022f);
        for (int k = -1; k <= 1; k += 2)
            DrawLineEx(Vector2{ core.x + k * ww * 0.2f, yy },
                       Vector2{ core.x + k * ww, yy + 5 * s }, 2.6f * s, withA(pal::Bone, 120));
    }
    skull(Vector2{ core.x, core.y - h * 0.40f }, s, pal::Bone, hex(0x6A5B48), edge, t);

    // Рога растут с фазой, с утолщением у основания
    for (int i = 0; i <= phase; ++i) {
        float dir = (i % 2 == 0) ? -1.0f : 1.0f;
        Vector2 b0{ core.x + dir * 20 * s, core.y - h * 0.44f };
        Vector2 b1{ core.x + dir * (32 + i * 6) * s, core.y - h * 0.56f - i * 9 * s };
        Vector2 b2{ core.x + dir * (40 + i * 8) * s, core.y - h * 0.68f - i * 12 * s };
        DrawLineEx(b0, b1, 5.5f * s, scaleC(pal::Bone, 0.75f));
        DrawLineEx(b1, b2, 3.0f * s, pal::Bone);
        DrawCircleV(b0, 3.2f * s, scaleC(pal::Bone, 0.6f));
    }
}

static void bodyCandelabrum(Vector2 c, float s, uint32_t seed, float t, Color body,
                            Color edge, int phase, float breathe) {
    float h = 220 * s;
    Color brass = lerpC(hex(0x8A6A32), body, 0.35f);
    Color deep = scaleC(brass, 0.35f), lit = scaleC(brass, 1.5f);

    DrawEllipse(int(c.x), int(c.y + h * 0.48f), int(52 * s), int(13 * s), Color{ 0, 0, 0, 140 });
    // Ступенчатое основание
    for (int i = 0; i < 3; ++i) {
        float rw = (46 - i * 11) * s;
        volumeEllipse(Vector2{ c.x, c.y + h * 0.44f - i * 7 * s }, rw, 8 * s - i * 1.2f * s,
                      deep, lit, 4);
    }
    // Точёная стойка с перехватами
    for (int i = 0; i < 7; ++i) {
        float y = c.y + h * 0.40f - i * h * 0.105f;
        float rw = (i % 2 == 0) ? 11 * s : 7 * s;
        volumeEllipse(Vector2{ c.x, y }, rw, 7 * s, deep, lit, 4);
    }
    DrawRectangleRounded(Rectangle{ c.x - 5.5f * s, c.y - h * 0.34f, 11 * s, h * 0.76f }, 0.5f, 6,
                         lerpC(deep, brass, 0.6f));
    DrawLineEx(Vector2{ c.x - 3 * s, c.y - h * 0.32f }, Vector2{ c.x - 3 * s, c.y + h * 0.38f },
               1.6f * s, withA(lit, 150));

    // Рожки: изогнутые дуги, а не прямые палки
    int arms = 3 + phase;
    for (int i = 0; i < arms; ++i) {
        float k = (i % 2 == 0) ? -1.0f : 1.0f;
        float lvl = c.y - h * 0.10f + (i / 2) * 36 * s;
        float len = (38 + (i / 2) * 9) * s;
        Vector2 prev{ c.x, lvl };
        for (int seg = 1; seg <= 8; ++seg) {
            float u = seg / 8.0f;
            Vector2 p{ c.x + k * len * u, lvl - std::sin(u * PI * 0.75f) * 30 * s };
            DrawLineEx(prev, p, (4.2f - u * 1.6f) * s, lerpC(brass, lit, 1.0f - u));
            prev = p;
        }
        // Чашечка и свеча
        Vector2 tip = prev;
        volumeEllipse(Vector2{ tip.x, tip.y + 2 * s }, 7 * s, 3.5f * s, deep, lit, 3);
        volumeEllipse(Vector2{ tip.x, tip.y - 9 * s }, 4.5f * s, 11 * s,
                      scaleC(pal::Vellum, 0.55f), pal::Vellum, 5);
        // Натёки воска
        for (int d = 0; d < 3; ++d) {
            float dx = (hash01(i, d, seed) - 0.5f) * 6 * s;
            float dl = 4 * s + hash01(i, d + 9, seed) * 9 * s;
            DrawRectangleRounded(Rectangle{ tip.x + dx - 1.2f * s, tip.y - 2 * s, 2.4f * s, dl },
                                 0.8f, 3, scaleC(pal::Vellum, 0.8f));
        }
        float fl = 0.72f + 0.28f * std::sin(t * 9.0f + i * 2.1f);
        Vector2 ft{ tip.x, tip.y - 20 * s };
        DrawCircleGradient(int(ft.x), int(ft.y), 26 * s * fl, withA(hex(0xFFBA08), 100), BLANK);
        DrawTriangle(Vector2{ ft.x, ft.y - 13 * s * fl }, Vector2{ ft.x - 4.5f * s, ft.y + 3 * s },
                     Vector2{ ft.x + 4.5f * s, ft.y + 3 * s }, hex(0xE85D04));
        DrawTriangle(Vector2{ ft.x, ft.y - 8 * s * fl }, Vector2{ ft.x - 2.4f * s, ft.y + 2 * s },
                     Vector2{ ft.x + 2.4f * s, ft.y + 2 * s }, hex(0xFFF3B0));
    }

    // Навершие: большое пламя с «лицом» внутри
    float top = c.y - h * 0.40f + breathe;
    float big = 0.85f + 0.15f * std::sin(t * 5.0f);
    DrawCircleGradient(int(c.x), int(top), 52 * s * big, withA(hex(0xFFBA08), 90), BLANK);
    for (int L = 0; L < 4; ++L) {
        float k = 1.0f - L * 0.22f;
        Color fc = L == 0 ? hex(0xB33A02) : (L == 1 ? hex(0xE85D04)
                 : (L == 2 ? hex(0xFFBA08) : hex(0xFFF3B0)));
        DrawTriangle(Vector2{ c.x, top - 40 * s * big * k },
                     Vector2{ c.x - 15 * s * k, top + 10 * s },
                     Vector2{ c.x + 15 * s * k, top + 10 * s }, fc);
    }
    for (int k = -1; k <= 1; k += 2) {
        DrawEllipse(int(c.x + k * 5.5f * s), int(top - 5 * s), int(2.6f * s), int(3.4f * s),
                    hex(0x07060A));
    }
    DrawEllipse(int(c.x), int(top + 3 * s), int(3.4f * s), int(2.0f * s), hex(0x2A0A04));
    (void)edge;
}

static void bodyPortrait(Vector2 c, float s, uint32_t seed, float t, Color body,
                         Color edge, int phase, float breathe) {
    float w = 180 * s, h = 232 * s;
    Rectangle frame{ c.x - w * 0.5f, c.y - h * 0.5f, w, h };
    DrawEllipse(int(c.x), int(c.y + h * 0.52f), int(w * 0.42f), int(11 * s), Color{ 0, 0, 0, 130 });

    // Холст: тёмный лак с боковым бликом
    DrawRectangleGradientV(int(frame.x), int(frame.y), int(frame.width), int(frame.height),
                           scaleC(body, 0.85f), hex(0x06050A));
    DrawRectangleGradientH(int(frame.x), int(frame.y), int(frame.width * 0.45f),
                           int(frame.height), withA(WHITE, 16), BLANK);
    speckle(c, w * 0.42f, h * 0.42f, seed, hex(0x000000), 50);

    // Резная рама в четыре профиля
    for (int i = 0; i < 4; ++i) {
        Rectangle f{ frame.x - i * 6 * s, frame.y - i * 6 * s,
                     frame.width + i * 12 * s, frame.height + i * 12 * s };
        Color gc = (i == 1) ? pal::GoldLit : (i == 2 ? pal::Gold : scaleC(pal::Gold, 0.55f));
        DrawRectangleLinesEx(f, (i == 1 ? 5.0f : 3.0f) * s, gc);
    }
    for (int i = 0; i < 9; ++i) {              // орнамент по периметру
        float u = (i + 0.5f) / 9.0f;
        for (int side = 0; side < 2; ++side) {
            float x = frame.x + frame.width * float(side);
            DrawCircleV(Vector2{ x, frame.y + frame.height * u }, 2.6f * s, pal::Gold);
            DrawCircleV(Vector2{ x, frame.y + frame.height * u }, 1.2f * s, pal::GoldLit);
        }
        DrawCircleV(Vector2{ frame.x + frame.width * u, frame.y }, 2.6f * s, pal::Gold);
        DrawCircleV(Vector2{ frame.x + frame.width * u, frame.y + frame.height }, 2.6f * s, pal::Gold);
    }
    for (int i = 0; i < 4; ++i) {              // угловые розетки
        float cx = frame.x + (i % 2) * frame.width;
        float cy = frame.y + (i / 2) * frame.height;
        volumeEllipse(Vector2{ cx, cy }, 11 * s, 11 * s, scaleC(pal::Gold, 0.4f), pal::GoldLit, 4);
        DrawPolyLinesEx(Vector2{ cx, cy }, 6, 8 * s, t * 6.0f, 1.2f, scaleC(pal::Gold, 0.7f));
    }

    // Фигура выбирается наружу
    float lean = (7.0f + phase * 5.0f) * s * (0.6f + 0.4f * std::sin(t * 1.4f));
    float hy = c.y - h * 0.14f + breathe;
    Color fleshDeep = hex(0x5E4A44), flesh = lerpC(pal::Vellum, body, 0.25f);
    volumeEllipse(Vector2{ c.x, hy + 54 * s }, 44 * s, 52 * s, fleshDeep, flesh, 7);
    DrawTriangle(Vector2{ c.x - 20 * s, hy + 26 * s }, Vector2{ c.x, hy + 52 * s },
                 Vector2{ c.x + 20 * s, hy + 26 * s }, scaleC(body, 1.2f));
    volumeEllipse(Vector2{ c.x, hy }, 25 * s, 30 * s, fleshDeep, flesh, 7);
    for (int k = -1; k <= 1; k += 2) {
        Vector2 e{ c.x + k * 9.5f * s, hy - 3 * s };
        volumeEllipse(e, 7 * s, 5.5f * s, hex(0x0A0810), hex(0x241C22), 4);
        DrawCircleV(Vector2{ e.x + lean * 0.12f, e.y + 1 * s }, 2.4f * s, pal::Bone);
        DrawCircleV(Vector2{ e.x + lean * 0.12f, e.y + 1 * s }, 1.1f * s, hex(0x07060A));
    }
    volumeEllipse(Vector2{ c.x, hy + 16 * s }, 6 * s, 9 * s, hex(0x07060A), hex(0x2A0E14), 4);
    for (int k = -1; k <= 1; k += 2) {         // кисти вцепились в раму
        float hx = c.x + k * (w * 0.5f - 2 * s);
        volumeEllipse(Vector2{ hx, c.y + 12 * s - lean }, 13 * s, 9 * s, fleshDeep, flesh, 5);
        for (int f = 0; f < 4; ++f) {
            float fy = c.y + 4 * s - lean + f * 5.5f * s;
            DrawLineEx(Vector2{ hx, fy }, Vector2{ hx - k * (9 + f * 2) * s, fy + 7 * s },
                       2.6f * s, flesh);
            DrawCircleV(Vector2{ hx - k * (9 + f * 2) * s, fy + 7 * s }, 1.4f * s, fleshDeep);
        }
    }
    DrawRectangleLinesEx(frame, 2.0f, withA(edge, 130));
}

static void bodyHound(Vector2 c, float s, uint32_t seed, float t, Color body,
                      Color edge, int phase, float breathe) {
    float step = std::sin(t * 4.0f) * 5.0f * s;
    float bx = c.x, by = c.y + 18 * s + breathe;
    Color deep = scaleC(body, 0.45f), lit = scaleC(body, 2.00f);

    DrawEllipse(int(bx), int(by + 56 * s), int(76 * s), int(12 * s), Color{ 0, 0, 0, 140 });

    // Задние лапы за корпусом
    for (int i = 0; i < 2; ++i) {
        float lx = bx - 40 * s + i * 8 * s;
        float sw = (i == 0) ? step : -step * 0.7f;
        volumeEllipse(Vector2{ lx + sw * 0.4f, by + 18 * s }, 13 * s, 20 * s, deep,
                      scaleC(body, 0.9f), 5);
        DrawLineEx(Vector2{ lx + sw * 0.4f, by + 30 * s }, Vector2{ lx + sw, by + 52 * s },
                   6.0f * s, scaleC(body, 0.75f));
        volumeEllipse(Vector2{ lx + sw, by + 54 * s }, 7 * s, 4.5f * s, deep, lit, 3);
    }
    // Грудь и круп — два объёма, а не один овал
    volumeEllipse(Vector2{ bx + 22 * s, by - 2 * s }, 40 * s, 30 * s, deep, lit, 8);
    volumeEllipse(Vector2{ bx - 30 * s, by }, 32 * s, 27 * s, deep, scaleC(body, 1.2f), 7);
    speckle(Vector2{ bx, by }, 60 * s, 24 * s, seed, scaleC(body, 0.25f), 45);

    for (int i = 0; i < 8 + phase; ++i) {      // позвонки
        float u = i / float(7 + phase);
        float x = bx - 52 * s + u * 104 * s;
        float y = by - 26 * s - std::sin(u * PI) * 8 * s;
        volumeEllipse(Vector2{ x, y }, 3.4f * s, 5.0f * s, scaleC(pal::Bone, 0.5f), pal::Bone, 3);
    }
    for (int i = 0; i < 4; ++i)                // рёбра
        DrawLineEx(Vector2{ bx + 4 * s + i * 11 * s, by - 14 * s },
                   Vector2{ bx + 10 * s + i * 11 * s, by + 14 * s }, 2.2f * s,
                   withA(pal::Bone, 90));

    Vector2 prev{ bx - 58 * s, by - 8 * s };   // хвост плетью
    for (int i = 1; i <= 6; ++i) {
        float u = i / 6.0f;
        Vector2 p{ prev.x - 8 * s, prev.y - 4 * s + std::sin(t * 3.0f + u * 4.0f) * 5 * s };
        DrawLineEx(prev, p, (5.0f - u * 3.4f) * s, scaleC(body, 0.8f));
        prev = p;
    }
    for (int i = 0; i < 2; ++i) {              // передние лапы
        float lx = bx + 44 * s + i * 9 * s;
        float sw = (i == 0) ? -step : step * 0.7f;
        DrawLineEx(Vector2{ lx, by + 10 * s }, Vector2{ lx + sw, by + 52 * s }, 7.0f * s,
                   scaleC(body, 0.85f));
        volumeEllipse(Vector2{ lx + sw, by + 54 * s }, 8 * s, 5 * s, deep, lit, 3);
    }
    // Шея и вытянутый череп
    float hx = bx + 84 * s, hy = by - 26 * s;
    DrawLineEx(Vector2{ bx + 50 * s, by - 10 * s }, Vector2{ hx - 10 * s, hy }, 19.0f * s,
               scaleC(body, 0.95f));
    volumeEllipse(Vector2{ hx, hy }, 32 * s, 15 * s, scaleC(pal::Bone, 0.45f),
                  lerpC(pal::Bone, body, 0.25f), 6);
    volumeEllipse(Vector2{ hx - 6 * s, hy - 5 * s }, 5 * s, 4 * s, hex(0x07060A), hex(0x1A1420), 3);
    float glow = 0.5f + 0.5f * std::sin(t * 6.0f);
    DrawCircleV(Vector2{ hx - 6 * s, hy - 4 * s }, 2.2f * s,
                withA(edge, (unsigned char)(170 + 85 * glow)));
    DrawLineEx(Vector2{ hx - 8 * s, hy + 7 * s }, Vector2{ hx + 30 * s, hy + 5 * s }, 2.0f * s,
               hex(0x0A0810));
    for (int i = 0; i < 7; ++i) {              // клыки сверху и снизу
        float tx = hx + 2 * s + i * 4.4f * s;
        DrawTriangle(Vector2{ tx, hy + 6 * s }, Vector2{ tx + 3.2f * s, hy + 6 * s },
                     Vector2{ tx + 1.6f * s, hy + 14 * s }, pal::Bone);
        DrawTriangle(Vector2{ tx, hy + 5 * s }, Vector2{ tx + 3.2f * s, hy + 5 * s },
                     Vector2{ tx + 1.6f * s, hy - 2 * s }, scaleC(pal::Bone, 0.9f));
    }
    for (int k = 0; k < 2; ++k)                // прижатые уши
        DrawTriangle(Vector2{ hx - 20 * s, hy - 10 * s }, Vector2{ hx - 6 * s, hy - 13 * s },
                     Vector2{ hx - 20 * s - k * 4 * s, hy - 32 * s - k * 5 * s },
                     scaleC(body, 0.7f + k * 0.3f));
}

static void bodyAutomaton(Vector2 c, float s, uint32_t seed, float t, Color body,
                          Color edge, int phase, float breathe) {
    Color brass = lerpC(hex(0xA8813F), body, 0.30f);
    Color deep = scaleC(brass, 0.33f), lit = scaleC(brass, 1.55f);
    float w = 130 * s, h = 200 * s;
    float y = c.y + breathe * 0.3f;

    DrawEllipse(int(c.x), int(y + h * 0.50f), int(w * 0.52f), int(12 * s), Color{ 0, 0, 0, 140 });

    // Корпус из трёх клёпаных сегментов
    for (int i = 0; i < 3; ++i) {
        float sw = w * (1.0f - i * 0.16f);
        Rectangle seg{ c.x - sw * 0.5f, y - h * 0.30f + i * h * 0.23f, sw, h * 0.21f };
        // Объём: несколько горизонтальных полос от тени к свету
        for (int L = 0; L < 6; ++L) {
            float k = L / 5.0f;
            Rectangle band{ seg.x, seg.y + seg.height * k * 0.92f, seg.width, seg.height * 0.20f };
            DrawRectangleRounded(band, 0.35f, 5, lerpC(lit, deep, k));
        }
        DrawRectangleRoundedLinesEx(seg, 0.18f, 6, 2.2f * s, scaleC(brass, 0.55f));
        // Заклёпки по кромке
        for (int b = 0; b < 6; ++b) {
            float bx = seg.x + 8 * s + b * (seg.width - 16 * s) / 5.0f;
            volumeEllipse(Vector2{ bx, seg.y + 6 * s }, 2.6f * s, 2.6f * s, deep, lit, 3);
            volumeEllipse(Vector2{ bx, seg.y + seg.height - 6 * s }, 2.6f * s, 2.6f * s, deep, lit, 3);
        }
        speckle(Vector2{ seg.x + seg.width * 0.5f, seg.y + seg.height * 0.5f },
                seg.width * 0.42f, seg.height * 0.38f, seed + i, hex(0x2A1E10), 22);
    }
    // Открытое нутро с шестернями
    Rectangle gearbox{ c.x - 34 * s, y + h * 0.00f, 68 * s, 40 * s };
    DrawRectangleRounded(gearbox, 0.2f, 5, hex(0x0C0A10));
    DrawRectangleRoundedLinesEx(gearbox, 0.2f, 5, 2.0f * s, scaleC(brass, 0.6f));
    for (int g = 0; g < 2 + phase; ++g) {
        float gx = c.x - 22 * s + g * 22 * s, gy = y + h * 0.05f;
        float rot = t * (34.0f + g * 17.0f) * ((g % 2) ? -1.0f : 1.0f);
        volumeEllipse(Vector2{ gx, gy }, 12 * s, 12 * s, scaleC(brass, 0.45f), brass, 4);
        DrawPolyLinesEx(Vector2{ gx, gy }, 9, 13 * s, rot, 2.6f * s, pal::GoldLit);
        DrawCircleV(Vector2{ gx, gy }, 3.4f * s, hex(0x0A0810));
    }
    // Руки-клещи на шарнирах
    for (int k = -1; k <= 1; k += 2) {
        float ax = c.x + k * w * 0.54f;
        volumeEllipse(Vector2{ ax, y - h * 0.22f }, 9 * s, 9 * s, deep, lit, 4);
        for (int seg = 0; seg < 3; ++seg) {
            float y0 = y - h * 0.20f + seg * h * 0.12f;
            DrawRectangleRounded(Rectangle{ ax - 6 * s, y0, 12 * s, h * 0.11f }, 0.5f, 5,
                                 lerpC(lit, deep, seg / 2.0f));
            volumeEllipse(Vector2{ ax, y0 + h * 0.11f }, 6 * s, 6 * s, deep, lit, 3);
        }
        float open = 7 * s + 6 * s * std::sin(t * 2.2f + k);
        Vector2 j{ ax, y + h * 0.15f };
        DrawLineEx(j, Vector2{ ax - open, y + h * 0.29f }, 5.0f * s, brass);
        DrawLineEx(j, Vector2{ ax + open, y + h * 0.29f }, 5.0f * s, brass);
        DrawCircleV(Vector2{ ax - open, y + h * 0.29f }, 3.0f * s, lit);
        DrawCircleV(Vector2{ ax + open, y + h * 0.29f }, 3.0f * s, lit);
    }
    // Голова-фонарь с линзой и бликом
    float hy = y - h * 0.45f;
    Rectangle head{ c.x - 28 * s, hy - 24 * s, 56 * s, 48 * s };
    for (int L = 0; L < 6; ++L) {
        float k = L / 5.0f;
        DrawRectangleRounded(Rectangle{ head.x, head.y + head.height * k * 0.9f,
                                        head.width, head.height * 0.22f }, 0.3f, 5,
                             lerpC(lit, deep, k));
    }
    DrawRectangleRoundedLinesEx(head, 0.25f, 6, 2.4f * s, pal::GoldLit);
    for (int b = 0; b < 4; ++b)                // болты по углам
        volumeEllipse(Vector2{ head.x + (b % 2) * head.width, head.y + (b / 2) * head.height },
                      3.0f * s, 3.0f * s, deep, lit, 3);
    float glow = 0.5f + 0.5f * std::sin(t * 2.5f);
    DrawCircleGradient(int(c.x), int(hy), 38 * s, withA(edge, (unsigned char)(80 + 70 * glow)), BLANK);
    volumeEllipse(Vector2{ c.x, hy }, 15 * s, 15 * s, hex(0x07060A), hex(0x1C1622), 4);
    DrawCircleV(Vector2{ c.x, hy }, 9 * s, withA(edge, (unsigned char)(170 + 80 * glow)));
    DrawCircleV(Vector2{ c.x - 3 * s, hy - 3 * s }, 3 * s, withA(WHITE, 190));   // блик на линзе
    DrawCircleLines(int(c.x), int(hy), 15 * s, pal::GoldLit);
    // Труба с паром
    DrawRectangleRounded(Rectangle{ c.x + 18 * s, hy - 40 * s, 9 * s, 20 * s }, 0.4f, 4, deep);
    for (int i = 0; i < 3; ++i) {
        float u = std::fmod(t * 0.5f + i * 0.33f, 1.0f);
        DrawCircleV(Vector2{ c.x + 22 * s + std::sin(u * 5.0f) * 5 * s, hy - 44 * s - u * 26 * s },
                    (3.0f + u * 5.0f) * s, withA(pal::Fog, (unsigned char)(70 * (1.0f - u))));
    }
}

static void bodyChoir(Vector2 c, float s, uint32_t seed, float t, Color body,
                      Color edge, int phase, float breathe) {
    float h = 240 * s, w = 80 * s;
    float y = c.y + breathe;
    Color deep = scaleC(body, 0.44f), lit = scaleC(body, 1.90f);

    DrawEllipse(int(c.x), int(y + h * 0.50f), int(w * 0.9f), int(12 * s), Color{ 0, 0, 0, 140 });

    // Органные трубы за спиной, с бликом по металлу
    for (int i = 0; i < 9; ++i) {
        float px = c.x - 100 * s + i * 25 * s;
        float ph = (110 - std::abs(4 - i) * 16) * s;
        Rectangle pipe{ px - 8 * s, y - h * 0.46f - ph * 0.30f, 16 * s, ph };
        DrawRectangleRounded(pipe, 0.5f, 5, scaleC(body, 0.42f));
        DrawRectangleRounded(Rectangle{ pipe.x + 3 * s, pipe.y, 4 * s, pipe.height }, 0.5f, 4,
                             scaleC(body, 0.85f));
        volumeEllipse(Vector2{ px, pipe.y }, 8 * s, 4 * s, deep, lit, 3);
        DrawLineEx(Vector2{ px - 6 * s, pipe.y + pipe.height * 0.35f },
                   Vector2{ px + 6 * s, pipe.y + pipe.height * 0.35f }, 1.6f * s,
                   withA(hex(0x07060A), 200));
    }

    // Ряса столбом: набираем слоями трапеций
    Vector2 hemPts[5];
    for (int L = 0; L < 7; ++L) {
        float k = L / 6.0f;
        float ww = w * (0.35f + 0.65f * k);
        Color col = lerpC(lit, deep, k);
        DrawTriangle(Vector2{ c.x - ww * 0.92f, y + h * 0.48f },
                     Vector2{ c.x, y - h * 0.44f + k * 10 * s },
                     Vector2{ c.x + ww * 0.92f, y + h * 0.48f }, col);
    }
    for (int i = 0; i < 5; ++i)
        hemPts[i] = Vector2{ c.x - w * 0.8f + i * w * 0.4f, y + h * 0.47f };
    folds(Vector2{ c.x, y - h * 0.40f }, hemPts, 5, scaleC(body, 0.18f), lit, t);
    speckle(Vector2{ c.x, y + h * 0.1f }, w * 0.6f, h * 0.3f, seed, scaleC(body, 0.2f), 40);

    // Капюшон
    volumeEllipse(Vector2{ c.x, y - h * 0.40f }, 26 * s, 22 * s, deep, scaleC(body, 1.1f), 6);
    DrawEllipse(int(c.x), int(y - h * 0.38f), int(18 * s), int(15 * s), hex(0x07060A));

    // Столб поющих ртов
    int mouths = 4 + phase;
    for (int i = 0; i < mouths; ++i) {
        float my = y - h * 0.28f + i * (h * 0.66f / mouths);
        float open = 5 * s + 7 * s * std::abs(std::sin(t * 2.0f + i * 0.9f));
        float mw = 8 * s + i * 1.6f * s;
        // Губы объёмом, зев тенью
        volumeEllipse(Vector2{ c.x, my }, mw + 3 * s, open + 3 * s,
                      scaleC(body, 0.5f), lerpC(pal::Vellum, body, 0.5f), 4);
        DrawEllipse(int(c.x), int(my), int(mw), int(open), hex(0x0A0610));
        for (int d = 0; d < 5; ++d) {           // зубы
            float dx = c.x - mw * 0.7f + d * mw * 0.35f;
            DrawRectangle(int(dx), int(my - open), int(2.0f * s), int(open * 0.5f),
                          scaleC(pal::Bone, 0.9f));
        }
        for (int k = -1; k <= 1; k += 2) {      // глаза по бокам рта
            Vector2 e{ c.x + k * (mw + 10 * s), my - 4 * s };
            DrawCircleV(e, 3.4f * s, hex(0x07060A));
            DrawCircleV(e, 1.7f * s,
                        withA(edge, (unsigned char)(140 + 90 * std::sin(t * 3.0f + i))));
        }
    }
}

static void bodyDoll(Vector2 c, float s, uint32_t seed, float t, Color body,
                     Color edge, int phase, float breathe) {
    Color wax = lerpC(hex(0xEADAC0), body, 0.22f);
    Color deep = scaleC(wax, 0.45f), lit = scaleC(wax, 1.12f);
    float y = c.y + breathe;
    float tilt = std::sin(t * 1.1f) * 3.5f * s;

    DrawEllipse(int(c.x), int(y + 100 * s), int(50 * s), int(11 * s), Color{ 0, 0, 0, 130 });

    // Ноги и руки на шарнирах
    for (int k = -1; k <= 1; k += 2) {
        volumeEllipse(Vector2{ c.x + k * 18 * s, y + 62 * s }, 8 * s, 22 * s, deep, lit, 5);
        volumeEllipse(Vector2{ c.x + k * 22 * s, y + 92 * s }, 9 * s, 8 * s, deep, lit, 4);
        volumeEllipse(Vector2{ c.x + k * 40 * s, y + 16 * s }, 7 * s, 24 * s, deep, lit, 5);
        volumeEllipse(Vector2{ c.x + k * 38 * s, y - 8 * s }, 10 * s, 10 * s, deep, lit, 4);
        volumeEllipse(Vector2{ c.x + k * (46 + phase * 4) * s, y + 42 * s }, 8 * s, 8 * s,
                      deep, lit, 4);
    }
    // Туловище
    volumeEllipse(Vector2{ c.x, y + 20 * s }, 42 * s, 50 * s, deep, lit, 8);
    // Шов по центру со стежками
    for (int i = 0; i < 9; ++i) {
        float sy = y - 24 * s + i * 10 * s;
        DrawLineEx(Vector2{ c.x - 6 * s, sy }, Vector2{ c.x + 6 * s, sy + 4 * s }, 1.8f * s,
                   hex(0x4A3A30));
        DrawLineEx(Vector2{ c.x - 4 * s, sy + 3 * s }, Vector2{ c.x + 4 * s, sy + 1 * s }, 1.0f * s,
                   withA(hex(0x2A1E18), 180));
    }
    DrawLineEx(Vector2{ c.x, y - 28 * s }, Vector2{ c.x, y + 66 * s }, 1.2f * s,
               withA(hex(0x3A2E26), 150));
    speckle(Vector2{ c.x, y + 20 * s }, 34 * s, 42 * s, seed, hex(0x8A7358), 30);

    // Голова
    float hy = y - 54 * s;
    volumeEllipse(Vector2{ c.x + tilt, hy }, 36 * s, 38 * s, deep, lit, 8);
    // Трещины по фарфору
    for (int i = 0; i < 3 + phase; ++i) {
        float a = hash01(i, 7, seed) * 6.28f;
        Vector2 p{ c.x + tilt + std::cos(a) * 9 * s, hy + std::sin(a) * 9 * s };
        for (int seg = 0; seg < 4; ++seg) {
            float a2 = a + (hash01(i, seg, seed) - 0.5f) * 1.1f;
            Vector2 q{ p.x + std::cos(a2) * 8 * s, p.y + std::sin(a2) * 8 * s };
            DrawLineEx(p, q, (2.0f - seg * 0.35f) * s, hex(0x6A5A48));
            p = q;
        }
    }
    // Нарисованные глаза с бликом
    for (int k = -1; k <= 1; k += 2) {
        Vector2 e{ c.x + tilt + k * 13 * s, hy - 4 * s };
        volumeEllipse(e, 7 * s, 8 * s, hex(0xF0E6D2), WHITE, 3);
        DrawCircleV(e, 4.4f * s, hex(0x2A1A10));
        DrawCircleV(e, 2.2f * s, hex(0x07060A));
        DrawCircleV(Vector2{ e.x - 1.6f * s, e.y - 2.0f * s }, 1.4f * s, withA(WHITE, 220));
        for (int l = 0; l < 4; ++l)             // ресницы
            DrawLineEx(Vector2{ e.x - 6 * s + l * 4 * s, e.y - 7 * s },
                       Vector2{ e.x - 7 * s + l * 4 * s, e.y - 11 * s }, 1.2f * s, hex(0x2A1A10));
    }
    DrawCircleV(Vector2{ c.x + tilt - 22 * s, hy + 10 * s }, 6 * s, withA(pal::Blood, 90));
    DrawCircleV(Vector2{ c.x + tilt + 22 * s, hy + 10 * s }, 6 * s, withA(pal::Blood, 90));
    // Рот-шов
    DrawLineEx(Vector2{ c.x + tilt - 9 * s, hy + 17 * s },
               Vector2{ c.x + tilt + 9 * s, hy + 17 * s }, 2.0f * s, hex(0x5A2A2A));
    for (int i = 0; i < 5; ++i)
        DrawLineEx(Vector2{ c.x + tilt - 8 * s + i * 4 * s, hy + 14 * s },
                   Vector2{ c.x + tilt - 8 * s + i * 4 * s, hy + 20 * s }, 1.1f * s, hex(0x5A2A2A));
    (void)edge;
}

void drawEnemyBody(Vector2 center, float scale, uint32_t seed, ElementId flavour,
                   float t, float shake, Color tint, int phase, Archetype arche) {
    Vector2 c{ center.x + std::sin(t * 23.0f) * shake, center.y + std::cos(t * 19.0f) * shake };
    Color body = lerpC(hex(0x4A3E68), tint, 0.42f + phase * 0.07f);
    Color edge = lerpC(elem(flavour).primary, WHITE, 0.32f);
    float breathe = std::sin(t * 1.3f) * 3.0f * scale;

    // Мягкое зарево позади фигуры отделяет её от фона
    DrawCircleGradient(int(c.x), int(c.y), 250 * scale, withA(edge, 34), BLANK);

    switch (arche) {
        case Archetype::Candelabrum: bodyCandelabrum(c, scale, seed, t, body, edge, phase, breathe); break;
        case Archetype::Portrait:    bodyPortrait(c, scale, seed, t, body, edge, phase, breathe); break;
        case Archetype::Hound:       bodyHound(c, scale, seed, t, body, edge, phase, breathe); break;
        case Archetype::Automaton:   bodyAutomaton(c, scale, seed, t, body, edge, phase, breathe); break;
        case Archetype::Choir:       bodyChoir(c, scale, seed, t, body, edge, phase, breathe); break;
        case Archetype::Doll:        bodyDoll(c, scale, seed, t, body, edge, phase, breathe); break;
        default:                     bodyReaper(c, scale, seed, t, body, edge, phase, breathe); break;
    }
    drawRuneRing(Vector2{ c.x, c.y + breathe }, 150 * scale * 0.86f,
                 6 + phase * 2, withA(edge, 90), t);
}


// ── Фигурка врага на карте мира ───────────────────────────────────────────
// Тот же облик, что встретится в бою, только мелкий и сверху-сбоку. Раньше
// здесь был безликий треугольник, и игрок не знал, к кому идёт.
void drawEnemyToken(Vector2 pos, float scale, Archetype arche, ElementId flavour,
                    float t, float aggro, uint32_t seed) {
    const float s = scale;
    const Color tint = elem(flavour).primary;
    Color body = lerpC(hex(0x2E2544), tint, 0.34f + aggro * 0.18f);
    Color deep = scaleC(body, 0.45f), lit = scaleC(body, 1.85f);
    Color eyes = aggro > 0.35f ? hex(0xFF5A40) : lerpC(tint, WHITE, 0.4f);
    float bob = std::sin(t * 3.0f + pos.x * 0.03f) * 1.6f * s;
    float y = pos.y + bob;

    // Контактная тень
    DrawEllipse(int(pos.x), int(pos.y + 13 * s), int(11 * s), int(4 * s), Color{ 0, 0, 0, 130 });

    auto eye = [&](float dx, float dy, float r) {
        DrawCircleV(Vector2{ pos.x + dx, y + dy }, r * 1.7f, hex(0x07060A));
        DrawCircleV(Vector2{ pos.x + dx, y + dy }, r, eyes);
    };

    switch (arche) {
        case Archetype::Candelabrum: {
            blob(Vector2{ pos.x, y + 9 * s }, 8 * s, 3 * s, deep, lit, 3);   // основание
            DrawRectangleRounded(Rectangle{ pos.x - 1.6f * s, y - 7 * s, 3.2f * s, 17 * s },
                                 0.5f, 4, body);
            for (int k = -1; k <= 1; k += 2) {                                // рожки
                DrawLineEx(Vector2{ pos.x, y - 2 * s },
                           Vector2{ pos.x + k * 7 * s, y - 7 * s }, 1.8f * s, lit);
                float fl = 0.7f + 0.3f * std::sin(t * 9.0f + k);
                DrawCircleGradient(int(pos.x + k * 7 * s), int(y - 10 * s), 7 * s * fl,
                                   withA(hex(0xFFBA08), 130), BLANK);
                DrawTriangle(Vector2{ pos.x + k * 7 * s, y - 13 * s * fl },
                             Vector2{ pos.x + k * 7 * s - 2 * s, y - 8 * s },
                             Vector2{ pos.x + k * 7 * s + 2 * s, y - 8 * s }, hex(0xFFD60A));
            }
            float fl = 0.8f + 0.2f * std::sin(t * 6.0f);                      // навершие
            DrawCircleGradient(int(pos.x), int(y - 12 * s), 11 * s * fl,
                               withA(hex(0xFFBA08), 120), BLANK);
            DrawTriangle(Vector2{ pos.x, y - 18 * s * fl }, Vector2{ pos.x - 4 * s, y - 8 * s },
                         Vector2{ pos.x + 4 * s, y - 8 * s }, hex(0xE85D04));
            eye(-2.0f * s, -11 * s, 1.0f * s);
            eye(2.0f * s, -11 * s, 1.0f * s);
            break;
        }
        case Archetype::Portrait: {
            Rectangle fr{ pos.x - 9 * s, y - 13 * s, 18 * s, 24 * s };        // рама
            DrawRectangleRec(fr, hex(0x0D0A12));
            DrawRectangleGradientV(int(fr.x), int(fr.y), int(fr.width), int(fr.height),
                                   scaleC(body, 1.1f), hex(0x07060A));
            DrawRectangleLinesEx(fr, 2.4f * s, pal::Gold);
            DrawRectangleLinesEx(Rectangle{ fr.x - 1.5f * s, fr.y - 1.5f * s,
                                            fr.width + 3 * s, fr.height + 3 * s },
                                 1.2f * s, scaleC(pal::Gold, 0.6f));
            blob(Vector2{ pos.x, y + 2 * s }, 6 * s, 7 * s, deep, lit, 4);    // фигура
            blob(Vector2{ pos.x, y - 5 * s }, 4 * s, 4.5f * s,
                 scaleC(pal::Vellum, 0.5f), pal::Vellum, 4);
            eye(-1.6f * s, -5.5f * s, 0.9f * s);
            eye(1.6f * s, -5.5f * s, 0.9f * s);
            for (int k = -1; k <= 1; k += 2)                                   // кисти на раме
                blob(Vector2{ pos.x + k * 9 * s, y + 5 * s }, 2.6f * s, 2.0f * s, deep, lit, 3);
            break;
        }
        case Archetype::Hound: {
            float step = std::sin(t * 5.0f) * 2.0f * s;
            for (int i = 0; i < 4; ++i) {                                      // лапы
                float lx = pos.x - 7 * s + (i % 2) * 14 * s + (i / 2) * 2 * s;
                DrawLineEx(Vector2{ lx, y + 3 * s },
                           Vector2{ lx + ((i % 2) ? step : -step), y + 11 * s }, 2.4f * s, deep);
            }
            blob(Vector2{ pos.x - 3 * s, y + 1 * s }, 8 * s, 5.5f * s, deep, lit, 5);
            blob(Vector2{ pos.x + 5 * s, y }, 6 * s, 5 * s, deep, lit, 4);
            DrawLineEx(Vector2{ pos.x - 10 * s, y - 1 * s },                   // хвост
                       Vector2{ pos.x - 15 * s, y - 5 * s + step }, 1.8f * s, body);
            blob(Vector2{ pos.x + 12 * s, y - 3 * s }, 6 * s, 3.2f * s,        // морда
                 scaleC(pal::Bone, 0.45f), lerpC(pal::Bone, body, 0.3f), 4);
            eye(10 * s, -4.5f * s, 1.0f * s);
            for (int i = 0; i < 4; ++i)                                        // клыки
                DrawTriangle(Vector2{ pos.x + 12 * s + i * 1.6f * s, y - 1.5f * s },
                             Vector2{ pos.x + 13 * s + i * 1.6f * s, y - 1.5f * s },
                             Vector2{ pos.x + 12.5f * s + i * 1.6f * s, y + 1.5f * s },
                             pal::Bone);
            DrawTriangle(Vector2{ pos.x + 6 * s, y - 6 * s }, Vector2{ pos.x + 10 * s, y - 7 * s },
                         Vector2{ pos.x + 6 * s, y - 12 * s }, body);          // ухо
            break;
        }
        case Archetype::Automaton: {
            Color brass = lerpC(hex(0xA8813F), body, 0.3f);
            for (int i = 0; i < 2; ++i)                                        // сегменты
                blob(Vector2{ pos.x, y + 1 * s + i * 6 * s }, (8 - i * 1.5f) * s, 4.5f * s,
                     scaleC(brass, 0.4f), scaleC(brass, 1.6f), 4);
            for (int k = -1; k <= 1; k += 2) {                                 // клещи
                DrawLineEx(Vector2{ pos.x + k * 7 * s, y - 2 * s },
                           Vector2{ pos.x + k * 10 * s, y + 7 * s }, 2.2f * s, brass);
                DrawCircleV(Vector2{ pos.x + k * 10 * s, y + 7 * s }, 1.8f * s,
                            scaleC(brass, 1.5f));
            }
            DrawPolyLinesEx(Vector2{ pos.x, y + 3 * s }, 8, 3.4f * s, t * 60.0f, 1.2f * s,
                            pal::GoldLit);                                     // шестерня
            DrawRectangleRounded(Rectangle{ pos.x - 6 * s, y - 13 * s, 12 * s, 10 * s },
                                 0.3f, 4, brass);                              // голова-фонарь
            DrawRectangleRoundedLinesEx(Rectangle{ pos.x - 6 * s, y - 13 * s, 12 * s, 10 * s },
                                        0.3f, 4, 1.2f * s, pal::GoldLit);
            DrawCircleGradient(int(pos.x), int(y - 8 * s), 9 * s, withA(eyes, 110), BLANK);
            eye(0.0f, -8 * s, 2.2f * s);
            break;
        }
        case Archetype::Choir: {
            for (int i = 0; i < 5; ++i) {                                      // трубы
                float px = pos.x - 9 * s + i * 4.5f * s;
                float ph = (9 - std::abs(2 - i) * 2) * s;
                DrawRectangleRounded(Rectangle{ px - 1.4f * s, y - 10 * s - ph, 2.8f * s, ph },
                                     0.5f, 3, scaleC(body, 0.6f));
            }
            DrawTriangle(Vector2{ pos.x - 8 * s, y + 11 * s }, Vector2{ pos.x, y - 11 * s },
                         Vector2{ pos.x + 8 * s, y + 11 * s }, body);          // ряса
            DrawTriangle(Vector2{ pos.x - 4 * s, y + 11 * s }, Vector2{ pos.x, y - 9 * s },
                         Vector2{ pos.x + 4 * s, y + 11 * s }, lit);
            for (int i = 0; i < 3; ++i) {                                      // рты
                float my = y - 5 * s + i * 6 * s;
                float open = 1.4f * s + 1.6f * s * std::abs(std::sin(t * 2.2f + i));
                DrawEllipse(int(pos.x), int(my), int(2.6f * s), int(open), hex(0x0A0610));
                DrawEllipseLines(int(pos.x), int(my), 2.6f * s, open, withA(pal::Bone, 190));
            }
            eye(-4.5f * s, -7 * s, 0.9f * s);
            eye(4.5f * s, -7 * s, 0.9f * s);
            break;
        }
        case Archetype::Doll: {
            Color wax = lerpC(hex(0xEADAC0), body, 0.3f);
            for (int k = -1; k <= 1; k += 2) {                                 // конечности
                blob(Vector2{ pos.x + k * 5 * s, y + 9 * s }, 2.4f * s, 4.5f * s,
                     scaleC(wax, 0.5f), wax, 3);
                blob(Vector2{ pos.x + k * 9 * s, y + 1 * s }, 2.2f * s, 4.0f * s,
                     scaleC(wax, 0.5f), wax, 3);
            }
            blob(Vector2{ pos.x, y + 3 * s }, 7 * s, 8 * s, scaleC(wax, 0.45f), wax, 5);
            for (int i = 0; i < 4; ++i)                                        // шов
                DrawLineEx(Vector2{ pos.x - 1.5f * s, y - 2 * s + i * 3 * s },
                           Vector2{ pos.x + 1.5f * s, y - 1 * s + i * 3 * s }, 0.9f * s,
                           hex(0x4A3A30));
            blob(Vector2{ pos.x, y - 8 * s }, 6.5f * s, 6.5f * s, scaleC(wax, 0.45f), wax, 5);
            eye(-2.4f * s, -8.5f * s, 1.2f * s);
            eye(2.4f * s, -8.5f * s, 1.2f * s);
            DrawCircleV(Vector2{ pos.x - 4.5f * s, y - 6 * s }, 1.4f * s, withA(pal::Blood, 110));
            DrawCircleV(Vector2{ pos.x + 4.5f * s, y - 6 * s }, 1.4f * s, withA(pal::Blood, 110));
            DrawLineEx(Vector2{ pos.x - 2 * s, y - 4.5f * s },
                       Vector2{ pos.x + 2 * s, y - 4.5f * s }, 0.9f * s, hex(0x5A2A2A));
            break;
        }
        default: {   // Жнец
            const int N = 10;
            Vector2 pts[N];
            for (int i = 0; i < N; ++i) {
                float ang = float(i) / N * 6.2832f - 1.5708f;
                float rad = 1.0f + (hash01(i, int(seed % 503), seed) - 0.5f) * 0.35f;
                if (std::sin(ang) > 0.2f) rad += 0.18f;
                pts[i] = Vector2{ pos.x + std::cos(ang) * 9 * s * rad,
                                  y + std::sin(ang) * 12 * s * rad };
            }
            volumePoly(pts, N, Vector2{ pos.x, y }, deep, lit, -2.0f * s, 5);
            blob(Vector2{ pos.x, y - 8 * s }, 5 * s, 5.5f * s,
                 scaleC(pal::Bone, 0.5f), pal::Bone, 4);
            eye(-2.0f * s, -8.5f * s, 1.1f * s);
            eye(2.0f * s, -8.5f * s, 1.1f * s);
            for (int i = 0; i < 3; ++i)                                        // зубы
                DrawLineEx(Vector2{ pos.x - 2 * s + i * 2 * s, y - 5 * s },
                           Vector2{ pos.x - 2 * s + i * 2 * s, y - 3 * s }, 0.8f * s,
                           hex(0x07060A));
            break;
        }
    }

    // Стихия подсвечивает фигурку, а тревога — восклицательный знак
    DrawCircleGradient(int(pos.x), int(y), 20 * s, withA(tint, (unsigned char)(28 + 40 * aggro)), BLANK);
    if (aggro > 0.35f) {
        float p = 0.6f + 0.4f * std::sin(t * 9.0f);
        textC("!", pos.x, y - 26 * s, 15.0f * p + 8.0f, withA(hex(0xFF5A40), 230));
    }
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

// ── Card scenes / Сюжетные окна карт ──────────────────────────────────────
// Вместо голого сигила карта показывает маленькую сцену: небо в цветах
// стихии, силуэт горизонта поместья и мотив — пламя, волна, разряд, грани.
namespace shichoy {
namespace art {

static void motif(ElementId e, Rectangle w, float t, Color c, Color acc) {
    const float cx = w.x + w.width * 0.5f;
    const float base = w.y + w.height * 0.82f;
    const float s = w.height;

    auto flame = [&](float x, float h, float ph) {
        float f = 0.75f + 0.25f * std::sin(t * 7.0f + ph);
        DrawTriangle(Vector2{ x, base - h * f }, Vector2{ x - h * 0.22f, base },
                     Vector2{ x + h * 0.22f, base }, c);
        DrawTriangle(Vector2{ x, base - h * 0.55f * f }, Vector2{ x - h * 0.11f, base },
                     Vector2{ x + h * 0.11f, base }, acc);
    };
    auto wave = [&](float y, float amp, float ph, Color col) {
        for (int i = 0; i < 14; ++i) {
            float x0 = w.x + w.width * (i / 14.0f), x1 = w.x + w.width * ((i + 1) / 14.0f);
            float y0 = y + std::sin(t * 2.0f + i * 0.6f + ph) * amp;
            float y1 = y + std::sin(t * 2.0f + (i + 1) * 0.6f + ph) * amp;
            DrawLineEx(Vector2{ x0, y0 }, Vector2{ x1, y1 }, 2.0f, col);
        }
    };
    auto bolt = [&](float x, float ph) {
        Vector2 p{ x, w.y + s * 0.06f };
        for (int i = 0; i < 5; ++i) {
            float nx = x + std::sin(ph + i * 2.3f) * s * 0.16f;
            Vector2 q{ nx, p.y + s * 0.16f };
            DrawLineEx(p, q, 2.6f, i < 2 ? acc : c);
            p = q;
        }
    };
    auto shard = [&](float x, float y, float h, float rot, Color col) {
        DrawPoly(Vector2{ x, y }, 3, h, rot, col);
        DrawPolyLinesEx(Vector2{ x, y }, 3, h, rot, 1.2f, acc);
    };

    switch (e) {
        case ElementId::Fire:
            flame(cx, s * 0.55f, 0.0f); flame(cx - s * 0.26f, s * 0.34f, 1.7f);
            flame(cx + s * 0.26f, s * 0.38f, 3.1f);
            break;
        case ElementId::Water:
            wave(base - s * 0.10f, s * 0.05f, 0.0f, c);
            wave(base - s * 0.22f, s * 0.04f, 1.4f, acc);
            wave(base - s * 0.34f, s * 0.03f, 2.6f, withA(c, 160));
            break;
        case ElementId::Earth:
            DrawTriangle(Vector2{ cx, w.y + s * 0.18f }, Vector2{ cx - s * 0.42f, base },
                         Vector2{ cx + s * 0.42f, base }, c);
            DrawTriangle(Vector2{ cx, w.y + s * 0.18f }, Vector2{ cx - s * 0.14f, base },
                         Vector2{ cx + s * 0.10f, base }, acc);
            break;
        case ElementId::Air:
            for (int i = 0; i < 4; ++i) {
                float y = w.y + s * (0.25f + i * 0.14f);
                float o = std::sin(t * 1.8f + i) * s * 0.10f;
                DrawLineEx(Vector2{ w.x + s * 0.10f + o, y },
                           Vector2{ w.x + w.width - s * 0.12f + o, y }, 2.0f, i % 2 ? acc : c);
            }
            break;
        case ElementId::Lightning:
            bolt(cx, t * 3.0f); bolt(cx - s * 0.28f, t * 2.2f + 1.0f);
            break;
        case ElementId::Ice:
            shard(cx, base - s * 0.22f, s * 0.26f, 0.0f, c);
            shard(cx - s * 0.30f, base - s * 0.12f, s * 0.16f, 180.0f, acc);
            shard(cx + s * 0.28f, base - s * 0.14f, s * 0.18f, 20.0f, acc);
            break;
        case ElementId::Nature:
            DrawLineEx(Vector2{ cx, base }, Vector2{ cx, base - s * 0.46f }, 3.0f, c);
            for (int i = 0; i < 4; ++i) {
                float y = base - s * (0.16f + i * 0.10f);
                float k = (i % 2 ? 1.0f : -1.0f);
                DrawLineEx(Vector2{ cx, y }, Vector2{ cx + k * s * 0.22f, y - s * 0.10f }, 2.2f, acc);
            }
            break;
        case ElementId::Shadow:
            DrawCircleV(Vector2{ cx, base - s * 0.30f }, s * 0.26f, hex(0x07060A));
            DrawCircleLines(int(cx), int(base - s * 0.30f), s * 0.26f, c);
            for (int i = 0; i < 7; ++i) {
                float a = t * 0.6f + i * 0.9f;
                float bx = cx, by = base - s * 0.30f;
                DrawLineEx(Vector2{ bx + std::cos(a) * s * 0.28f, by + std::sin(a) * s * 0.28f },
                           Vector2{ bx + std::cos(a) * s * 0.42f, by + std::sin(a) * s * 0.42f },
                           1.6f, withA(acc, 170));
            }
            break;
        case ElementId::Metal:
            DrawLineEx(Vector2{ cx, base }, Vector2{ cx, w.y + s * 0.14f }, 4.0f, c);
            DrawLineEx(Vector2{ cx - s * 0.22f, base - s * 0.34f },
                       Vector2{ cx + s * 0.22f, base - s * 0.34f }, 3.0f, acc);
            DrawTriangle(Vector2{ cx, w.y + s * 0.06f }, Vector2{ cx - s * 0.08f, w.y + s * 0.20f },
                         Vector2{ cx + s * 0.08f, w.y + s * 0.20f }, acc);
            break;
        case ElementId::Light: {
            DrawCircleGradient(int(cx), int(base - s * 0.34f), s * 0.44f, withA(acc, 150), BLANK);
            DrawCircleV(Vector2{ cx, base - s * 0.34f }, s * 0.15f, acc);
            for (int i = 0; i < 10; ++i) {
                float a = t * 0.5f + i * 0.628f;
                DrawLineEx(Vector2{ cx + std::cos(a) * s * 0.20f, base - s * 0.34f + std::sin(a) * s * 0.20f },
                           Vector2{ cx + std::cos(a) * s * 0.40f, base - s * 0.34f + std::sin(a) * s * 0.40f },
                           1.8f, withA(c, 200));
            }
            break;
        }
        case ElementId::Poison:
            for (int i = 0; i < 6; ++i) {
                float ph = t * 1.6f + i * 1.1f;
                float y = base - std::fmod(ph * s * 0.12f, s * 0.55f);
                DrawCircleV(Vector2{ cx + std::sin(ph) * s * 0.26f, y }, s * 0.055f, withA(c, 210));
            }
            DrawEllipse(int(cx), int(base), int(s * 0.34f), int(s * 0.06f), acc);
            break;
        case ElementId::Aether:
            for (int i = 0; i < 3; ++i)
                DrawEllipseLines(int(cx), int(base - s * 0.32f),
                                 s * (0.16f + i * 0.11f), s * (0.30f - i * 0.07f), withA(c, 200));
            DrawCircleV(Vector2{ cx, base - s * 0.32f }, s * 0.08f, acc);
            break;
        case ElementId::Blood:
            for (int i = 0; i < 5; ++i) {
                float x = cx - s * 0.30f + i * s * 0.15f;
                float d = std::fmod(t * 0.5f + i * 0.37f, 1.0f);
                DrawCircleV(Vector2{ x, w.y + s * 0.16f + d * s * 0.56f }, s * 0.05f, c);
            }
            DrawEllipse(int(cx), int(base), int(s * 0.30f), int(s * 0.07f), acc);
            break;
        case ElementId::Storm:
            wave(w.y + s * 0.20f, s * 0.05f, 0.0f, withA(c, 190));
            bolt(cx + s * 0.10f, t * 4.0f);
            for (int i = 0; i < 9; ++i)
                DrawLineEx(Vector2{ w.x + w.width * (i / 9.0f), base - s * 0.20f },
                           Vector2{ w.x + w.width * (i / 9.0f) - s * 0.05f, base }, 1.4f,
                           withA(acc, 160));
            break;
        case ElementId::Crystal:
            shard(cx, base - s * 0.26f, s * 0.30f, 0.0f, c);
            shard(cx - s * 0.26f, base - s * 0.12f, s * 0.17f, 12.0f, acc);
            shard(cx + s * 0.26f, base - s * 0.16f, s * 0.20f, 200.0f, acc);
            DrawCircleGradient(int(cx), int(base - s * 0.26f), s * 0.36f, withA(acc, 90), BLANK);
            break;
        case ElementId::Arcane:
            drawRuneRing(Vector2{ cx, base - s * 0.32f }, s * 0.30f, 8, withA(c, 210), t);
            DrawPolyLinesEx(Vector2{ cx, base - s * 0.32f }, 5, s * 0.18f, t * 22.0f, 1.8f, acc);
            break;
        default: break;
    }
}

void drawCardScene(Rectangle win, const Card& card, float t) {
    Color accent = card.tint();
    Color acc = lerpC(accent, WHITE, 0.45f);

    BeginScissorMode(int(win.x), int(win.y), int(win.width), int(win.height));
    // Небо в цветах стихии, уходящее в ночь
    DrawRectangleGradientV(int(win.x), int(win.y), int(win.width), int(win.height),
                           scaleC(accent, 0.30f), hex(0x07060A));
    // Далёкий горизонт: крыши и шпили поместья
    float hy = win.y + win.height * 0.62f;
    for (int i = 0; i < 7; ++i) {
        float x = win.x + win.width * (i / 7.0f);
        float w7 = win.width / 7.0f;
        float h = win.height * (0.10f + 0.16f * hash01(i, int(card.uid % 97), 13u));
        DrawRectangle(int(x), int(hy - h), int(w7) + 1, int(win.height), hex(0x0D0B14));
        if (h > win.height * 0.18f)
            DrawTriangle(Vector2{ x + w7 * 0.5f, hy - h - win.height * 0.10f },
                         Vector2{ x, hy - h }, Vector2{ x + w7, hy - h }, hex(0x0D0B14));
    }
    DrawRectangle(int(win.x), int(hy), int(win.width), int(win.height), hex(0x0A0810));

    if (card.element != ElementId::None) {
        motif(card.element, win, t, accent, acc);
    } else if (card.type == CardType::Spell) {
        Vector2 c{ win.x + win.width * 0.5f, win.y + win.height * 0.48f };
        drawRuneRing(c, win.height * 0.30f, 7, withA(pal::GoldLit, 210), t);
        DrawPolyLinesEx(c, 5, win.height * 0.17f, t * 16.0f, 2.0f, pal::GoldLit);
    } else {
        Rectangle box{ win.x + win.width * 0.30f, win.y + win.height * 0.30f,
                       win.width * 0.40f, win.height * 0.40f };
        DrawRectangleRoundedLinesEx(box, 0.2f, 6, 2.0f, withA(pal::Bone, 220));
        DrawLineEx(Vector2{ box.x, box.y + box.height * 0.5f },
                   Vector2{ box.x + box.width, box.y + box.height * 0.5f }, 1.8f, pal::Bone);
    }
    // Туман по земле
    DrawRectangleGradientV(int(win.x), int(hy - win.height * 0.10f), int(win.width),
                           int(win.height * 0.30f), BLANK, withA(pal::Fog, 60));
    EndScissorMode();

    // Рама окна и арка над ним
    DrawRectangleLinesEx(win, 1.6f, withA(pal::Gold, 190));
    DrawCircleSectorLines(Vector2{ win.x + win.width * 0.5f, win.y },
                          win.width * 0.5f, 180, 360, 14, withA(pal::Gold, 150));
    DrawCircleV(Vector2{ win.x, win.y }, 2.0f, pal::GoldLit);
    DrawCircleV(Vector2{ win.x + win.width, win.y }, 2.0f, pal::GoldLit);
}

} // namespace art
} // namespace shichoy
