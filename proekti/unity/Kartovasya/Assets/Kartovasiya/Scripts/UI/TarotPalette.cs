// Tarot / chaos color system / Цветовая система Таро-хаоса
// Chaos Spellweaver (Картовасия)

using UnityEngine;
using Kartovasiya.Core;

namespace Kartovasiya.UI
{
    /// <summary>
    /// Central palette for tarot-styled cards (2.5D mystical look).
    /// Центральная палитра для карт в эстетике Таро.
    /// </summary>
    public static class TarotPalette
    {
        public static readonly Color Parchment = new Color(0.93f, 0.86f, 0.72f, 1f);
        public static readonly Color Ink = new Color(0.12f, 0.08f, 0.1f, 1f);
        public static readonly Color Gold = new Color(0.85f, 0.7f, 0.28f, 1f);
        public static readonly Color GoldBright = new Color(1f, 0.88f, 0.45f, 1f);
        public static readonly Color VoidPurple = new Color(0.14f, 0.06f, 0.2f, 1f);
        public static readonly Color Night = new Color(0.05f, 0.04f, 0.08f, 1f);
        public static readonly Color BloodRed = new Color(0.55f, 0.1f, 0.15f, 1f);
        public static readonly Color Highlight = new Color(1f, 0.92f, 0.45f, 1f);
        public static readonly Color Risk = new Color(1f, 0.35f, 0.25f, 1f);

        public static Color ForElement(ElementId id)
        {
            return id switch
            {
                ElementId.Fire => Hex("#E85D04"),
                ElementId.Water => Hex("#0077B6"),
                ElementId.Earth => Hex("#6B705C"),
                ElementId.Air => Hex("#ADE8F4"),
                ElementId.Lightning => Hex("#9D4EDD"),
                ElementId.Ice => Hex("#48CAE4"),
                ElementId.Nature => Hex("#2D6A4F"),
                ElementId.Shadow => Hex("#5A189A"),
                ElementId.Metal => Hex("#ADB5BD"),
                ElementId.Light => Hex("#FFD60A"),
                ElementId.Poison => Hex("#52B788"),
                ElementId.Aether => Hex("#7209B7"),
                ElementId.Blood => Hex("#9B2226"),
                ElementId.Storm => Hex("#4CC9F0"),
                ElementId.Crystal => Hex("#F72585"),
                ElementId.Arcane => Hex("#B5179E"),
                _ => Gold
            };
        }

        public static Color ForCardType(CardType type)
        {
            return type switch
            {
                CardType.Spell => new Color(0.28f, 0.1f, 0.38f, 1f),
                CardType.Element => new Color(0.08f, 0.28f, 0.38f, 1f),
                CardType.Effect => new Color(0.18f, 0.32f, 0.16f, 1f),
                _ => new Color(0.28f, 0.24f, 0.16f, 1f)
            };
        }

        public static Color ForRarity(CardRarity rarity)
        {
            return rarity switch
            {
                CardRarity.Common => new Color(0.65f, 0.65f, 0.68f),
                CardRarity.Uncommon => new Color(0.35f, 0.78f, 0.45f),
                CardRarity.Rare => new Color(0.35f, 0.55f, 0.95f),
                CardRarity.Epic => new Color(0.7f, 0.35f, 0.9f),
                CardRarity.Legendary => new Color(0.95f, 0.7f, 0.2f),
                CardRarity.Mythic => new Color(1f, 0.3f, 0.45f),
                _ => Gold
            };
        }

        public static string ElementGlyph(ElementId id)
        {
            // Unicode-ish mystical symbols (fallback readable letters)
            return id switch
            {
                ElementId.Fire => "🜂",
                ElementId.Water => "🜄",
                ElementId.Earth => "🜃",
                ElementId.Air => "🜁",
                ElementId.Lightning => "⚡",
                ElementId.Ice => "❄",
                ElementId.Nature => "❀",
                ElementId.Shadow => "☽",
                ElementId.Metal => "⚔",
                ElementId.Light => "☀",
                ElementId.Poison => "☠",
                ElementId.Aether => "✧",
                ElementId.Blood => "❤",
                ElementId.Storm => "☁",
                ElementId.Crystal => "◆",
                ElementId.Arcane => "✦",
                _ => "•"
            };
        }

        public static string ElementGlyphAscii(ElementId id)
        {
            // Safer for built-in fonts without alchemical glyphs
            return id switch
            {
                ElementId.Fire => "F",
                ElementId.Water => "W",
                ElementId.Earth => "E",
                ElementId.Air => "A",
                ElementId.Lightning => "Z",
                ElementId.Ice => "I",
                ElementId.Nature => "N",
                ElementId.Shadow => "S",
                ElementId.Metal => "M",
                ElementId.Light => "L",
                ElementId.Poison => "P",
                ElementId.Aether => "Æ",
                ElementId.Blood => "B",
                ElementId.Storm => "T",
                ElementId.Crystal => "C",
                ElementId.Arcane => "★",
                _ => "?"
            };
        }

        private static Color Hex(string hex)
        {
            ColorUtility.TryParseHtmlString(hex, out var c);
            return c;
        }
    }
}
