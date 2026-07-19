// Default data for all 16 elements / Дефолтные данные 16 стихий
// Chaos Spellweaver (Картовасия)

using UnityEngine;
using Kartovasiya.Core;

namespace Kartovasiya.Elements
{
    /// <summary>
    /// Immutable seed data for generating ElementSO assets in the Editor.
    /// Неизменяемые seed-данные для генерации ElementSO.
    /// Basic: Fire…Shadow | Extended: Metal…Arcane.
    /// </summary>
    public static class ElementDefaults
    {
        public readonly struct Definition
        {
            public readonly ElementId Id;
            public readonly ElementTier Tier;
            public readonly string NameEn;
            public readonly string NameRu;
            public readonly Color Primary;
            public readonly Color Accent;
            public readonly ElementId Opposed;
            public readonly int SortOrder;
            public readonly string DescEn;
            public readonly string DescRu;

            public Definition(
                ElementId id,
                ElementTier tier,
                string nameEn,
                string nameRu,
                Color primary,
                Color accent,
                ElementId opposed,
                int sortOrder,
                string descEn,
                string descRu)
            {
                Id = id;
                Tier = tier;
                NameEn = nameEn;
                NameRu = nameRu;
                Primary = primary;
                Accent = accent;
                Opposed = opposed;
                SortOrder = sortOrder;
                DescEn = descEn;
                DescRu = descRu;
            }
        }

        public static readonly Definition[] All =
        {
            // ── Basic / Базовые ───────────────────────────────────────
            Def(ElementId.Fire, ElementTier.Basic, "Fire", "Огонь",
                "#E85D04", "#FFBA08", ElementId.Water, 0,
                "Raw heat and destruction. Burns over time.",
                "Жар и разрушение. Поджог и урон со временем."),

            Def(ElementId.Water, ElementTier.Basic, "Water", "Вода",
                "#0077B6", "#90E0EF", ElementId.Fire, 1,
                "Flow, pressure and cleansing tides.",
                "Поток, давление и очищающие приливы."),

            Def(ElementId.Earth, ElementTier.Basic, "Earth", "Земля",
                "#6B705C", "#A5A58D", ElementId.Air, 2,
                "Stone, armor and crushing weight.",
                "Камень, броня и сокрушительный вес."),

            Def(ElementId.Air, ElementTier.Basic, "Air", "Воздух",
                "#ADE8F4", "#CAF0F8", ElementId.Earth, 3,
                "Speed, push and precision strikes.",
                "Скорость, отталкивание и точные удары."),

            Def(ElementId.Lightning, ElementTier.Basic, "Lightning", "Молния",
                "#7B2CBF", "#E0AAFF", ElementId.Earth, 4,
                "Chain arcs and burst damage.",
                "Цепные разряды и взрывной урон."),

            Def(ElementId.Ice, ElementTier.Basic, "Ice", "Лёд",
                "#48CAE4", "#CAF0F8", ElementId.Fire, 5,
                "Freeze, slow and brittle shatter.",
                "Заморозка, замедление и хрупкий раскол."),

            Def(ElementId.Nature, ElementTier.Basic, "Nature", "Природа",
                "#2D6A4F", "#95D5B2", ElementId.Shadow, 6,
                "Growth, thorns and regeneration.",
                "Рост, шипы и регенерация."),

            Def(ElementId.Shadow, ElementTier.Basic, "Shadow", "Тень",
                "#240046", "#5A189A", ElementId.Light, 7,
                "Stealth damage, fear and drain.",
                "Скрытый урон, страх и вампиризм."),

            // ── Extended (rare) / Расширенные ─────────────────────────
            Def(ElementId.Metal, ElementTier.Extended, "Metal", "Металл",
                "#6C757D", "#DEE2E6", ElementId.Lightning, 8,
                "Blades, pierce and magnetic pull.",
                "Клинки, пробитие и магнитное притяжение."),

            Def(ElementId.Light, ElementTier.Extended, "Light", "Свет",
                "#FFD60A", "#FFF3B0", ElementId.Shadow, 9,
                "Holy burst, cleanse and reveal.",
                "Святой импульс, очищение и раскрытие."),

            Def(ElementId.Poison, ElementTier.Extended, "Poison", "Яд",
                "#52B788", "#B7E4C7", ElementId.Nature, 10,
                "Acid, toxins and corrosion (Poison/Acid).",
                "Кислота, токсины и коррозия (Яд/Кислота)."),

            Def(ElementId.Aether, ElementTier.Extended, "Aether", "Эфир",
                "#3A0CA3", "#F72585", ElementId.Arcane, 11,
                "Void rifts and reality shear (Aether/Void).",
                "Разломы пустоты и сдвиг реальности (Эфир/Пустота)."),

            Def(ElementId.Blood, ElementTier.Extended, "Blood", "Кровь",
                "#9B2226", "#AE2012", ElementId.Light, 12,
                "Life trade, soul pacts (Blood/Soul).",
                "Обмен жизнями, душевные пакты (Кровь/Душа)."),

            Def(ElementId.Storm, ElementTier.Extended, "Storm", "Буря",
                "#4CC9F0", "#4361EE", ElementId.Earth, 13,
                "Tempest: wind + lightning fusion theme.",
                "Буря: слияние ветра и молнии."),

            Def(ElementId.Crystal, ElementTier.Extended, "Crystal", "Кристалл",
                "#B5179E", "#F72585", ElementId.Metal, 14,
                "Refract, amplify and prismatic shards.",
                "Преломление, усиление и призматические осколки."),

            Def(ElementId.Arcane, ElementTier.Extended, "Arcane", "Аркана",
                "#7209B7", "#F72585", ElementId.Aether, 15,
                "Pure magic: unstable high-roll spells.",
                "Чистая магия: нестабильные сильные заклинания.")
        };

        private static Definition Def(
            ElementId id, ElementTier tier, string en, string ru,
            string primaryHex, string accentHex, ElementId opposed, int order,
            string descEn, string descRu)
        {
            return new Definition(
                id, tier, en, ru,
                Hex(primaryHex), Hex(accentHex),
                opposed, order, descEn, descRu);
        }

        private static Color Hex(string hex)
        {
            if (ColorUtility.TryParseHtmlString(hex, out var c))
                return c;
            return Color.magenta;
        }
    }
}
