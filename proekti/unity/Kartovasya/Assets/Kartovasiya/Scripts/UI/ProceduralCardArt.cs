// Beautiful 2D procedural card illustrations (elements / types)
// Красивые 2D-иллюстрации карт без внешних ассетов
// Chaos Spellweaver (Картовасия)

using System.Collections.Generic;
using UnityEngine;
using Kartovasiya.Core;

namespace Kartovasiya.UI
{
    /// <summary>
    /// Runtime 2D art plates for cards — stylized flat fantasy icons.
    /// </summary>
    public static class ProceduralCardArt
    {
        private static readonly Dictionary<string, Sprite> Cache = new Dictionary<string, Sprite>();
        private const int Size = 256;

        public static Sprite ForElement(ElementId id)
        {
            string key = "el_" + (int)id;
            if (Cache.TryGetValue(key, out var s))
                return s;
            s = BuildElementArt(id);
            Cache[key] = s;
            return s;
        }

        public static Sprite ForCardType(CardType type, ElementId accent)
        {
            string key = "ct_" + (int)type + "_" + (int)accent;
            if (Cache.TryGetValue(key, out var s))
                return s;

            s = type switch
            {
                CardType.Element => ForElement(accent),
                CardType.Effect => BuildEffectArt(),
                CardType.Material => BuildMaterialArt(),
                _ => BuildSpellArt(accent)
            };
            Cache[key] = s;
            return s;
        }

        private static Sprite BuildElementArt(ElementId id)
        {
            var px = NewCanvas(out var bgDeep, out var bgMid);
            Color accent = TarotPalette.ForElement(id);
            Color glow = Color.Lerp(accent, Color.white, 0.35f);
            Color deep = Color.Lerp(accent, Color.black, 0.55f);

            // Soft radial atmosphere
            FillRadial(px, Size * 0.5f, Size * 0.5f, Size * 0.55f, deep, bgDeep);
            FillRadial(px, Size * 0.5f, Size * 0.42f, Size * 0.32f, accent * 0.55f, Color.clear);

            switch (id)
            {
                case ElementId.Fire:
                    DrawFlames(px, accent, glow);
                    break;
                case ElementId.Water:
                    DrawWaves(px, accent, glow);
                    break;
                case ElementId.Earth:
                    DrawMountains(px, accent, glow);
                    break;
                case ElementId.Air:
                    DrawSwirls(px, accent, glow);
                    break;
                case ElementId.Lightning:
                    DrawBolt(px, accent, glow);
                    break;
                case ElementId.Ice:
                    DrawCrystal(px, accent, glow);
                    break;
                case ElementId.Nature:
                    DrawLeaf(px, accent, glow);
                    break;
                case ElementId.Shadow:
                    DrawCrescent(px, accent, glow);
                    break;
                case ElementId.Light:
                    DrawSun(px, accent, glow);
                    break;
                case ElementId.Metal:
                    DrawShield(px, accent, glow);
                    break;
                case ElementId.Poison:
                    DrawDroplet(px, accent, glow, skull: true);
                    break;
                case ElementId.Aether:
                    DrawStarRing(px, accent, glow);
                    break;
                case ElementId.Blood:
                    DrawDroplet(px, accent, glow, skull: false);
                    break;
                case ElementId.Storm:
                    DrawCloudBolt(px, accent, glow);
                    break;
                case ElementId.Crystal:
                    DrawGem(px, accent, glow);
                    break;
                case ElementId.Arcane:
                    DrawPentacle(px, accent, glow);
                    break;
                default:
                    DrawStarRing(px, TarotPalette.Gold, TarotPalette.GoldBright);
                    break;
            }

            // Soft vignette frame
            ApplyVignette(px, 0.22f);
            return ToSprite(px, "el_" + id);
        }

        private static Sprite BuildSpellArt(ElementId accent)
        {
            var px = NewCanvas(out _, out _);
            Color a = TarotPalette.ForElement(accent);
            Color deep = Color.Lerp(TarotPalette.VoidPurple, a, 0.4f);
            FillRadial(px, Size * 0.5f, Size * 0.5f, Size * 0.6f, deep, TarotPalette.Night);

            // Ornate open book / sigil plate
            DrawRoundedRect(px, Size * 0.22f, Size * 0.28f, Size * 0.56f, Size * 0.48f, 18, a * 0.35f, TarotPalette.Gold);
            DrawPentacle(px, a, Color.Lerp(a, Color.white, 0.4f));
            // Small floating runes
            for (int i = 0; i < 6; i++)
            {
                float ang = i / 6f * Mathf.PI * 2f;
                float cx = Size * 0.5f + Mathf.Cos(ang) * Size * 0.28f;
                float cy = Size * 0.5f + Mathf.Sin(ang) * Size * 0.28f;
                FillCircle(px, cx, cy, 5f, TarotPalette.GoldBright);
            }

            ApplyVignette(px, 0.28f);
            return ToSprite(px, "spell_" + accent);
        }

        private static Sprite BuildEffectArt()
        {
            var px = NewCanvas(out _, out _);
            FillRadial(px, Size * 0.5f, Size * 0.5f, Size * 0.55f,
                new Color(0.12f, 0.28f, 0.16f), TarotPalette.Night);
            // 4-point star (buff marker)
            DrawStar4(px, Size * 0.5f, Size * 0.5f, Size * 0.22f, Size * 0.08f, TarotPalette.GoldBright);
            DrawStar4(px, Size * 0.5f, Size * 0.5f, Size * 0.12f, Size * 0.04f, Color.white);
            ApplyVignette(px, 0.25f);
            return ToSprite(px, "effect");
        }

        private static Sprite BuildMaterialArt()
        {
            var px = NewCanvas(out _, out _);
            FillRadial(px, Size * 0.5f, Size * 0.5f, Size * 0.5f,
                new Color(0.25f, 0.18f, 0.1f), TarotPalette.Night);
            DrawGem(px, TarotPalette.Gold, TarotPalette.GoldBright);
            ApplyVignette(px, 0.25f);
            return ToSprite(px, "mat");
        }

        // ── Motifs ────────────────────────────────────────────────────

        private static void DrawFlames(Color[] px, Color accent, Color glow)
        {
            // Layered flame tongues
            for (int i = 0; i < 5; i++)
            {
                float x = Size * (0.32f + i * 0.09f);
                float h = Size * (0.22f + (i % 3) * 0.08f);
                float y0 = Size * 0.28f;
                DrawFlameTongue(px, x, y0, Size * 0.08f, h, Color.Lerp(accent, glow, i * 0.12f));
            }

            FillCircle(px, Size * 0.5f, Size * 0.32f, Size * 0.12f, glow * 0.8f);
        }

        private static void DrawFlameTongue(Color[] px, float cx, float y0, float w, float h, Color c)
        {
            int steps = 40;
            for (int i = 0; i < steps; i++)
            {
                float t = i / (float)steps;
                float y = y0 + t * h;
                float wave = Mathf.Sin(t * Mathf.PI * 3f + cx) * w * 0.25f;
                float half = w * (1f - t) * (0.6f + 0.4f * Mathf.Sin(t * Mathf.PI));
                float a = (1f - t) * 0.95f;
                var col = c;
                col.a = a;
                FillCircle(px, cx + wave, y, half, col);
            }
        }

        private static void DrawWaves(Color[] px, Color accent, Color glow)
        {
            for (int row = 0; row < 5; row++)
            {
                float y = Size * (0.30f + row * 0.09f);
                float amp = Size * (0.04f + row * 0.005f);
                Color c = Color.Lerp(accent, glow, row * 0.12f);
                for (int x = 20; x < Size - 20; x++)
                {
                    float yy = y + Mathf.Sin(x * 0.08f + row) * amp;
                    FillCircle(px, x, yy, 3.2f - row * 0.2f, c);
                }
            }

            // Droplet
            FillCircle(px, Size * 0.5f, Size * 0.62f, Size * 0.1f, glow);
            FillCircle(px, Size * 0.5f, Size * 0.72f, Size * 0.06f, Color.Lerp(glow, Color.white, 0.3f));
        }

        private static void DrawMountains(Color[] px, Color accent, Color glow)
        {
            DrawTriangle(px, Size * 0.5f, Size * 0.78f, Size * 0.18f, Size * 0.22f, Size * 0.82f, Size * 0.22f, accent);
            DrawTriangle(px, Size * 0.32f, Size * 0.68f, Size * 0.08f, Size * 0.28f, Size * 0.56f, Size * 0.28f, Color.Lerp(accent, deepGreen(), 0.3f));
            DrawTriangle(px, Size * 0.68f, Size * 0.70f, Size * 0.48f, Size * 0.30f, Size * 0.88f, Size * 0.30f, Color.Lerp(accent, glow, 0.25f));
            // Snow caps
            DrawTriangle(px, Size * 0.5f, Size * 0.78f, Size * 0.40f, Size * 0.58f, Size * 0.60f, Size * 0.58f, glow);
        }

        private static Color deepGreen() => new Color(0.25f, 0.35f, 0.2f);

        private static void DrawSwirls(Color[] px, Color accent, Color glow)
        {
            for (int s = 0; s < 3; s++)
            {
                float cx = Size * (0.35f + s * 0.15f);
                float cy = Size * (0.45f + (s % 2) * 0.1f);
                for (int i = 0; i < 48; i++)
                {
                    float t = i / 48f;
                    float ang = t * Mathf.PI * 3.2f + s;
                    float r = Size * (0.04f + t * 0.16f);
                    float x = cx + Mathf.Cos(ang) * r;
                    float y = cy + Mathf.Sin(ang) * r * 0.7f;
                    FillCircle(px, x, y, 3.5f - t * 2f, Color.Lerp(accent, glow, t));
                }
            }
        }

        private static void DrawBolt(Color[] px, Color accent, Color glow)
        {
            // Zigzag lightning
            Vector2[] pts =
            {
                new Vector2(0.55f, 0.82f),
                new Vector2(0.42f, 0.60f),
                new Vector2(0.55f, 0.55f),
                new Vector2(0.38f, 0.28f),
                new Vector2(0.52f, 0.22f)
            };
            for (int i = 0; i < pts.Length - 1; i++)
            {
                DrawThickLine(px,
                    pts[i].x * Size, pts[i].y * Size,
                    pts[i + 1].x * Size, pts[i + 1].y * Size,
                    7f - i, Color.Lerp(glow, accent, i * 0.2f));
            }

            FillCircle(px, Size * 0.42f, Size * 0.60f, 8f, Color.white);
        }

        private static void DrawCrystal(Color[] px, Color accent, Color glow)
        {
            // Hex crystal
            float cx = Size * 0.5f, cy = Size * 0.5f, r = Size * 0.22f;
            for (int i = 0; i < 6; i++)
            {
                float a0 = i / 6f * Mathf.PI * 2f - Mathf.PI / 2f;
                float a1 = (i + 1) / 6f * Mathf.PI * 2f - Mathf.PI / 2f;
                DrawTriangle(px, cx, cy,
                    cx + Mathf.Cos(a0) * r, cy + Mathf.Sin(a0) * r,
                    cx + Mathf.Cos(a1) * r, cy + Mathf.Sin(a1) * r,
                    Color.Lerp(accent, glow, i % 2 == 0 ? 0.4f : 0.1f));
            }

            FillCircle(px, cx, cy + r * 0.15f, r * 0.25f, Color.Lerp(glow, Color.white, 0.5f));
        }

        private static void DrawLeaf(Color[] px, Color accent, Color glow)
        {
            float cx = Size * 0.5f, cy = Size * 0.48f;
            for (int i = 0; i < 30; i++)
            {
                float t = i / 30f;
                float y = cy - Size * 0.22f + t * Size * 0.48f;
                float w = Mathf.Sin(t * Mathf.PI) * Size * 0.16f;
                FillCircle(px, cx, y, w, Color.Lerp(accent, glow, t * 0.4f));
            }

            DrawThickLine(px, cx, cy - Size * 0.2f, cx, cy + Size * 0.24f, 3f, deepGreen());
        }

        private static void DrawCrescent(Color[] px, Color accent, Color glow)
        {
            FillCircle(px, Size * 0.52f, Size * 0.5f, Size * 0.22f, accent);
            FillCircle(px, Size * 0.62f, Size * 0.54f, Size * 0.18f, TarotPalette.Night);
            // Stars
            for (int i = 0; i < 8; i++)
            {
                float x = Size * (0.2f + Hash01(i * 17) * 0.6f);
                float y = Size * (0.25f + Hash01(i * 31) * 0.55f);
                FillCircle(px, x, y, 2f + Hash01(i) * 2f, glow);
            }
        }

        private static void DrawSun(Color[] px, Color accent, Color glow)
        {
            FillCircle(px, Size * 0.5f, Size * 0.5f, Size * 0.16f, glow);
            FillCircle(px, Size * 0.5f, Size * 0.5f, Size * 0.10f, Color.white);
            for (int i = 0; i < 12; i++)
            {
                float ang = i / 12f * Mathf.PI * 2f;
                float x0 = Size * 0.5f + Mathf.Cos(ang) * Size * 0.18f;
                float y0 = Size * 0.5f + Mathf.Sin(ang) * Size * 0.18f;
                float x1 = Size * 0.5f + Mathf.Cos(ang) * Size * 0.32f;
                float y1 = Size * 0.5f + Mathf.Sin(ang) * Size * 0.32f;
                DrawThickLine(px, x0, y0, x1, y1, 4f, accent);
            }
        }

        private static void DrawShield(Color[] px, Color accent, Color glow)
        {
            DrawRoundedRect(px, Size * 0.32f, Size * 0.28f, Size * 0.36f, Size * 0.48f, 12, accent, glow);
            DrawThickLine(px, Size * 0.5f, Size * 0.35f, Size * 0.5f, Size * 0.68f, 4f, glow);
            DrawThickLine(px, Size * 0.38f, Size * 0.52f, Size * 0.62f, Size * 0.52f, 4f, glow);
        }

        private static void DrawDroplet(Color[] px, Color accent, Color glow, bool skull)
        {
            FillCircle(px, Size * 0.5f, Size * 0.42f, Size * 0.14f, accent);
            DrawTriangle(px, Size * 0.5f, Size * 0.72f,
                Size * 0.38f, Size * 0.48f, Size * 0.62f, Size * 0.48f, accent);
            FillCircle(px, Size * 0.46f, Size * 0.48f, Size * 0.04f, glow);
            if (skull)
            {
                FillCircle(px, Size * 0.5f, Size * 0.40f, Size * 0.07f, Color.Lerp(accent, Color.black, 0.3f));
            }
        }

        private static void DrawStarRing(Color[] px, Color accent, Color glow)
        {
            float cx = Size * 0.5f, cy = Size * 0.5f;
            for (int i = 0; i < 8; i++)
            {
                float ang = i / 8f * Mathf.PI * 2f;
                float x = cx + Mathf.Cos(ang) * Size * 0.22f;
                float y = cy + Mathf.Sin(ang) * Size * 0.22f;
                FillCircle(px, x, y, 6f, Color.Lerp(accent, glow, 0.5f));
            }

            DrawStar4(px, cx, cy, Size * 0.14f, Size * 0.05f, glow);
        }

        private static void DrawCloudBolt(Color[] px, Color accent, Color glow)
        {
            FillCircle(px, Size * 0.40f, Size * 0.55f, Size * 0.12f, Color.Lerp(accent, Color.white, 0.2f));
            FillCircle(px, Size * 0.52f, Size * 0.58f, Size * 0.14f, Color.Lerp(accent, Color.white, 0.25f));
            FillCircle(px, Size * 0.62f, Size * 0.52f, Size * 0.10f, Color.Lerp(accent, Color.white, 0.15f));
            DrawThickLine(px, Size * 0.52f, Size * 0.45f, Size * 0.42f, Size * 0.25f, 5f, glow);
            DrawThickLine(px, Size * 0.42f, Size * 0.25f, Size * 0.55f, Size * 0.22f, 4f, glow);
        }

        private static void DrawGem(Color[] px, Color accent, Color glow)
        {
            DrawTriangle(px, Size * 0.5f, Size * 0.78f, Size * 0.28f, Size * 0.48f, Size * 0.72f, Size * 0.48f, accent);
            DrawTriangle(px, Size * 0.5f, Size * 0.28f, Size * 0.28f, Size * 0.48f, Size * 0.72f, Size * 0.48f, glow);
            DrawThickLine(px, Size * 0.5f, Size * 0.78f, Size * 0.5f, Size * 0.28f, 2f, Color.white);
        }

        private static void DrawPentacle(Color[] px, Color accent, Color glow)
        {
            float cx = Size * 0.5f, cy = Size * 0.5f, r = Size * 0.2f;
            // Circle
            for (int i = 0; i < 64; i++)
            {
                float a0 = i / 64f * Mathf.PI * 2f;
                float a1 = (i + 1) / 64f * Mathf.PI * 2f;
                DrawThickLine(px,
                    cx + Mathf.Cos(a0) * r, cy + Mathf.Sin(a0) * r,
                    cx + Mathf.Cos(a1) * r, cy + Mathf.Sin(a1) * r,
                    3f, glow);
            }

            // 5-point star
            for (int i = 0; i < 5; i++)
            {
                float a0 = -Mathf.PI / 2f + i * 4f * Mathf.PI / 5f;
                float a1 = -Mathf.PI / 2f + (i + 1) * 4f * Mathf.PI / 5f;
                DrawThickLine(px,
                    cx + Mathf.Cos(a0) * r * 0.85f, cy + Mathf.Sin(a0) * r * 0.85f,
                    cx + Mathf.Cos(a1) * r * 0.85f, cy + Mathf.Sin(a1) * r * 0.85f,
                    3.5f, accent);
            }
        }

        private static void DrawStar4(Color[] px, float cx, float cy, float outer, float inner, Color c)
        {
            // 4-point diamond star
            DrawTriangle(px, cx, cy + outer, cx - inner, cy, cx + inner, cy, c);
            DrawTriangle(px, cx, cy - outer, cx - inner, cy, cx + inner, cy, c);
            DrawTriangle(px, cx + outer, cy, cx, cy - inner, cx, cy + inner, c);
            DrawTriangle(px, cx - outer, cy, cx, cy - inner, cx, cy + inner, c);
        }

        // ── Raster helpers ────────────────────────────────────────────

        private static Color[] NewCanvas(out Color deep, out Color mid)
        {
            deep = new Color(0.06f, 0.04f, 0.1f, 1f);
            mid = new Color(0.12f, 0.08f, 0.18f, 1f);
            var px = new Color[Size * Size];
            for (int i = 0; i < px.Length; i++)
                px[i] = deep;
            return px;
        }

        private static void FillRadial(Color[] px, float cx, float cy, float r, Color center, Color edge)
        {
            float r2 = r * r;
            for (int y = 0; y < Size; y++)
            for (int x = 0; x < Size; x++)
            {
                float dx = x + 0.5f - cx;
                float dy = y + 0.5f - cy;
                float d2 = dx * dx + dy * dy;
                if (d2 > r2) continue;
                float t = Mathf.Sqrt(d2) / r;
                var c = Color.Lerp(center, edge, t * t);
                if (edge.a < 0.01f)
                    c.a = center.a * (1f - t);
                Blend(px, x, y, c);
            }
        }

        private static void FillCircle(Color[] px, float cx, float cy, float r, Color c)
        {
            if (r <= 0.01f) return;
            int x0 = Mathf.Max(0, Mathf.FloorToInt(cx - r - 1));
            int x1 = Mathf.Min(Size - 1, Mathf.CeilToInt(cx + r + 1));
            int y0 = Mathf.Max(0, Mathf.FloorToInt(cy - r - 1));
            int y1 = Mathf.Min(Size - 1, Mathf.CeilToInt(cy + r + 1));
            float r2 = r * r;
            for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++)
            {
                float dx = x + 0.5f - cx;
                float dy = y + 0.5f - cy;
                if (dx * dx + dy * dy <= r2)
                    Blend(px, x, y, c);
            }
        }

        private static void DrawThickLine(Color[] px, float x0, float y0, float x1, float y1, float w, Color c)
        {
            float dx = x1 - x0;
            float dy = y1 - y0;
            float len = Mathf.Sqrt(dx * dx + dy * dy);
            if (len < 0.001f) return;
            int steps = Mathf.CeilToInt(len);
            for (int i = 0; i <= steps; i++)
            {
                float t = i / (float)steps;
                FillCircle(px, x0 + dx * t, y0 + dy * t, w * 0.5f, c);
            }
        }

        private static void DrawTriangle(Color[] px, float x0, float y0, float x1, float y1, float x2, float y2, Color c)
        {
            int minX = Mathf.Max(0, Mathf.FloorToInt(Mathf.Min(x0, Mathf.Min(x1, x2))));
            int maxX = Mathf.Min(Size - 1, Mathf.CeilToInt(Mathf.Max(x0, Mathf.Max(x1, x2))));
            int minY = Mathf.Max(0, Mathf.FloorToInt(Mathf.Min(y0, Mathf.Min(y1, y2))));
            int maxY = Mathf.Min(Size - 1, Mathf.CeilToInt(Mathf.Max(y0, Mathf.Max(y1, y2))));
            for (int y = minY; y <= maxY; y++)
            for (int x = minX; x <= maxX; x++)
            {
                if (PointInTri(x + 0.5f, y + 0.5f, x0, y0, x1, y1, x2, y2))
                    Blend(px, x, y, c);
            }
        }

        private static bool PointInTri(float px, float py, float x0, float y0, float x1, float y1, float x2, float y2)
        {
            float d1 = Sign(px, py, x0, y0, x1, y1);
            float d2 = Sign(px, py, x1, y1, x2, y2);
            float d3 = Sign(px, py, x2, y2, x0, y0);
            bool hasNeg = d1 < 0 || d2 < 0 || d3 < 0;
            bool hasPos = d1 > 0 || d2 > 0 || d3 > 0;
            return !(hasNeg && hasPos);
        }

        private static float Sign(float px, float py, float x0, float y0, float x1, float y1) =>
            (px - x1) * (y0 - y1) - (x0 - x1) * (py - y1);

        private static void DrawRoundedRect(Color[] px, float x, float y, float w, float h, float rad, Color fill, Color border)
        {
            for (int j = 0; j < (int)h; j++)
            for (int i = 0; i < (int)w; i++)
            {
                float pxX = x + i;
                float pxY = y + j;
                // simple rect for speed
                bool edge = i < 3 || j < 3 || i > w - 4 || j > h - 4;
                Blend(px, Mathf.RoundToInt(pxX), Mathf.RoundToInt(pxY), edge ? border : fill);
            }
        }

        private static void ApplyVignette(Color[] px, float strength)
        {
            float cx = Size * 0.5f, cy = Size * 0.5f;
            float maxD = Mathf.Sqrt(cx * cx + cy * cy);
            for (int y = 0; y < Size; y++)
            for (int x = 0; x < Size; x++)
            {
                float d = Vector2.Distance(new Vector2(x, y), new Vector2(cx, cy)) / maxD;
                float v = Mathf.SmoothStep(0.55f, 1.05f, d) * strength;
                int i = y * Size + x;
                px[i] = Color.Lerp(px[i], Color.black, v);
                px[i].a = 1f;
            }
        }

        private static void Blend(Color[] px, int x, int y, Color c)
        {
            if ((uint)x >= Size || (uint)y >= Size) return;
            int i = y * Size + x;
            if (c.a >= 0.99f)
            {
                px[i] = c;
                px[i].a = 1f;
                return;
            }

            var dst = px[i];
            float a = c.a;
            px[i] = Color.Lerp(dst, c, a);
            px[i].a = 1f;
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

        private static Sprite ToSprite(Color[] px, string name)
        {
            var tex = new Texture2D(Size, Size, TextureFormat.RGBA32, false);
            tex.filterMode = FilterMode.Bilinear;
            tex.wrapMode = TextureWrapMode.Clamp;
            tex.SetPixels(px);
            tex.Apply(false, true);
            tex.name = name;
            return Sprite.Create(tex, new Rect(0, 0, Size, Size), new Vector2(0.5f, 0.5f), 100f);
        }
    }
}
