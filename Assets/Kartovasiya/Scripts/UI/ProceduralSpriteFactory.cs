// Runtime sprites for tarot frames (no external art required)
// Runtime-спрайты рамок Таро (без внешних ассетов)
// Chaos Spellweaver (Картовасия)

using System.Collections.Generic;
using UnityEngine;

namespace Kartovasiya.UI
{
    /// <summary>
    /// Generates and caches simple ornamental UI sprites (frames, discs, vignettes).
    /// Генерирует и кэширует орнаментальные UI-спрайты.
    /// </summary>
    public static class ProceduralSpriteFactory
    {
        private static readonly Dictionary<string, Sprite> Cache = new Dictionary<string, Sprite>();

        public static Sprite RoundedRect(int w, int h, int radius, Color fill, Color border, int borderW = 3)
        {
            string key = $"rr_{w}x{h}_r{radius}_b{borderW}_{ColorKey(fill)}_{ColorKey(border)}";
            if (Cache.TryGetValue(key, out var s))
                return s;

            var tex = new Texture2D(w, h, TextureFormat.RGBA32, false);
            tex.wrapMode = TextureWrapMode.Clamp;
            tex.filterMode = FilterMode.Bilinear;

            for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                bool inside = InsideRounded(x, y, w, h, radius);
                bool insideInner = InsideRounded(x, y, w, h, Mathf.Max(0, radius - borderW));
                Color c = Color.clear;
                if (inside)
                {
                    if (borderW > 0 && !insideInner)
                        c = border;
                    else
                        c = fill;
                }

                tex.SetPixel(x, y, c);
            }

            tex.Apply(false, true);
            s = Sprite.Create(tex, new Rect(0, 0, w, h), new Vector2(0.5f, 0.5f), 100f);
            Cache[key] = s;
            return s;
        }

        public static Sprite Disc(int size, Color fill, Color rim, int rimW = 4)
        {
            string key = $"disc_{size}_{rimW}_{ColorKey(fill)}_{ColorKey(rim)}";
            if (Cache.TryGetValue(key, out var s))
                return s;

            var tex = new Texture2D(size, size, TextureFormat.RGBA32, false);
            tex.filterMode = FilterMode.Bilinear;
            float r = size * 0.5f - 0.5f;
            float cx = size * 0.5f;
            float cy = size * 0.5f;

            for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++)
            {
                float d = Vector2.Distance(new Vector2(x + 0.5f, y + 0.5f), new Vector2(cx, cy));
                Color c = Color.clear;
                if (d <= r)
                {
                    c = d > r - rimW ? rim : fill;
                    // soft edge
                    float edge = Mathf.Clamp01((r - d) * 2f);
                    c.a *= edge;
                }

                tex.SetPixel(x, y, c);
            }

            tex.Apply(false, true);
            s = Sprite.Create(tex, new Rect(0, 0, size, size), new Vector2(0.5f, 0.5f), 100f);
            Cache[key] = s;
            return s;
        }

        public static Sprite Vignette(int w, int h, Color center, Color edge)
        {
            string key = $"vig_{w}x{h}_{ColorKey(center)}_{ColorKey(edge)}";
            if (Cache.TryGetValue(key, out var s))
                return s;

            var tex = new Texture2D(w, h, TextureFormat.RGBA32, false);
            float cx = (w - 1) * 0.5f;
            float cy = (h - 1) * 0.5f;
            float maxD = Mathf.Sqrt(cx * cx + cy * cy);

            for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                float d = Vector2.Distance(new Vector2(x, y), new Vector2(cx, cy)) / maxD;
                d = Mathf.SmoothStep(0f, 1f, d);
                tex.SetPixel(x, y, Color.Lerp(center, edge, d));
            }

            tex.Apply(false, true);
            s = Sprite.Create(tex, new Rect(0, 0, w, h), new Vector2(0.5f, 0.5f), 100f);
            Cache[key] = s;
            return s;
        }

        /// <summary>Soft radial glow for VFX. / Мягкое радиальное свечение.</summary>
        public static Sprite SoftGlow(int size, Color color)
        {
            string key = $"glow_{size}_{ColorKey(color)}";
            if (Cache.TryGetValue(key, out var s))
                return s;

            var tex = new Texture2D(size, size, TextureFormat.RGBA32, false);
            float r = size * 0.5f;
            float cx = r;
            float cy = r;

            for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++)
            {
                float d = Vector2.Distance(new Vector2(x + 0.5f, y + 0.5f), new Vector2(cx, cy)) / r;
                float a = Mathf.Clamp01(1f - d);
                a = a * a;
                var c = color;
                c.a = a * color.a;
                tex.SetPixel(x, y, c);
            }

            tex.Apply(false, true);
            s = Sprite.Create(tex, new Rect(0, 0, size, size), new Vector2(0.5f, 0.5f), 100f);
            Cache[key] = s;
            return s;
        }

        private static bool InsideRounded(int x, int y, int w, int h, int radius)
        {
            if (radius <= 0)
                return true;

            int r = radius;
            // center rect
            if (x >= r && x < w - r && y >= 0 && y < h) return true;
            if (y >= r && y < h - r && x >= 0 && x < w) return true;

            // corners
            if (InCorner(x, y, r, r, r)) return true;
            if (InCorner(x, y, w - 1 - r, r, r)) return true;
            if (InCorner(x, y, r, h - 1 - r, r)) return true;
            if (InCorner(x, y, w - 1 - r, h - 1 - r, r)) return true;
            return false;
        }

        private static bool InCorner(int x, int y, int cx, int cy, int r)
        {
            int dx = x - cx;
            int dy = y - cy;
            return dx * dx + dy * dy <= r * r;
        }

        private static string ColorKey(Color c) =>
            $"{(int)(c.r * 255)}_{(int)(c.g * 255)}_{(int)(c.b * 255)}_{(int)(c.a * 255)}";
    }
}
