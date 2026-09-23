#include "art/Vfx.h"
#include "art/ProcArt.h"
#include <algorithm>
#include <cmath>

namespace shichoy {
namespace art {

// k: 1 в начале эффекта → 0 в конце. Почти всё гаснет по k.
// e: 0..1 «раскрытие» — первые 25% времени эффект вырастает, потом спадает.
static float envelope(float k) {
    float grow = clampf((1.0f - k) / 0.25f, 0.0f, 1.0f);
    return grow * k;
}

// ── Языки пламени вдоль нижней кромки и боков ─────────────────────────────
static void edgeFire(int sw, int sh, float t, float k, Color c, uint32_t seed) {
    float e = envelope(k);
    unsigned char a = (unsigned char)(235 * e);
    DrawRectangleGradientV(0, int(sh * 0.62f), sw, int(sh * 0.38f), BLANK,
                           withA(scaleC(c, 0.7f), (unsigned char)(90 * e)));
    // Свечение у самой кромки — пламя не должно начинаться резкой линией
    DrawRectangleGradientV(0, int(sh * 0.80f), sw, int(sh * 0.20f), BLANK,
                           withA(lerpC(c, hex(0xFFBA08), 0.35f), (unsigned char)(120 * e)));
    // Три прохода: широкое тёмное основание, средний тон, узкое яркое ядро.
    // Смещения случайные, иначе получается ровная пила.
    struct Pass { int n; float hs, ws, mix; unsigned char al; };
    const Pass passes[3] = { { 22, 1.00f, 2.10f, 0.00f, 150 },
                             { 30, 0.74f, 1.25f, 0.30f, 200 },
                             { 26, 0.44f, 0.62f, 0.62f, 235 } };
    for (int p = 0; p < 3; ++p) {
        const Pass& ps = passes[p];
        for (int i = 0; i < ps.n; ++i) {
            float jitter = (hash01(i, p * 7 + 1, seed) - 0.5f) * (sw / ps.n) * 1.4f;
            float x = (i + 0.5f) / ps.n * sw + jitter + std::sin(t * 2.6f + i * 0.8f + p) * 7.0f;
            float flick = 0.58f + 0.42f * std::sin(t * (10.0f + p * 3.0f) + i * 1.9f
                                                   + hash01(i, p + 2, seed) * 6.2832f);
            float h = sh * (0.13f + 0.30f * hash01(i, p * 5 + 3, seed)) * e * flick * ps.hs;
            float w = (sw / ps.n) * ps.ws * (0.7f + hash01(i, p * 3 + 4, seed) * 0.7f);
            Color col = withA(lerpC(scaleC(c, 0.8f), hex(0xFFE08A), ps.mix),
                              (unsigned char)(ps.al * e));
            // Язык из двух треугольников — с наклоном, а не симметричный
            float lean = std::sin(t * 3.4f + i) * w * 0.30f;
            DrawTriangle(Vector2{ x + lean, float(sh) - h },
                         Vector2{ x - w * 0.5f, float(sh) },
                         Vector2{ x, float(sh) - h * 0.10f }, col);
            DrawTriangle(Vector2{ x + lean, float(sh) - h },
                         Vector2{ x, float(sh) - h * 0.10f },
                         Vector2{ x + w * 0.5f, float(sh) }, col);
        }
    }
    // Боковые языки
    for (int side = 0; side < 2; ++side) {
        for (int i = 0; i < 10; ++i) {
            float y = sh * (0.25f + i / 10.0f * 0.75f);
            float f = 0.5f + 0.5f * std::sin(t * 11.0f + i * 2.3f + side * 3.0f);
            float w = sw * 0.10f * e * f;
            float x = side ? sw - w : w;
            DrawTriangle(Vector2{ side ? float(sw) : 0.0f, y - 26 },
                         Vector2{ x, y }, Vector2{ side ? float(sw) : 0.0f, y + 26 },
                         withA(scaleC(c, 0.8f), (unsigned char)(a * 0.8f)));
        }
    }
    // Искры
    for (int i = 0; i < 24; ++i) {
        float u = std::fmod(t * 0.7f + hash01(i, 5, seed), 1.0f);
        float x = hash01(i, 6, seed) * sw + std::sin(t * 2.0f + i) * 20.0f;
        float y = sh - u * sh * 0.75f;
        DrawCircleV(Vector2{ x, y }, 1.4f + 1.6f * (1.0f - u),
                    withA(lerpC(c, WHITE, 0.6f), (unsigned char)(200 * e * (1.0f - u))));
    }
}

// ── Вода: волна снизу и стекающие капли ───────────────────────────────────
static void edgeWater(int sw, int sh, float t, float k, Color c, uint32_t seed) {
    float e = envelope(k);
    unsigned char a = (unsigned char)(200 * e);
    DrawRectangleGradientV(0, int(sh * 0.66f), sw, int(sh * 0.34f), BLANK,
                           withA(scaleC(c, 0.8f), (unsigned char)(110 * e)));
    for (int L = 0; L < 3; ++L) {
        float base = sh - sh * 0.10f * e * (1.0f + L * 0.55f);
        for (int i = 0; i < 46; ++i) {
            float x0 = i / 46.0f * sw, x1 = (i + 1) / 46.0f * sw;
            float y0 = base + std::sin(t * 2.4f + i * 0.35f + L) * 9.0f * e;
            float y1 = base + std::sin(t * 2.4f + (i + 1) * 0.35f + L) * 9.0f * e;
            DrawTriangle(Vector2{ x0, y0 }, Vector2{ x0, float(sh) }, Vector2{ x1, y1 },
                         withA(scaleC(c, 0.6f + L * 0.2f), (unsigned char)(a * 0.55f)));
            DrawTriangle(Vector2{ x1, y1 }, Vector2{ x0, float(sh) }, Vector2{ x1, float(sh) },
                         withA(scaleC(c, 0.6f + L * 0.2f), (unsigned char)(a * 0.55f)));
            DrawLineEx(Vector2{ x0, y0 }, Vector2{ x1, y1 }, 2.0f,
                       withA(lerpC(c, WHITE, 0.5f), a));
        }
    }
    for (int i = 0; i < 20; ++i) {           // капли по стеклу
        float x = hash01(i, 1, seed) * sw;
        float u = std::fmod(t * 0.5f + hash01(i, 2, seed), 1.0f);
        DrawEllipse(int(x), int(u * sh), 2, 5, withA(lerpC(c, WHITE, 0.4f),
                    (unsigned char)(150 * e)));
    }
}

// ── Земля: пыль, камни и трещины ──────────────────────────────────────────
static void edgeEarth(int sw, int sh, float t, float k, Color c, uint32_t seed) {
    float e = envelope(k);
    DrawRectangleGradientV(0, int(sh * 0.70f), sw, int(sh * 0.30f), BLANK,
                           withA(scaleC(c, 0.9f), (unsigned char)(120 * e)));
    for (int i = 0; i < 26; ++i) {           // взлетающие камни
        float u = std::fmod(t * 0.9f + hash01(i, 1, seed), 1.0f);
        float x = hash01(i, 2, seed) * sw;
        float y = sh - u * sh * 0.5f * e;
        float r = 3.0f + hash01(i, 3, seed) * 6.0f;
        DrawPoly(Vector2{ x, y }, 5, r * (1.0f - u * 0.4f), t * 90.0f + i * 40.0f,
                 withA(scaleC(c, 0.7f), (unsigned char)(220 * e * (1.0f - u))));
    }
    for (int i = 0; i < 8; ++i) {            // трещины от краёв внутрь
        float x0 = hash01(i, 4, seed) * sw;
        Vector2 p{ x0, float(sh) };
        for (int s = 0; s < 5; ++s) {
            Vector2 q{ p.x + (hash01(i, s + 9, seed) - 0.5f) * 40.0f,
                       p.y - sh * 0.06f * e };
            DrawLineEx(p, q, (3.2f - s * 0.5f) * e, withA(scaleC(c, 0.45f),
                       (unsigned char)(200 * e)));
            p = q;
        }
    }
}

// ── Воздух: полосы, рассекающие экран ─────────────────────────────────────
static void edgeAir(int sw, int sh, float t, float k, Color c, uint32_t seed) {
    float e = envelope(k);
    for (int i = 0; i < 22; ++i) {
        float y = hash01(i, 1, seed) * sh;
        float u = std::fmod(t * 1.6f + hash01(i, 2, seed), 1.0f);
        float len = sw * (0.18f + hash01(i, 3, seed) * 0.30f) * e;
        float x = (i % 2 ? u : 1.0f - u) * (sw + len) - len;
        DrawLineEx(Vector2{ x, y }, Vector2{ x + len, y + (hash01(i, 4, seed) - 0.5f) * 12.0f },
                   1.2f + 2.2f * e, withA(lerpC(c, WHITE, 0.5f), (unsigned char)(150 * e)));
    }
    DrawRectangleGradientH(0, 0, int(sw * 0.16f), sh, withA(c, (unsigned char)(70 * e)), BLANK);
    DrawRectangleGradientH(int(sw * 0.84f), 0, int(sw * 0.16f), sh, BLANK,
                           withA(c, (unsigned char)(70 * e)));
}

// ── Молния: ветвящиеся разряды по периметру и вспышка ─────────────────────
static void edgeLightning(int sw, int sh, float t, float k, Color c, uint32_t seed) {
    float e = envelope(k);
    // Короткая белая вспышка в самом начале
    float flash = clampf((1.0f - k) < 0.12f ? (1.0f - (1.0f - k) / 0.12f) : 0.0f, 0.0f, 1.0f);
    if (flash > 0.01f) DrawRectangle(0, 0, sw, sh, withA(lerpC(c, WHITE, 0.8f),
                                     (unsigned char)(120 * flash)));
    for (int i = 0; i < 9; ++i) {
        int side = i % 4;
        float seedf = hash01(i, int(t * 9.0f) % 97, seed);
        Vector2 p;
        Vector2 dir;
        switch (side) {
            case 0: p = Vector2{ seedf * sw, 0.0f };        dir = Vector2{ 0, 1 }; break;
            case 1: p = Vector2{ seedf * sw, float(sh) };   dir = Vector2{ 0, -1 }; break;
            case 2: p = Vector2{ 0.0f, seedf * sh };        dir = Vector2{ 1, 0 }; break;
            default: p = Vector2{ float(sw), seedf * sh };  dir = Vector2{ -1, 0 }; break;
        }
        float seg = (side < 2 ? sh : sw) * 0.09f * e;
        for (int s = 0; s < 6; ++s) {
            float jitter = (hash01(i, s + int(t * 14.0f) % 53, seed) - 0.5f) * seg * 1.3f;
            Vector2 q{ p.x + dir.x * seg + dir.y * jitter,
                       p.y + dir.y * seg + dir.x * jitter };
            DrawLineEx(p, q, (4.0f - s * 0.5f) * e, withA(lerpC(c, WHITE, 0.55f),
                       (unsigned char)(235 * e)));
            DrawLineEx(p, q, (1.6f - s * 0.2f) * e, withA(WHITE, (unsigned char)(200 * e)));
            p = q;
        }
    }
    DrawCircleGradient(sw / 2, sh / 2, sh * 0.8f, BLANK, withA(c, (unsigned char)(70 * e)));
}

// ── Лёд: кристаллы из углов и иней по кромке ──────────────────────────────
static void edgeIce(int sw, int sh, float t, float k, Color c, uint32_t seed) {
    float e = envelope(k);
    for (int corner = 0; corner < 4; ++corner) {
        Vector2 o{ (corner % 2) ? float(sw) : 0.0f, (corner / 2) ? float(sh) : 0.0f };
        float dx = (corner % 2) ? -1.0f : 1.0f, dy = (corner / 2) ? -1.0f : 1.0f;
        for (int i = 0; i < 7; ++i) {
            float len = sh * (0.12f + hash01(corner, i, seed) * 0.26f) * e;
            float ang = hash01(corner, i + 20, seed);
            Vector2 tip{ o.x + dx * len * (0.3f + ang), o.y + dy * len * (1.2f - ang) };
            Vector2 s1{ o.x + dx * len * 0.16f, o.y + dy * len * 0.10f };
            Vector2 s2{ o.x + dx * len * 0.06f, o.y + dy * len * 0.34f };
            DrawTriangle(dx * dy > 0 ? tip : s1, dx * dy > 0 ? s1 : tip, s2,
                         withA(scaleC(c, 0.85f), (unsigned char)(190 * e)));
            DrawLineEx(s1, tip, 1.4f, withA(lerpC(c, WHITE, 0.7f), (unsigned char)(220 * e)));
        }
    }
    // Иней по всему периметру
    for (int i = 0; i < 40; ++i) {
        float u = i / 40.0f;
        float w = 14.0f * e * (0.5f + hash01(i, 3, seed));
        DrawLineEx(Vector2{ u * sw, 0 }, Vector2{ u * sw, w }, 2.0f,
                   withA(lerpC(c, WHITE, 0.6f), (unsigned char)(120 * e)));
        DrawLineEx(Vector2{ u * sw, float(sh) }, Vector2{ u * sw, sh - w }, 2.0f,
                   withA(lerpC(c, WHITE, 0.6f), (unsigned char)(120 * e)));
    }
    DrawCircleGradient(sw / 2, sh / 2, sh * 0.85f, BLANK, withA(c, (unsigned char)(60 * e)));
    (void)t;
}

// ── Природа: лозы врастают внутрь кадра ───────────────────────────────────
static void edgeNature(int sw, int sh, float t, float k, Color c, uint32_t seed) {
    float e = envelope(k);
    for (int i = 0; i < 14; ++i) {
        int side = i % 4;
        float u = hash01(i, 1, seed);
        Vector2 p;
        Vector2 dir;
        switch (side) {
            case 0: p = Vector2{ u * sw, 0.0f };       dir = Vector2{ 0, 1 }; break;
            case 1: p = Vector2{ u * sw, float(sh) };  dir = Vector2{ 0, -1 }; break;
            case 2: p = Vector2{ 0.0f, u * sh };       dir = Vector2{ 1, 0 }; break;
            default: p = Vector2{ float(sw), u * sh }; dir = Vector2{ -1, 0 }; break;
        }
        float seg = (side < 2 ? sh : sw) * 0.055f * e;
        for (int s = 0; s < 6; ++s) {
            float wob = std::sin(t * 1.6f + s * 1.2f + i) * seg * 0.55f;
            Vector2 q{ p.x + dir.x * seg + dir.y * wob, p.y + dir.y * seg + dir.x * wob };
            DrawLineEx(p, q, (4.5f - s * 0.6f) * e, withA(scaleC(c, 0.75f),
                       (unsigned char)(215 * e)));
            // Листья
            if (s % 2 == 1) {
                Vector2 lf{ q.x + dir.y * 9 * e, q.y + dir.x * 9 * e };
                DrawEllipse(int(lf.x), int(lf.y), int(7 * e), int(4 * e),
                            withA(lerpC(c, WHITE, 0.3f), (unsigned char)(200 * e)));
            }
            p = q;
        }
    }
    DrawRectangleGradientV(0, int(sh * 0.78f), sw, int(sh * 0.22f), BLANK,
                           withA(scaleC(c, 0.6f), (unsigned char)(80 * e)));
}

// ── Тень: щупальца тьмы и сжимающаяся виньетка ────────────────────────────
static void edgeShadow(int sw, int sh, float t, float k, Color c, uint32_t seed) {
    float e = envelope(k);
    // Тьма сжимает кадр
    DrawCircleGradient(sw / 2, sh / 2, sh * (1.25f - 0.22f * e), BLANK,
                       withA(hex(0x05040A), (unsigned char)(140 * e)));
    for (int i = 0; i < 18; ++i) {
        int side = i % 4;
        float u = hash01(i, 1, seed);
        Vector2 p;
        Vector2 dir;
        switch (side) {
            case 0: p = Vector2{ u * sw, 0.0f };       dir = Vector2{ 0, 1 }; break;
            case 1: p = Vector2{ u * sw, float(sh) };  dir = Vector2{ 0, -1 }; break;
            case 2: p = Vector2{ 0.0f, u * sh };       dir = Vector2{ 1, 0 }; break;
            default: p = Vector2{ float(sw), u * sh }; dir = Vector2{ -1, 0 }; break;
        }
        float seg = (side < 2 ? sh : sw) * 0.07f * e;
        for (int s = 0; s < 5; ++s) {
            float wob = std::sin(t * 2.3f + s * 1.6f + i * 0.9f) * seg * 0.8f;
            Vector2 q{ p.x + dir.x * seg + dir.y * wob, p.y + dir.y * seg + dir.x * wob };
            DrawLineEx(p, q, (12.0f - s * 2.2f) * e, withA(hex(0x0A0714),
                       (unsigned char)(230 * e)));
            DrawLineEx(p, q, (4.0f - s * 0.7f) * e, withA(scaleC(c, 1.3f),
                       (unsigned char)(120 * e)));
            p = q;
        }
    }
}

// ── Металл: росчерки лезвий ───────────────────────────────────────────────
static void edgeMetal(int sw, int sh, float t, float k, Color c, uint32_t seed) {
    float e = envelope(k);
    for (int i = 0; i < 7; ++i) {
        float a = hash01(i, 1, seed) * 6.2832f + t * 0.4f;
        Vector2 mid{ sw * 0.5f + std::cos(a) * sw * 0.42f, sh * 0.5f + std::sin(a) * sh * 0.46f };
        Vector2 d{ -std::sin(a), std::cos(a) };
        float len = sh * (0.28f + hash01(i, 2, seed) * 0.4f) * e;
        Vector2 p0{ mid.x - d.x * len, mid.y - d.y * len };
        Vector2 p1{ mid.x + d.x * len, mid.y + d.y * len };
        DrawLineEx(p0, p1, 5.0f * e, withA(scaleC(c, 0.6f), (unsigned char)(160 * e)));
        DrawLineEx(p0, p1, 1.6f * e, withA(WHITE, (unsigned char)(220 * e)));
    }
    for (int i = 0; i < 4; ++i) {
        Vector2 o{ (i % 2) ? float(sw) : 0.0f, (i / 2) ? float(sh) : 0.0f };
        DrawCircleGradient(int(o.x), int(o.y), sh * 0.30f * e, withA(c, (unsigned char)(90 * e)), BLANK);
    }
}

// ── Свет: лучи из центра и общая засветка ─────────────────────────────────
static void edgeLight(int sw, int sh, float t, float k, Color c, uint32_t seed) {
    float e = envelope(k);
    Vector2 ctr{ sw * 0.5f, sh * 0.40f };
    DrawCircleGradient(int(ctr.x), int(ctr.y), sh * 1.0f * e,
                       withA(lerpC(c, WHITE, 0.5f), (unsigned char)(120 * e)), BLANK);
    for (int i = 0; i < 20; ++i) {
        float a = i / 20.0f * 6.2832f + t * 0.25f;
        float w = (0.012f + hash01(i, 1, seed) * 0.03f);
        Vector2 p1{ ctr.x + std::cos(a - w) * sh * 1.4f, ctr.y + std::sin(a - w) * sh * 1.4f };
        Vector2 p2{ ctr.x + std::cos(a + w) * sh * 1.4f, ctr.y + std::sin(a + w) * sh * 1.4f };
        DrawTriangle(ctr, p1, p2, withA(lerpC(c, WHITE, 0.6f), (unsigned char)(60 * e)));
    }
    DrawRectangle(0, 0, sw, sh, withA(WHITE, (unsigned char)(45 * e)));
}

// ── Яд: пузыри и зелёная дымка ────────────────────────────────────────────
static void edgePoison(int sw, int sh, float t, float k, Color c, uint32_t seed) {
    float e = envelope(k);
    DrawRectangleGradientV(0, int(sh * 0.55f), sw, int(sh * 0.45f), BLANK,
                           withA(scaleC(c, 0.8f), (unsigned char)(120 * e)));
    for (int i = 0; i < 34; ++i) {
        float u = std::fmod(t * (0.25f + hash01(i, 1, seed) * 0.35f) + hash01(i, 2, seed), 1.0f);
        float x = hash01(i, 3, seed) * sw + std::sin(t * 2.0f + i) * 22.0f;
        float y = sh - u * sh * 0.9f;
        float r = (3.0f + hash01(i, 4, seed) * 9.0f) * e;
        DrawCircleV(Vector2{ x, y }, r, withA(scaleC(c, 0.7f), (unsigned char)(120 * e * (1.0f - u))));
        DrawCircleLines(int(x), int(y), r, withA(lerpC(c, WHITE, 0.4f),
                        (unsigned char)(180 * e * (1.0f - u))));
    }
}

// ── Эфир: разломы-кольца по периметру ─────────────────────────────────────
static void edgeAether(int sw, int sh, float t, float k, Color c, uint32_t seed) {
    float e = envelope(k);
    for (int i = 0; i < 7; ++i) {
        float a = hash01(i, 1, seed) * 6.2832f;
        Vector2 p{ sw * 0.5f + std::cos(a) * sw * 0.46f, sh * 0.5f + std::sin(a) * sh * 0.48f };
        float r = sh * (0.08f + hash01(i, 2, seed) * 0.16f) * e;
        for (int L = 0; L < 3; ++L)
            DrawEllipseLines(int(p.x), int(p.y), r * (1.0f - L * 0.22f),
                             r * (0.45f + L * 0.14f),
                             withA(lerpC(c, WHITE, 0.3f + L * 0.2f), (unsigned char)(190 * e)));
        DrawCircleGradient(int(p.x), int(p.y), r * 1.5f, withA(c, (unsigned char)(90 * e)), BLANK);
    }
    // Лёгкое «сползание» кадра по краям
    DrawRectangleGradientH(0, 0, int(sw * 0.2f), sh, withA(c, (unsigned char)(80 * e)), BLANK);
    DrawRectangleGradientH(int(sw * 0.8f), 0, int(sw * 0.2f), sh, BLANK,
                           withA(c, (unsigned char)(80 * e)));
    (void)t;
}

// ── Кровь: потёки сверху ──────────────────────────────────────────────────
static void edgeBlood(int sw, int sh, float t, float k, Color c, uint32_t seed) {
    float e = envelope(k);
    DrawRectangleGradientV(0, 0, sw, int(sh * 0.30f), withA(scaleC(c, 0.8f),
                           (unsigned char)(150 * e)), BLANK);
    for (int i = 0; i < 26; ++i) {
        float x = hash01(i, 1, seed) * sw;
        float len = sh * (0.10f + hash01(i, 2, seed) * 0.38f) * e
                  * (0.6f + 0.4f * std::sin(t * 0.8f + i));
        float w = 3.0f + hash01(i, 3, seed) * 7.0f;
        DrawRectangleRounded(Rectangle{ x - w * 0.5f, 0, w, len }, 0.5f, 4,
                             withA(scaleC(c, 0.75f), (unsigned char)(220 * e)));
        DrawCircleV(Vector2{ x, len }, w * 0.62f, withA(scaleC(c, 0.9f),
                    (unsigned char)(230 * e)));
    }
    DrawCircleGradient(sw / 2, sh / 2, sh * 0.9f, BLANK, withA(scaleC(c, 0.5f),
                       (unsigned char)(110 * e)));
}

// ── Буря: косой ливень и разряды ──────────────────────────────────────────
static void edgeStorm(int sw, int sh, float t, float k, Color c, uint32_t seed) {
    float e = envelope(k);
    for (int i = 0; i < 70; ++i) {
        float u = std::fmod(t * 1.9f + hash01(i, 1, seed), 1.0f);
        float x = hash01(i, 2, seed) * (sw + 300.0f) - 150.0f + u * 140.0f;
        float y = u * sh;
        DrawLineEx(Vector2{ x, y }, Vector2{ x - 16, y + 34 }, 1.5f,
                   withA(lerpC(c, WHITE, 0.4f), (unsigned char)(140 * e)));
    }
    edgeLightning(sw, sh, t, k, c, seed + 7u);
    DrawRectangleGradientV(0, 0, sw, int(sh * 0.3f), withA(scaleC(c, 0.5f),
                           (unsigned char)(90 * e)), BLANK);
}

// ── Кристалл: призматические осколки и радужные отсветы ───────────────────
static void edgeCrystal(int sw, int sh, float t, float k, Color c, uint32_t seed) {
    float e = envelope(k);
    for (int i = 0; i < 16; ++i) {
        float a = i / 16.0f * 6.2832f + t * 0.2f;
        Vector2 p{ sw * 0.5f + std::cos(a) * sw * 0.46f, sh * 0.5f + std::sin(a) * sh * 0.50f };
        float r = sh * (0.06f + hash01(i, 1, seed) * 0.16f) * e;
        Color fc = lerpC(c, i % 2 ? hex(0x7CE8FF) : hex(0xFFB3F0), 0.35f);
        DrawPoly(p, 3, r, a * RAD2DEG + t * 30.0f, withA(fc, (unsigned char)(150 * e)));
        DrawPolyLinesEx(p, 3, r, a * RAD2DEG + t * 30.0f, 1.6f,
                        withA(lerpC(fc, WHITE, 0.6f), (unsigned char)(220 * e)));
        DrawCircleGradient(int(p.x), int(p.y), r * 2.0f, withA(fc, (unsigned char)(60 * e)), BLANK);
    }
}

// ── Аркана: вращающиеся рунные кольца по периметру ────────────────────────
static void edgeArcane(int sw, int sh, float t, float k, Color c, uint32_t seed) {
    float e = envelope(k);
    Vector2 ctr{ sw * 0.5f, sh * 0.42f };
    for (int L = 0; L < 3; ++L) {
        float r = sh * (0.45f + L * 0.16f) * (0.75f + 0.25f * e);
        drawRuneRing(ctr, r, 10 + L * 5, withA(c, (unsigned char)(190 * e)),
                     t * (1.0f + L * 0.6f) * (L % 2 ? -1.0f : 1.0f));
        DrawCircleLines(int(ctr.x), int(ctr.y), r, withA(lerpC(c, WHITE, 0.4f),
                        (unsigned char)(90 * e)));
    }
    for (int i = 0; i < 10; ++i) {
        float a = i / 10.0f * 6.2832f - t * 0.5f;
        Vector2 p{ ctr.x + std::cos(a) * sw * 0.44f, ctr.y + std::sin(a) * sh * 0.46f };
        DrawPolyLinesEx(p, 5 + (i % 3), 16.0f * e, t * 40.0f + i * 30.0f, 2.0f,
                        withA(lerpC(c, WHITE, 0.5f), (unsigned char)(210 * e)));
    }
    (void)seed;
}

// ── Диспетчер ─────────────────────────────────────────────────────────────
static void drawElementEdge(ElementId e, int sw, int sh, float t, float k,
                            Color tint, uint32_t seed) {
    switch (e) {
        case ElementId::Fire:      edgeFire(sw, sh, t, k, tint, seed); break;
        case ElementId::Water:     edgeWater(sw, sh, t, k, tint, seed); break;
        case ElementId::Earth:     edgeEarth(sw, sh, t, k, tint, seed); break;
        case ElementId::Air:       edgeAir(sw, sh, t, k, tint, seed); break;
        case ElementId::Lightning: edgeLightning(sw, sh, t, k, tint, seed); break;
        case ElementId::Ice:       edgeIce(sw, sh, t, k, tint, seed); break;
        case ElementId::Nature:    edgeNature(sw, sh, t, k, tint, seed); break;
        case ElementId::Shadow:    edgeShadow(sw, sh, t, k, tint, seed); break;
        case ElementId::Metal:     edgeMetal(sw, sh, t, k, tint, seed); break;
        case ElementId::Light:     edgeLight(sw, sh, t, k, tint, seed); break;
        case ElementId::Poison:    edgePoison(sw, sh, t, k, tint, seed); break;
        case ElementId::Aether:    edgeAether(sw, sh, t, k, tint, seed); break;
        case ElementId::Blood:     edgeBlood(sw, sh, t, k, tint, seed); break;
        case ElementId::Storm:     edgeStorm(sw, sh, t, k, tint, seed); break;
        case ElementId::Crystal:   edgeCrystal(sw, sh, t, k, tint, seed); break;
        case ElementId::Arcane:    edgeArcane(sw, sh, t, k, tint, seed); break;
        default: break;
    }
}

void ScreenFx::trigger(ElementId a, ElementId b, Color tint, float power, bool reaction) {
    CardFx f;
    f.a = a; f.b = b; f.tint = tint;
    f.power = clampf(power, 0.4f, 3.0f);
    f.reaction = reaction;
    f.maxLife = reaction ? 1.35f : 0.95f;
    f.life = f.maxLife;
    fx_.push_back(f);
    if (fx_.size() > 4) fx_.erase(fx_.begin());   // не копим бесконечно
}

void ScreenFx::update(float dt) {
    for (auto& f : fx_) f.life -= dt;
    fx_.erase(std::remove_if(fx_.begin(), fx_.end(),
                             [](const CardFx& f) { return f.life <= 0.0f; }), fx_.end());
}

float ScreenFx::intensity() const {
    float s = 0.0f;
    for (const auto& f : fx_) s += envelope(f.life / f.maxLife) * f.power;
    return clampf(s, 0.0f, 2.0f);
}

void ScreenFx::draw(int sw, int sh, float t) const {
    for (size_t i = 0; i < fx_.size(); ++i) {
        const CardFx& f = fx_[i];
        float k = clampf(f.life / f.maxLife, 0.0f, 1.0f) * clampf(f.power, 0.4f, 1.6f);
        uint32_t seed = uint32_t(i * 7919u + uint32_t(eidx(f.a)) * 131u);

        // Ведущая стихия рисуется цветом реакции — так Огонь+Тень читается
        // именно как тёмный огонь, а не как оранжевое пламя рядом с тенью.
        // Цвет реакции сдвигает оттенок, но светимость возвращаем — иначе
        // тёмные реакции вроде Тёмного огня просто тонут в фоне.
        Color ca = f.reaction ? scaleC(lerpC(elem(f.a).primary, f.tint, 0.55f), 1.55f)
                              : lerpC(f.tint, WHITE, 0.10f);
        drawElementEdge(f.a, sw, sh, t, k, ca, seed);

        if (f.b != ElementId::None && f.b != f.a) {
            Color cb = f.reaction ? scaleC(lerpC(elem(f.b).primary, f.tint, 0.55f), 1.45f)
                                  : elem(f.b).primary;
            drawElementEdge(f.b, sw, sh, t, k * 0.72f, cb, seed + 3331u);
        }

        // У реакции — общий отсвет цветом реакции поверх всего
        if (f.reaction) {
            float e = envelope(k);
            DrawRectangle(0, 0, sw, sh, withA(f.tint, (unsigned char)(34 * e)));
            drawRuneRing(Vector2{ sw * 0.5f, sh * 0.40f }, sh * 0.34f * (1.4f - 0.4f * k),
                         12, withA(lerpC(f.tint, WHITE, 0.4f), (unsigned char)(150 * e)), t * 2.0f);
        }
    }
}

} // namespace art
} // namespace shichoy
