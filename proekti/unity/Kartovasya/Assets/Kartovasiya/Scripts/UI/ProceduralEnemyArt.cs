// Procedural 2D enemy portraits by visual kind
// 2D-портреты врагов (не квадрат)
// Chaos Spellweaver (Картовасия)

using System.Collections.Generic;
using UnityEngine;
using Kartovasiya.Combat;
using Kartovasiya.Core;

namespace Kartovasiya.UI
{
    public static class ProceduralEnemyArt
    {
        private static readonly Dictionary<int, Sprite> Cache = new Dictionary<int, Sprite>();
        private const int Size = 256;

        public static Sprite Get(EnemyVisual visual, ElementId affinity = ElementId.None)
        {
            int key = (int)visual * 100 + (int)affinity;
            if (Cache.TryGetValue(key, out var s))
                return s;
            s = Build(visual, affinity);
            Cache[key] = s;
            return s;
        }

        private static Sprite Build(EnemyVisual v, ElementId aff)
        {
            var px = new Color[Size * Size];
            Color bg = new Color(0.08f, 0.05f, 0.12f, 1f);
            for (int i = 0; i < px.Length; i++) px[i] = bg;

            Color accent = aff != ElementId.None
                ? TarotPalette.ForElement(aff)
                : AccentFor(v);

            // Atmosphere
            FillRadial(px, 0.5f, 0.45f, 0.55f, Color.Lerp(accent, bg, 0.5f), bg);

            // Silhouette family
            switch (v)
            {
                case EnemyVisual.DragonFire:
                case EnemyVisual.DragonIce:
                case EnemyVisual.DragonShadow:
                case EnemyVisual.DragonAncient:
                case EnemyVisual.LordFlame:
                    DrawDragon(px, accent);
                    break;
                case EnemyVisual.Griffin:
                case EnemyVisual.Harpy:
                case EnemyVisual.Phoenix:
                case EnemyVisual.BloodRaven:
                    DrawWinged(px, accent);
                    break;
                case EnemyVisual.Hydra:
                case EnemyVisual.Cerberus:
                case EnemyVisual.GiantWolf:
                case EnemyVisual.HellHound:
                case EnemyVisual.GiantBear:
                    DrawBeast(px, accent);
                    break;
                case EnemyVisual.Minotaur:
                case EnemyVisual.Ogre:
                case EnemyVisual.Troll:
                case EnemyVisual.Orc:
                case EnemyVisual.Gnoll:
                case EnemyVisual.Titan:
                    DrawBrute(px, accent);
                    break;
                case EnemyVisual.Vampire:
                case EnemyVisual.DarkElf:
                case EnemyVisual.DarkMage:
                case EnemyVisual.Necromancer:
                case EnemyVisual.FallenKnight:
                case EnemyVisual.FallenAristocrat:
                case EnemyVisual.MadAlchemist:
                case EnemyVisual.Cultist:
                case EnemyVisual.Bandit:
                case EnemyVisual.ArtifactHunter:
                case EnemyVisual.Lamia:
                    DrawHumanoid(px, accent);
                    break;
                case EnemyVisual.Ghost:
                case EnemyVisual.Banshee:
                case EnemyVisual.Poltergeist:
                case EnemyVisual.AncestorSpirit:
                case EnemyVisual.NatureSpirit:
                case EnemyVisual.VoidEntity:
                case EnemyVisual.TimeParasite:
                case EnemyVisual.ShadowWeaver:
                case EnemyVisual.ChaosRift:
                    DrawSpirit(px, accent);
                    break;
                case EnemyVisual.ElementalFire:
                case EnemyVisual.ElementalWater:
                case EnemyVisual.ElementalEarth:
                case EnemyVisual.ElementalAir:
                    DrawElemental(px, accent);
                    break;
                case EnemyVisual.Skeleton:
                case EnemyVisual.Zombie:
                case EnemyVisual.SkeletonKnight:
                case EnemyVisual.Ghoul:
                case EnemyVisual.Lich:
                case EnemyVisual.DeadKing:
                    DrawUndead(px, accent);
                    break;
                case EnemyVisual.GolemStone:
                case EnemyVisual.GolemIron:
                case EnemyVisual.Automaton:
                case EnemyVisual.RuinGuardian:
                case EnemyVisual.CrystalConstruct:
                case EnemyVisual.SteamGolem:
                case EnemyVisual.WarMachine:
                case EnemyVisual.AncientRuinGuard:
                    DrawConstruct(px, accent);
                    break;
                case EnemyVisual.Goblin:
                case EnemyVisual.Kobold:
                case EnemyVisual.Fairy:
                    DrawSmallFolk(px, accent);
                    break;
                case EnemyVisual.Kraken:
                case EnemyVisual.Leviathan:
                case EnemyVisual.Siren:
                    DrawAquatic(px, accent);
                    break;
                case EnemyVisual.Beholder:
                    DrawEye(px, accent);
                    break;
                case EnemyVisual.GiantSpider:
                    DrawSpider(px, accent);
                    break;
                case EnemyVisual.Werewolf:
                case EnemyVisual.Chimera:
                case EnemyVisual.Manticore:
                case EnemyVisual.Basilisk:
                case EnemyVisual.Dragonkin:
                case EnemyVisual.MutantBeast:
                    DrawChimera(px, accent);
                    break;
                case EnemyVisual.QueenIce:
                    DrawDragon(px, TarotPalette.ForElement(ElementId.Ice));
                    break;
                case EnemyVisual.Gargoyle:
                    DrawWinged(px, Color.Lerp(accent, Color.gray, 0.4f));
                    break;
                case EnemyVisual.DemonLesser:
                case EnemyVisual.DemonLord:
                case EnemyVisual.FallenAngel:
                case EnemyVisual.ExileGod:
                    DrawDemon(px, accent);
                    break;
                case EnemyVisual.MirrorTwin:
                    DrawHumanoid(px, Color.Lerp(accent, Color.white, 0.5f));
                    break;
                default:
                    DrawBrute(px, accent);
                    break;
            }

            // Gold frame vignette
            ApplyFrame(px);
            return ToSprite(px, v.ToString());
        }

        private static Color AccentFor(EnemyVisual v) => v switch
        {
            EnemyVisual.DragonFire or EnemyVisual.LordFlame or EnemyVisual.ElementalFire or EnemyVisual.HellHound or EnemyVisual.Phoenix
                => TarotPalette.ForElement(ElementId.Fire),
            EnemyVisual.DragonIce or EnemyVisual.QueenIce => TarotPalette.ForElement(ElementId.Ice),
            EnemyVisual.DragonShadow or EnemyVisual.ShadowWeaver or EnemyVisual.Vampire or EnemyVisual.Banshee
                => TarotPalette.ForElement(ElementId.Shadow),
            EnemyVisual.ElementalWater or EnemyVisual.Kraken or EnemyVisual.Leviathan or EnemyVisual.Siren
                => TarotPalette.ForElement(ElementId.Water),
            EnemyVisual.ElementalEarth or EnemyVisual.GolemStone or EnemyVisual.Minotaur
                => TarotPalette.ForElement(ElementId.Earth),
            EnemyVisual.ElementalAir or EnemyVisual.Griffin or EnemyVisual.Harpy
                => TarotPalette.ForElement(ElementId.Air),
            EnemyVisual.Lich or EnemyVisual.Beholder or EnemyVisual.ChaosRift
                => TarotPalette.ForElement(ElementId.Arcane),
            EnemyVisual.FallenAristocrat => new Color(0.15f, 0.12f, 0.18f),
            EnemyVisual.DeadKing => TarotPalette.ForElement(ElementId.Blood),
            _ => TarotPalette.Gold
        };

        // ── Silhouettes ───────────────────────────────────────────────

        private static void DrawDragon(Color[] px, Color c)
        {
            // Body
            FillEllipse(px, 0.50f, 0.40f, 0.22f, 0.18f, c);
            // Head
            FillEllipse(px, 0.68f, 0.55f, 0.12f, 0.10f, Color.Lerp(c, Color.white, 0.15f));
            // Wings
            FillTriangle(px, 0.35f, 0.55f, 0.15f, 0.70f, 0.40f, 0.35f, Color.Lerp(c, Color.black, 0.25f));
            FillTriangle(px, 0.45f, 0.50f, 0.25f, 0.75f, 0.48f, 0.38f, Color.Lerp(c, Color.black, 0.15f));
            // Tail
            FillEllipse(px, 0.28f, 0.28f, 0.14f, 0.06f, c);
            // Eye
            FillCircle(px, 0.72f, 0.58f, 0.025f, TarotPalette.GoldBright);
            // Horns
            FillTriangle(px, 0.64f, 0.64f, 0.60f, 0.78f, 0.68f, 0.64f, Color.Lerp(c, Color.white, 0.3f));
            FillTriangle(px, 0.74f, 0.64f, 0.78f, 0.78f, 0.70f, 0.64f, Color.Lerp(c, Color.white, 0.3f));
        }

        private static void DrawWinged(Color[] px, Color c)
        {
            FillEllipse(px, 0.5f, 0.42f, 0.12f, 0.16f, c);
            FillCircle(px, 0.5f, 0.62f, 0.08f, Color.Lerp(c, Color.white, 0.2f));
            FillTriangle(px, 0.5f, 0.55f, 0.15f, 0.70f, 0.35f, 0.40f, Color.Lerp(c, Color.black, 0.2f));
            FillTriangle(px, 0.5f, 0.55f, 0.85f, 0.70f, 0.65f, 0.40f, Color.Lerp(c, Color.black, 0.2f));
            FillCircle(px, 0.47f, 0.64f, 0.02f, Color.white);
            FillCircle(px, 0.53f, 0.64f, 0.02f, Color.white);
        }

        private static void DrawBeast(Color[] px, Color c)
        {
            FillEllipse(px, 0.5f, 0.38f, 0.22f, 0.14f, c);
            FillEllipse(px, 0.68f, 0.48f, 0.12f, 0.11f, Color.Lerp(c, Color.white, 0.1f));
            // legs
            FillRect(px, 0.38f, 0.18f, 0.06f, 0.18f, Color.Lerp(c, Color.black, 0.2f));
            FillRect(px, 0.50f, 0.18f, 0.06f, 0.18f, Color.Lerp(c, Color.black, 0.2f));
            FillRect(px, 0.58f, 0.18f, 0.06f, 0.18f, Color.Lerp(c, Color.black, 0.2f));
            FillCircle(px, 0.72f, 0.50f, 0.02f, TarotPalette.Risk);
            // ears/mane
            FillTriangle(px, 0.62f, 0.58f, 0.58f, 0.70f, 0.66f, 0.58f, c);
        }

        private static void DrawBrute(Color[] px, Color c)
        {
            // torso
            FillRect(px, 0.35f, 0.28f, 0.30f, 0.32f, c);
            // head
            FillEllipse(px, 0.5f, 0.68f, 0.12f, 0.12f, Color.Lerp(c, Color.white, 0.15f));
            // arms
            FillRect(px, 0.22f, 0.40f, 0.10f, 0.22f, Color.Lerp(c, Color.black, 0.15f));
            FillRect(px, 0.68f, 0.40f, 0.10f, 0.22f, Color.Lerp(c, Color.black, 0.15f));
            // horns optional
            FillTriangle(px, 0.42f, 0.76f, 0.38f, 0.90f, 0.48f, 0.78f, TarotPalette.Gold);
            FillTriangle(px, 0.58f, 0.76f, 0.62f, 0.90f, 0.52f, 0.78f, TarotPalette.Gold);
            FillCircle(px, 0.46f, 0.70f, 0.02f, Color.white);
            FillCircle(px, 0.54f, 0.70f, 0.02f, Color.white);
        }

        private static void DrawHumanoid(Color[] px, Color c)
        {
            FillRect(px, 0.38f, 0.30f, 0.24f, 0.28f, c);
            FillEllipse(px, 0.5f, 0.66f, 0.10f, 0.12f, new Color(0.85f, 0.75f, 0.7f));
            FillRect(px, 0.30f, 0.42f, 0.08f, 0.18f, Color.Lerp(c, Color.black, 0.2f));
            FillRect(px, 0.62f, 0.42f, 0.08f, 0.18f, Color.Lerp(c, Color.black, 0.2f));
            // cloak flare
            FillTriangle(px, 0.5f, 0.55f, 0.22f, 0.22f, 0.78f, 0.22f, Color.Lerp(c, Color.black, 0.35f));
            FillCircle(px, 0.46f, 0.68f, 0.018f, Color.Lerp(c, Color.white, 0.5f));
            FillCircle(px, 0.54f, 0.68f, 0.018f, Color.Lerp(c, Color.white, 0.5f));
            // hat / hair
            FillEllipse(px, 0.5f, 0.76f, 0.11f, 0.05f, Color.black);
        }

        private static void DrawSpirit(Color[] px, Color c)
        {
            // wispy body
            for (int i = 0; i < 5; i++)
            {
                float y = 0.25f + i * 0.1f;
                float w = 0.18f - i * 0.02f;
                var col = c;
                col.a = 0.55f + i * 0.08f;
                FillEllipse(px, 0.5f, y, w, 0.08f, col);
            }
            FillEllipse(px, 0.5f, 0.68f, 0.11f, 0.12f, Color.Lerp(c, Color.white, 0.35f));
            FillCircle(px, 0.46f, 0.70f, 0.02f, Color.white);
            FillCircle(px, 0.54f, 0.70f, 0.02f, Color.white);
        }

        private static void DrawElemental(Color[] px, Color c)
        {
            FillCircle(px, 0.5f, 0.48f, 0.22f, c);
            FillCircle(px, 0.5f, 0.48f, 0.12f, Color.Lerp(c, Color.white, 0.4f));
            for (int i = 0; i < 8; i++)
            {
                float a = i / 8f * Mathf.PI * 2f;
                FillCircle(px, 0.5f + Mathf.Cos(a) * 0.28f, 0.48f + Mathf.Sin(a) * 0.28f, 0.04f, c);
            }
        }

        private static void DrawUndead(Color[] px, Color c)
        {
            FillRect(px, 0.38f, 0.28f, 0.24f, 0.28f, Color.Lerp(c, Color.gray, 0.4f));
            FillEllipse(px, 0.5f, 0.64f, 0.10f, 0.11f, new Color(0.9f, 0.9f, 0.85f));
            // hollow eyes
            FillCircle(px, 0.46f, 0.66f, 0.025f, Color.black);
            FillCircle(px, 0.54f, 0.66f, 0.025f, Color.black);
            FillCircle(px, 0.46f, 0.66f, 0.01f, TarotPalette.Risk);
            FillCircle(px, 0.54f, 0.66f, 0.01f, TarotPalette.Risk);
            // ribs
            for (int i = 0; i < 4; i++)
                FillRect(px, 0.40f, 0.40f + i * 0.04f, 0.20f, 0.012f, Color.white);
        }

        private static void DrawConstruct(Color[] px, Color c)
        {
            FillRect(px, 0.32f, 0.28f, 0.36f, 0.36f, c);
            FillRect(px, 0.38f, 0.62f, 0.24f, 0.14f, Color.Lerp(c, Color.gray, 0.3f));
            // glowing core
            FillCircle(px, 0.5f, 0.48f, 0.06f, TarotPalette.GoldBright);
            // bolts
            FillRect(px, 0.34f, 0.44f, 0.06f, 0.06f, Color.gray);
            FillRect(px, 0.60f, 0.44f, 0.06f, 0.06f, Color.gray);
        }

        private static void DrawSmallFolk(Color[] px, Color c)
        {
            // three little silhouettes for swarm vibe
            for (int i = 0; i < 3; i++)
            {
                float x = 0.28f + i * 0.22f;
                FillEllipse(px, x, 0.42f, 0.08f, 0.12f, c);
                FillCircle(px, x, 0.58f, 0.055f, Color.Lerp(c, Color.white, 0.2f));
                FillCircle(px, x - 0.02f, 0.59f, 0.012f, Color.white);
            }
        }

        private static void DrawAquatic(Color[] px, Color c)
        {
            FillEllipse(px, 0.5f, 0.45f, 0.28f, 0.14f, c);
            FillTriangle(px, 0.22f, 0.45f, 0.08f, 0.55f, 0.08f, 0.35f, Color.Lerp(c, Color.black, 0.2f));
            FillCircle(px, 0.68f, 0.48f, 0.03f, Color.white);
            // tentacles
            for (int i = 0; i < 4; i++)
                FillEllipse(px, 0.35f + i * 0.1f, 0.28f, 0.04f, 0.1f, Color.Lerp(c, Color.black, 0.15f));
        }

        private static void DrawEye(Color[] px, Color c)
        {
            FillCircle(px, 0.5f, 0.5f, 0.28f, c);
            FillCircle(px, 0.5f, 0.5f, 0.16f, Color.white);
            FillCircle(px, 0.5f, 0.5f, 0.08f, Color.black);
            FillCircle(px, 0.52f, 0.52f, 0.03f, Color.white);
            // eye stalks
            for (int i = 0; i < 5; i++)
            {
                float a = -0.6f + i * 0.3f;
                float x = 0.5f + Mathf.Sin(a) * 0.32f;
                float y = 0.5f + Mathf.Cos(a) * 0.32f;
                FillCircle(px, x, y, 0.04f, Color.Lerp(c, Color.red, 0.3f));
            }
        }

        private static void DrawSpider(Color[] px, Color c)
        {
            FillEllipse(px, 0.5f, 0.45f, 0.16f, 0.12f, c);
            FillCircle(px, 0.5f, 0.58f, 0.08f, Color.Lerp(c, Color.black, 0.2f));
            for (int i = 0; i < 4; i++)
            {
                float y = 0.40f + i * 0.04f;
                FillRect(px, 0.18f, y, 0.18f, 0.015f, Color.Lerp(c, Color.black, 0.3f));
                FillRect(px, 0.64f, y, 0.18f, 0.015f, Color.Lerp(c, Color.black, 0.3f));
            }
        }

        private static void DrawChimera(Color[] px, Color c)
        {
            DrawBeast(px, c);
            FillTriangle(px, 0.55f, 0.55f, 0.70f, 0.75f, 0.75f, 0.50f, Color.Lerp(c, Color.red, 0.3f));
        }

        private static void DrawDemon(Color[] px, Color c)
        {
            DrawHumanoid(px, c);
            FillTriangle(px, 0.40f, 0.78f, 0.34f, 0.95f, 0.46f, 0.80f, Color.Lerp(c, Color.black, 0.2f));
            FillTriangle(px, 0.60f, 0.78f, 0.66f, 0.95f, 0.54f, 0.80f, Color.Lerp(c, Color.black, 0.2f));
            FillTriangle(px, 0.5f, 0.50f, 0.30f, 0.35f, 0.70f, 0.35f, Color.Lerp(c, Color.black, 0.4f));
        }

        // ── Raster ────────────────────────────────────────────────────

        private static void FillRadial(Color[] px, float cx, float cy, float r, Color center, Color edge)
        {
            float R = r * Size;
            float CX = cx * Size, CY = cy * Size;
            for (int y = 0; y < Size; y++)
            for (int x = 0; x < Size; x++)
            {
                float d = Vector2.Distance(new Vector2(x, y), new Vector2(CX, CY)) / R;
                if (d > 1f) continue;
                px[y * Size + x] = Color.Lerp(center, edge, d * d);
            }
        }

        private static void FillCircle(Color[] px, float nx, float ny, float nr, Color c)
        {
            float cx = nx * Size, cy = ny * Size, r = nr * Size;
            int x0 = Mathf.Max(0, (int)(cx - r - 1));
            int x1 = Mathf.Min(Size - 1, (int)(cx + r + 1));
            int y0 = Mathf.Max(0, (int)(cy - r - 1));
            int y1 = Mathf.Min(Size - 1, (int)(cy + r + 1));
            float r2 = r * r;
            for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++)
            {
                float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
                if (dx * dx + dy * dy <= r2)
                    Blend(px, x, y, c);
            }
        }

        private static void FillEllipse(Color[] px, float nx, float ny, float nrx, float nry, Color c)
        {
            float cx = nx * Size, cy = ny * Size, rx = nrx * Size, ry = nry * Size;
            int x0 = Mathf.Max(0, (int)(cx - rx - 1));
            int x1 = Mathf.Min(Size - 1, (int)(cx + rx + 1));
            int y0 = Mathf.Max(0, (int)(cy - ry - 1));
            int y1 = Mathf.Min(Size - 1, (int)(cy + ry + 1));
            for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++)
            {
                float dx = (x + 0.5f - cx) / rx;
                float dy = (y + 0.5f - cy) / ry;
                if (dx * dx + dy * dy <= 1f)
                    Blend(px, x, y, c);
            }
        }

        private static void FillRect(Color[] px, float nx, float ny, float nw, float nh, Color c)
        {
            int x0 = Mathf.Clamp(Mathf.FloorToInt(nx * Size), 0, Size - 1);
            int y0 = Mathf.Clamp(Mathf.FloorToInt(ny * Size), 0, Size - 1);
            int x1 = Mathf.Clamp(Mathf.CeilToInt((nx + nw) * Size), 0, Size - 1);
            int y1 = Mathf.Clamp(Mathf.CeilToInt((ny + nh) * Size), 0, Size - 1);
            for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++)
                Blend(px, x, y, c);
        }

        private static void FillTriangle(Color[] px, float x0, float y0, float x1, float y1, float x2, float y2, Color c)
        {
            float ax = x0 * Size, ay = y0 * Size, bx = x1 * Size, by = y1 * Size, cx = x2 * Size, cy = y2 * Size;
            int minX = Mathf.Max(0, (int)Mathf.Min(ax, Mathf.Min(bx, cx)));
            int maxX = Mathf.Min(Size - 1, (int)Mathf.Max(ax, Mathf.Max(bx, cx)));
            int minY = Mathf.Max(0, (int)Mathf.Min(ay, Mathf.Min(by, cy)));
            int maxY = Mathf.Min(Size - 1, (int)Mathf.Max(ay, Mathf.Max(by, cy)));
            for (int y = minY; y <= maxY; y++)
            for (int x = minX; x <= maxX; x++)
            {
                if (InTri(x + 0.5f, y + 0.5f, ax, ay, bx, by, cx, cy))
                    Blend(px, x, y, c);
            }
        }

        private static bool InTri(float px, float py, float x0, float y0, float x1, float y1, float x2, float y2)
        {
            float d1 = (px - x1) * (y0 - y1) - (x0 - x1) * (py - y1);
            float d2 = (px - x2) * (y1 - y2) - (x1 - x2) * (py - y2);
            float d3 = (px - x0) * (y2 - y0) - (x2 - x0) * (py - y0);
            bool hasNeg = d1 < 0 || d2 < 0 || d3 < 0;
            bool hasPos = d1 > 0 || d2 > 0 || d3 > 0;
            return !(hasNeg && hasPos);
        }

        private static void ApplyFrame(Color[] px)
        {
            for (int y = 0; y < Size; y++)
            for (int x = 0; x < Size; x++)
            {
                float nx = x / (float)(Size - 1);
                float ny = y / (float)(Size - 1);
                float edge = Mathf.Min(Mathf.Min(nx, 1 - nx), Mathf.Min(ny, 1 - ny));
                if (edge < 0.04f)
                    px[y * Size + x] = Color.Lerp(px[y * Size + x], TarotPalette.Gold, 1f - edge / 0.04f);
                float vig = Vector2.Distance(new Vector2(nx, ny), new Vector2(0.5f, 0.5f));
                px[y * Size + x] = Color.Lerp(px[y * Size + x], Color.black, Mathf.SmoothStep(0.7f, 1.1f, vig) * 0.45f);
                px[y * Size + x].a = 1f;
            }
        }

        private static void Blend(Color[] px, int x, int y, Color c)
        {
            if ((uint)x >= Size || (uint)y >= Size) return;
            int i = y * Size + x;
            if (c.a >= 0.99f) { px[i] = c; px[i].a = 1f; return; }
            px[i] = Color.Lerp(px[i], c, c.a);
            px[i].a = 1f;
        }

        private static Sprite ToSprite(Color[] px, string name)
        {
            var tex = new Texture2D(Size, Size, TextureFormat.RGBA32, false);
            tex.filterMode = FilterMode.Bilinear;
            tex.SetPixels(px);
            tex.Apply(false, true);
            tex.name = "enemy_" + name;
            return Sprite.Create(tex, new Rect(0, 0, Size, Size), new Vector2(0.5f, 0.5f), 100f);
        }
    }
}
