// Procedural app icon (3 KTV cards) + main-menu backdrop
// Процедурная иконка (3 карты KTV) и фон главного меню
// Chaos Spellweaver (Картовасия)

using System.Collections.Generic;
using UnityEngine;

namespace Kartovasiya.UI
{
    /// <summary>
    /// Brand art: bright three-card KTV logo and atmospheric menu background.
    /// Без внешних PNG — точные буквы K/T/V через bitmap-шрифт.
    /// </summary>
    public static class ProceduralBrandArt
    {
        private static readonly Dictionary<string, Sprite> Cache = new Dictionary<string, Sprite>();

        // ── Public sprites ────────────────────────────────────────────

        public static Sprite KtvLogo(int size = 384)
        {
            string key = $"ktv_logo_{size}";
            if (Cache.TryGetValue(key, out var s))
                return s;

            var tex = BuildKtvIconTexture(size, opaqueBackground: false);
            tex.Apply(false, true);
            s = Sprite.Create(tex, new Rect(0, 0, size, size), new Vector2(0.5f, 0.5f), 100f);
            Cache[key] = s;
            return s;
        }

        public static Sprite MenuBackdrop(int w = 270, int h = 480)
        {
            string key = $"menu_bg_{w}x{h}";
            if (Cache.TryGetValue(key, out var s))
                return s;

            var tex = BuildMenuBackdropTexture(w, h);
            tex.Apply(false, true);
            s = Sprite.Create(tex, new Rect(0, 0, w, h), new Vector2(0.5f, 0.5f), 100f);
            Cache[key] = s;
            return s;
        }

        /// <summary>
        /// Readable texture for baking AppIcon.png (call EncodeToPNG, then Destroy).
        /// </summary>
        public static Texture2D BuildKtvIconTexture(int size, bool opaqueBackground)
        {
            size = Mathf.Clamp(size, 48, 2048);
            var tex = new Texture2D(size, size, TextureFormat.RGBA32, false);
            tex.wrapMode = TextureWrapMode.Clamp;
            tex.filterMode = FilterMode.Bilinear;

            float cx = size * 0.5f;
            float cy = size * 0.5f;
            float glowR = size * 0.48f;

            // Card geometry (in icon-normalized units → pixels)
            float cardW = size * 0.28f;
            float cardH = size * 0.42f;
            float frontW = size * 0.32f;
            float frontH = size * 0.48f;

            // Colors — яркие «неон / таро»
            var bgDeep = new Color(0.06f, 0.03f, 0.12f, opaqueBackground ? 1f : 0f);
            var glowMag = new Color(0.95f, 0.15f, 0.55f, 1f);
            var glowCyan = new Color(0.2f, 0.85f, 1f, 1f);
            var gold = new Color(1f, 0.86f, 0.35f, 1f);
            var parchment = new Color(0.98f, 0.92f, 0.78f, 1f);

            // Side cards brighter fills
            var fillK = new Color(0.95f, 0.12f, 0.45f, 1f);   // hot pink
            var fillT = new Color(0.45f, 0.2f, 0.95f, 1f);    // electric violet (front)
            var fillV = new Color(0.1f, 0.75f, 0.95f, 1f);    // cyan

            // Draw back-to-front: left, right, then center
            // Pre-clear
            var pixels = new Color[size * size];
            for (int i = 0; i < pixels.Length; i++)
                pixels[i] = bgDeep;

            // Soft dual glow
            for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++)
            {
                float dx = x + 0.5f - cx;
                float dy = y + 0.5f - cy;
                float d = Mathf.Sqrt(dx * dx + dy * dy) / glowR;
                float g = Mathf.Clamp01(1f - d);
                g = g * g;
                var baseC = pixels[y * size + x];
                var glow = Color.Lerp(glowMag, glowCyan, 0.5f + 0.5f * Mathf.Sin(dx * 0.02f));
                glow.a = 1f;
                if (opaqueBackground)
                {
                    baseC = Color.Lerp(bgDeep, new Color(0.12f, 0.04f, 0.2f, 1f), g * 0.85f);
                    baseC = Color.Lerp(baseC, glow, g * 0.35f);
                    baseC.a = 1f;
                }
                else
                {
                    // Transparent outside, soft glow disc for UI logo
                    float a = Mathf.Clamp01(g * 1.15f);
                    baseC = Color.Lerp(Color.clear, new Color(glow.r, glow.g, glow.b, 0.55f), a * 0.65f);
                    if (d > 1.05f)
                        baseC = Color.clear;
                }

                pixels[y * size + x] = baseC;
            }

            // Left card — K, tilted -18°
            BlitCard(pixels, size,
                cx - size * 0.18f, cy + size * 0.02f,
                cardW, cardH, -18f,
                fillK, gold, parchment, 'K');

            // Right card — V, tilted +18°
            BlitCard(pixels, size,
                cx + size * 0.18f, cy + size * 0.02f,
                cardW, cardH, 18f,
                fillV, gold, parchment, 'V');

            // Front center — T, larger
            BlitCard(pixels, size,
                cx, cy - size * 0.01f,
                frontW, frontH, 0f,
                fillT, gold, parchment, 'T');

            // Outer gold ring for app-icon polish (opaque only)
            if (opaqueBackground)
            {
                float ringOuter = size * 0.49f;
                float ringInner = size * 0.455f;
                for (int y = 0; y < size; y++)
                for (int x = 0; x < size; x++)
                {
                    float dx = x + 0.5f - cx;
                    float dy = y + 0.5f - cy;
                    float d = Mathf.Sqrt(dx * dx + dy * dy);
                    if (d <= ringOuter && d >= ringInner)
                    {
                        float t = 1f - Mathf.Abs(d - (ringOuter + ringInner) * 0.5f) / ((ringOuter - ringInner) * 0.5f);
                        t = Mathf.Clamp01(t);
                        int i = y * size + x;
                        pixels[i] = Color.Lerp(pixels[i], gold, t * 0.9f);
                        pixels[i].a = 1f;
                    }
                    else if (d > ringOuter)
                    {
                        // Soft square crop residual → keep bg
                        pixels[y * size + x].a = 1f;
                    }
                }
            }

            tex.SetPixels(pixels);
            return tex;
        }

        public static Texture2D BuildMenuBackdropTexture(int w, int h)
        {
            w = Mathf.Clamp(w, 64, 1080);
            h = Mathf.Clamp(h, 64, 1920);
            var tex = new Texture2D(w, h, TextureFormat.RGBA32, false);
            tex.wrapMode = TextureWrapMode.Clamp;
            tex.filterMode = FilterMode.Bilinear;

            var top = new Color(0.18f, 0.05f, 0.28f, 1f);
            var mid = new Color(0.08f, 0.03f, 0.14f, 1f);
            var bot = new Color(0.03f, 0.02f, 0.07f, 1f);
            var neonM = new Color(1f, 0.2f, 0.55f, 1f);
            var neonC = new Color(0.25f, 0.9f, 1f, 1f);
            var gold = new Color(1f, 0.82f, 0.35f, 1f);

            var pixels = new Color[w * h];

            for (int y = 0; y < h; y++)
            {
                float v = y / (float)(h - 1);
                // vertical gradient (bottom dark)
                Color col = v < 0.55f
                    ? Color.Lerp(bot, mid, v / 0.55f)
                    : Color.Lerp(mid, top, (v - 0.55f) / 0.45f);

                for (int x = 0; x < w; x++)
                {
                    float u = x / (float)(w - 1);
                    float nx = (u - 0.5f) * 2f;
                    float ny = (v - 0.5f) * 2f;

                    // Radial purple bloom upper-center
                    float dBloom = Mathf.Sqrt(nx * nx + (ny - 0.35f) * (ny - 0.35f));
                    col = Color.Lerp(col, new Color(0.45f, 0.08f, 0.35f, 1f), Mathf.Clamp01(1f - dBloom) * 0.45f);

                    // Diagonal neon streaks (HM / KZ vibe)
                    float streak1 = Mathf.Abs(nx * 0.7f + ny * 1.2f + 0.15f);
                    float streak2 = Mathf.Abs(nx * 1.1f - ny * 0.6f - 0.4f);
                    if (streak1 < 0.04f)
                        col = Color.Lerp(col, neonM, (1f - streak1 / 0.04f) * 0.35f);
                    if (streak2 < 0.03f)
                        col = Color.Lerp(col, neonC, (1f - streak2 / 0.03f) * 0.28f);

                    // Soft arcane circles
                    float ring = Mathf.Abs(Mathf.Sqrt(nx * nx * 0.7f + (ny + 0.1f) * (ny + 0.1f)) - 0.55f);
                    if (ring < 0.03f)
                        col = Color.Lerp(col, gold, (1f - ring / 0.03f) * 0.2f);

                    // Gold dust / stars (deterministic)
                    float n = Hash01(x * 374761 + y * 668265);
                    if (n > 0.992f)
                    {
                        float spark = (n - 0.992f) / 0.008f;
                        col = Color.Lerp(col, gold, spark * 0.85f);
                    }
                    else if (n > 0.985f)
                    {
                        col = Color.Lerp(col, neonC, 0.35f);
                    }

                    // Vignette
                    float vig = Mathf.Sqrt(nx * nx * 0.85f + ny * ny);
                    col = Color.Lerp(col, Color.black, Mathf.SmoothStep(0.75f, 1.35f, vig) * 0.55f);

                    pixels[y * w + x] = col;
                }
            }

            // Faint floating card silhouettes in the background
            BlitFaintCard(pixels, w, h, 0.22f, 0.62f, 0.12f, 0.18f, -22f, neonM);
            BlitFaintCard(pixels, w, h, 0.78f, 0.55f, 0.11f, 0.17f, 18f, neonC);
            BlitFaintCard(pixels, w, h, 0.5f, 0.78f, 0.14f, 0.2f, 4f, gold);
            BlitFaintCard(pixels, w, h, 0.15f, 0.28f, 0.1f, 0.15f, -10f, neonC);
            BlitFaintCard(pixels, w, h, 0.85f, 0.3f, 0.1f, 0.15f, 12f, neonM);

            tex.SetPixels(pixels);
            return tex;
        }

        // ── Card blitting ─────────────────────────────────────────────

        private static void BlitCard(
            Color[] pixels, int size,
            float cx, float cy, float cardW, float cardH, float angleDeg,
            Color fill, Color border, Color letterColor, char letter)
        {
            float halfW = cardW * 0.5f;
            float halfH = cardH * 0.5f;
            float rad = angleDeg * Mathf.Deg2Rad;
            float cos = Mathf.Cos(rad);
            float sin = Mathf.Sin(rad);
            // Inverse rotation
            float icos = Mathf.Cos(-rad);
            float isin = Mathf.Sin(-rad);

            float borderT = Mathf.Max(2f, size * 0.012f);
            float radius = Mathf.Max(4f, size * 0.025f);

            // Bounding box
            float extent = Mathf.Sqrt(halfW * halfW + halfH * halfH) + borderT + 2f;
            int x0 = Mathf.Max(0, Mathf.FloorToInt(cx - extent));
            int x1 = Mathf.Min(size - 1, Mathf.CeilToInt(cx + extent));
            int y0 = Mathf.Max(0, Mathf.FloorToInt(cy - extent));
            int y1 = Mathf.Min(size - 1, Mathf.CeilToInt(cy + extent));

            // Soft drop shadow
            float shadowOx = size * 0.012f;
            float shadowOy = -size * 0.018f;
            for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++)
            {
                float dx = x + 0.5f - (cx + shadowOx);
                float dy = y + 0.5f - (cy + shadowOy);
                float lx = dx * icos - dy * isin;
                float ly = dx * isin + dy * icos;
                if (InsideRoundRect(lx, ly, halfW, halfH, radius))
                {
                    int i = y * size + x;
                    var sh = pixels[i];
                    sh.r *= 0.55f;
                    sh.g *= 0.55f;
                    sh.b *= 0.65f;
                    sh.a = Mathf.Max(sh.a, 0.65f);
                    pixels[i] = sh;
                }
            }

            for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++)
            {
                float dx = x + 0.5f - cx;
                float dy = y + 0.5f - cy;
                float lx = dx * icos - dy * isin;
                float ly = dx * isin + dy * icos;

                if (!InsideRoundRect(lx, ly, halfW, halfH, radius))
                    continue;

                bool onBorder = !InsideRoundRect(lx, ly, halfW - borderT, halfH - borderT, Mathf.Max(0f, radius - borderT));

                // Inner gradient (bright top)
                float gv = Mathf.InverseLerp(-halfH, halfH, ly);
                Color body = Color.Lerp(fill * 0.55f, fill, 0.35f + gv * 0.65f);
                // Inner glow disc
                float id = Mathf.Sqrt((lx / halfW) * (lx / halfW) + (ly / halfH) * (ly / halfH));
                body = Color.Lerp(body, Color.white, Mathf.Clamp01(1f - id) * 0.18f);

                Color c = onBorder ? border : body;

                // Decorative top banner stripe
                if (!onBorder && ly > halfH * 0.55f && ly < halfH * 0.78f)
                    c = Color.Lerp(c, border, 0.55f);

                // Letter stays upright (counter-rotate into screen space)
                if (!onBorder)
                {
                    // Screen-space offset from card center, unrotated
                    float sx = dx;
                    float sy = dy;
                    float letterScale = Mathf.Min(halfW, halfH) * 0.95f;
                    if (SampleLetter(letter, sx / letterScale, sy / letterScale * 0.85f))
                        c = letterColor;
                    else if (SampleLetter(letter, sx / letterScale, sy / letterScale * 0.85f, outline: true))
                        c = Color.Lerp(border, letterColor, 0.35f);
                }

                // Specular edge
                if (onBorder)
                {
                    float edgeShine = Mathf.Clamp01(0.5f + 0.5f * Mathf.Sin((lx + ly) * 0.08f));
                    c = Color.Lerp(c, Color.white, edgeShine * 0.25f);
                }

                c.a = 1f;
                pixels[y * size + x] = c;
            }

            // Tiny corner pips (ornament)
            DrawPip(pixels, size, cx, cy, icos, isin, -halfW * 0.62f, halfH * 0.72f, borderT * 0.9f, border);
            DrawPip(pixels, size, cx, cy, icos, isin, halfW * 0.62f, -halfH * 0.72f, borderT * 0.9f, border);
        }

        private static void DrawPip(
            Color[] pixels, int size,
            float cx, float cy, float icos, float isin,
            float lx, float ly, float r, Color col)
        {
            // local → world via forward rotation (inverse of inverse)
            // We have icos/isin for inverse; forward is cos=icos of +angle = icos of -(-angle)...
            // Stored isin = sin(-a), icos = cos(-a). Forward: cos(a)=icos, sin(a)=-isin
            float fcos = icos;
            float fsin = -isin;
            float wx = cx + lx * fcos - ly * fsin;
            float wy = cy + lx * fsin + ly * fcos;
            int x0 = Mathf.Max(0, Mathf.FloorToInt(wx - r - 1));
            int x1 = Mathf.Min(size - 1, Mathf.CeilToInt(wx + r + 1));
            int y0 = Mathf.Max(0, Mathf.FloorToInt(wy - r - 1));
            int y1 = Mathf.Min(size - 1, Mathf.CeilToInt(wy + r + 1));
            float r2 = r * r;
            for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++)
            {
                float dx = x + 0.5f - wx;
                float dy = y + 0.5f - wy;
                if (dx * dx + dy * dy <= r2)
                    pixels[y * size + x] = col;
            }
        }

        private static void BlitFaintCard(
            Color[] pixels, int w, int h,
            float u, float v, float uw, float vh, float angleDeg, Color tint)
        {
            float cx = u * (w - 1);
            float cy = v * (h - 1);
            float halfW = uw * w * 0.5f;
            float halfH = vh * h * 0.5f;
            float rad = angleDeg * Mathf.Deg2Rad;
            float icos = Mathf.Cos(-rad);
            float isin = Mathf.Sin(-rad);
            float extent = Mathf.Sqrt(halfW * halfW + halfH * halfH) + 2f;
            int x0 = Mathf.Max(0, Mathf.FloorToInt(cx - extent));
            int x1 = Mathf.Min(w - 1, Mathf.CeilToInt(cx + extent));
            int y0 = Mathf.Max(0, Mathf.FloorToInt(cy - extent));
            int y1 = Mathf.Min(h - 1, Mathf.CeilToInt(cy + extent));
            float radius = Mathf.Min(halfW, halfH) * 0.12f;

            for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++)
            {
                float dx = x + 0.5f - cx;
                float dy = y + 0.5f - cy;
                float lx = dx * icos - dy * isin;
                float ly = dx * isin + dy * icos;
                if (!InsideRoundRect(lx, ly, halfW, halfH, radius))
                    continue;

                bool border = !InsideRoundRect(lx, ly, halfW * 0.88f, halfH * 0.9f, radius * 0.7f);
                int i = y * w + x;
                float a = border ? 0.22f : 0.08f;
                pixels[i] = Color.Lerp(pixels[i], tint, a);
            }
        }

        private static bool InsideRoundRect(float lx, float ly, float halfW, float halfH, float radius)
        {
            float ax = Mathf.Abs(lx);
            float ay = Mathf.Abs(ly);
            if (ax > halfW || ay > halfH) return false;
            if (ax <= halfW - radius || ay <= halfH - radius) return true;
            float cx = halfW - radius;
            float cy = halfH - radius;
            float dx = ax - cx;
            float dy = ay - cy;
            return dx * dx + dy * dy <= radius * radius;
        }

        // ── Bitmap letters K / T / V (7×9, # = ink) ───────────────────

        private static readonly string[] GlyphK =
        {
            "#.....#",
            "#....#.",
            "#...#..",
            "#..#...",
            "###....",
            "#..#...",
            "#...#..",
            "#....#.",
            "#.....#",
        };

        private static readonly string[] GlyphT =
        {
            "#######",
            "#######",
            "...#...",
            "...#...",
            "...#...",
            "...#...",
            "...#...",
            "...#...",
            "...#...",
        };

        private static readonly string[] GlyphV =
        {
            "#.....#",
            "#.....#",
            "#.....#",
            ".#...#.",
            ".#...#.",
            "..#.#..",
            "..#.#..",
            "...#...",
            "...#...",
        };

        private static bool SampleLetter(char ch, float nx, float ny, bool outline = false)
        {
            // nx, ny in roughly [-0.5, 0.5] centered
            string[] g = ch switch
            {
                'K' or 'k' => GlyphK,
                'T' or 't' => GlyphT,
                'V' or 'v' => GlyphV,
                _ => null
            };
            if (g == null) return false;

            const int gw = 7;
            const int gh = 9;
            // Map local to glyph: letter sits center of card
            float u = (nx + 0.42f) / 0.84f; // 0..1
            float v = (ny + 0.48f) / 0.96f;
            if (u < 0f || u > 1f || v < 0f || v > 1f) return false;

            // v: 0 bottom → top of glyph (texture y up, glyph row 0 is top)
            float gx = u * gw;
            float gy = (1f - v) * gh;
            int ix = Mathf.FloorToInt(gx);
            int iy = Mathf.FloorToInt(gy);
            if (ix < 0 || iy < 0 || ix >= gw || iy >= gh) return false;

            bool solid = g[iy][ix] == '#';
            if (!outline) return solid;

            if (solid) return false;
            // 4-neighbour outline
            for (int oy = -1; oy <= 1; oy++)
            for (int ox = -1; ox <= 1; ox++)
            {
                if (ox == 0 && oy == 0) continue;
                int sx = ix + ox;
                int sy = iy + oy;
                if (sx < 0 || sy < 0 || sx >= gw || sy >= gh) continue;
                if (g[sy][sx] == '#') return true;
            }

            return false;
        }

        private static float Hash01(int n)
        {
            unchecked
            {
                uint x = (uint)n;
                x ^= x >> 13;
                x *= 0x5bd1e995u;
                x ^= x >> 15;
                return (x & 0xFFFF) / 65535f;
            }
        }
    }
}
