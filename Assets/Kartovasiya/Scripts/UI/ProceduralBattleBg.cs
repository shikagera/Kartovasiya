// Dark fantasy / Tarot battle backdrop (magic circle + ruins vibe)
// Фон боя: тёмный магический круг
// Chaos Spellweaver (Картовасия)

using UnityEngine;

namespace Kartovasiya.UI
{
    public static class ProceduralBattleBg
    {
        private static Sprite _cached;

        public static Sprite Get()
        {
            if (_cached != null) return _cached;
            _cached = Build(540, 960);
            return _cached;
        }

        private static Sprite Build(int w, int h)
        {
            var px = new Color[w * h];
            var night = new Color(0.04f, 0.03f, 0.07f, 1f);
            var purple = new Color(0.12f, 0.05f, 0.16f, 1f);
            var gold = TarotPalette.Gold;
            var goldDim = new Color(0.45f, 0.35f, 0.15f, 1f);

            // Vertical gradient night → purple mist mid
            for (int y = 0; y < h; y++)
            {
                float v = y / (float)(h - 1);
                Color col = v < 0.45f
                    ? Color.Lerp(night, purple, v / 0.45f)
                    : Color.Lerp(purple, new Color(0.08f, 0.04f, 0.12f), (v - 0.45f) / 0.55f);
                for (int x = 0; x < w; x++)
                {
                    float u = x / (float)(w - 1);
                    // soft side darkening
                    float side = Mathf.Abs(u - 0.5f) * 2f;
                    col = Color.Lerp(col, night, side * 0.25f);
                    px[y * w + x] = col;
                }
            }

            // Magic circle in lower half (under player)
            float cx = w * 0.5f;
            float cy = h * 0.28f;
            float r = w * 0.38f;
            DrawCircle(px, w, h, cx, cy, r, goldDim, 3f, 0.35f);
            DrawCircle(px, w, h, cx, cy, r * 0.82f, gold, 2f, 0.28f);
            DrawCircle(px, w, h, cx, cy, r * 0.55f, goldDim, 1.5f, 0.22f);
            // Runes on ring
            for (int i = 0; i < 12; i++)
            {
                float a = i / 12f * Mathf.PI * 2f;
                float x = cx + Mathf.Cos(a) * r * 0.9f;
                float y = cy + Mathf.Sin(a) * r * 0.9f;
                FillDot(px, w, h, x, y, 3f, gold, 0.5f);
            }

            // Radial spokes
            for (int i = 0; i < 8; i++)
            {
                float a = i / 8f * Mathf.PI * 2f;
                DrawLine(px, w, h, cx, cy,
                    cx + Mathf.Cos(a) * r * 0.78f,
                    cy + Mathf.Sin(a) * r * 0.78f,
                    goldDim, 1.2f, 0.18f);
            }

            // Upper ruined arch hint (behind enemy)
            float topY = h * 0.82f;
            DrawLine(px, w, h, w * 0.12f, topY, w * 0.88f, topY, goldDim, 2f, 0.2f);
            for (int i = 0; i < 5; i++)
            {
                float x = w * (0.2f + i * 0.15f);
                DrawLine(px, w, h, x, topY, x, topY + h * 0.1f, goldDim, 2f, 0.15f);
            }

            // Floating dust
            var rng = new System.Random(42);
            for (int i = 0; i < 80; i++)
            {
                int x = rng.Next(0, w);
                int y = rng.Next(0, h);
                float a = 0.08f + (float)rng.NextDouble() * 0.2f;
                FillDot(px, w, h, x, y, 1.2f, gold, a);
            }

            // Vignette
            for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                float u = (x / (float)(w - 1) - 0.5f) * 2f;
                float v = (y / (float)(h - 1) - 0.5f) * 2f;
                float d = Mathf.Sqrt(u * u * 0.7f + v * v);
                float vig = Mathf.SmoothStep(0.75f, 1.35f, d) * 0.55f;
                int i = y * w + x;
                px[i] = Color.Lerp(px[i], Color.black, vig);
                px[i].a = 1f;
            }

            var tex = new Texture2D(w, h, TextureFormat.RGBA32, false);
            tex.filterMode = FilterMode.Bilinear;
            tex.SetPixels(px);
            tex.Apply(false, true);
            return Sprite.Create(tex, new Rect(0, 0, w, h), new Vector2(0.5f, 0.5f), 100f);
        }

        private static void DrawCircle(Color[] px, int w, int h, float cx, float cy, float r, Color c, float thickness, float alpha)
        {
            int steps = Mathf.Max(64, (int)(r * 4f));
            for (int i = 0; i < steps; i++)
            {
                float a0 = i / (float)steps * Mathf.PI * 2f;
                float a1 = (i + 1) / (float)steps * Mathf.PI * 2f;
                DrawLine(px, w, h,
                    cx + Mathf.Cos(a0) * r, cy + Mathf.Sin(a0) * r,
                    cx + Mathf.Cos(a1) * r, cy + Mathf.Sin(a1) * r,
                    c, thickness, alpha);
            }
        }

        private static void DrawLine(Color[] px, int w, int h, float x0, float y0, float x1, float y1, Color c, float thickness, float alpha)
        {
            float dx = x1 - x0, dy = y1 - y0;
            float len = Mathf.Sqrt(dx * dx + dy * dy);
            int steps = Mathf.Max(1, (int)len);
            var col = c; col.a = alpha;
            for (int i = 0; i <= steps; i++)
            {
                float t = i / (float)steps;
                FillDot(px, w, h, x0 + dx * t, y0 + dy * t, thickness * 0.5f, col, alpha);
            }
        }

        private static void FillDot(Color[] px, int w, int h, float cx, float cy, float r, Color c, float alpha)
        {
            var col = c; col.a = alpha;
            int x0 = Mathf.Max(0, (int)(cx - r - 1));
            int x1 = Mathf.Min(w - 1, (int)(cx + r + 1));
            int y0 = Mathf.Max(0, (int)(cy - r - 1));
            int y1 = Mathf.Min(h - 1, (int)(cy + r + 1));
            float r2 = r * r;
            for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++)
            {
                float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
                if (dx * dx + dy * dy > r2) continue;
                int i = y * w + x;
                px[i] = Color.Lerp(px[i], col, col.a);
                px[i].a = 1f;
            }
        }
    }
}
