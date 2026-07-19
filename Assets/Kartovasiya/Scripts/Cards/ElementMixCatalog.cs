// Element + Element mix names (Fire+Shadow = Dark Fire, etc.)
// Смеси стихий друг с другом
// Chaos Spellweaver (Картовасия)

using System.Collections.Generic;
using UnityEngine;
using Kartovasiya.Core;

namespace Kartovasiya.Cards
{
    public readonly struct ElementMixInfo
    {
        public readonly ElementId A;
        public readonly ElementId B;
        public readonly string NameEn;
        public readonly string NameRu;
        public readonly string DescEn;
        public readonly string DescRu;
        public readonly Color Tint;

        public ElementMixInfo(
            ElementId a, ElementId b,
            string en, string ru, string descEn, string descRu, string hex)
        {
            A = a;
            B = b;
            NameEn = en;
            NameRu = ru;
            DescEn = descEn;
            DescRu = descRu;
            ColorUtility.TryParseHtmlString(hex, out var c);
            Tint = c.a > 0.01f ? c : Color.white;
        }
    }

    /// <summary>
    /// Named mixes for basic elements that cannot be cast alone.
    /// Перетащи стихию на стихию → гибридная карта-элемент.
    /// </summary>
    public static class ElementMixCatalog
    {
        private static Dictionary<string, ElementMixInfo> _map;

        /// <summary>Elements that participate in free pairwise mixing.</summary>
        public static readonly ElementId[] MixableBasics =
        {
            ElementId.Fire,
            ElementId.Water,
            ElementId.Earth,
            ElementId.Air,
            ElementId.Lightning,
            ElementId.Ice,
            ElementId.Nature,
            ElementId.Shadow,
            ElementId.Light
        };

        public static bool TryGet(ElementId a, ElementId b, out ElementMixInfo info)
        {
            Ensure();
            info = default;
            if (a == ElementId.None || b == ElementId.None || a == b)
                return false;

            string key = Key(a, b);
            return _map.TryGetValue(key, out info);
        }

        public static string Key(ElementId a, ElementId b)
        {
            int x = (int)a;
            int y = (int)b;
            if (x > y)
            {
                int t = x;
                x = y;
                y = t;
            }

            return x + "+" + y;
        }

        /// <summary>
        /// Fallback name when no catalog entry (still allow mix).
        /// </summary>
        public static void GetGenericName(ElementId a, ElementId b, out string en, out string ru)
        {
            en = $"{a} + {b}";
            ru = $"{Ru(a)} + {Ru(b)}";
        }

        public static string Ru(ElementId e) => e switch
        {
            ElementId.Fire => "Огонь",
            ElementId.Water => "Вода",
            ElementId.Earth => "Земля",
            ElementId.Air => "Воздух",
            ElementId.Lightning => "Молния",
            ElementId.Ice => "Лёд",
            ElementId.Nature => "Природа",
            ElementId.Shadow => "Тьма",
            ElementId.Light => "Свет",
            ElementId.Metal => "Металл",
            ElementId.Poison => "Яд",
            ElementId.Aether => "Эфир",
            ElementId.Blood => "Кровь",
            ElementId.Storm => "Буря",
            ElementId.Crystal => "Кристалл",
            ElementId.Arcane => "Аркана",
            _ => e.ToString()
        };

        private static void Ensure()
        {
            if (_map != null) return;
            _map = new Dictionary<string, ElementMixInfo>(64);
            void Add(ElementMixInfo m)
            {
                string k = Key(m.A, m.B);
                if (!_map.ContainsKey(k))
                    _map.Add(k, m);
            }

            // ── Fire ────────────────────────────────────────────────
            Add(new ElementMixInfo(ElementId.Fire, ElementId.Shadow,
                "Dark Fire", "Тёмный огонь",
                "Shadow-fed flames.", "Пламя, вскормленное тьмой.", "#5A189A"));
            Add(new ElementMixInfo(ElementId.Fire, ElementId.Water,
                "Steam Essence", "Суть пара",
                "Scalding mist element.", "Обжигающий пар-стихия.", "#8ECAE6"));
            Add(new ElementMixInfo(ElementId.Fire, ElementId.Ice,
                "Thermal Shock", "Термошок",
                "Heat vs cold clash.", "Столкновение жара и холода.", "#FF6B35"));
            Add(new ElementMixInfo(ElementId.Fire, ElementId.Air,
                "Wildfire Wind", "Огненный ветер",
                "Spreading burn wind.", "Разносящий ожог ветер.", "#E85D04"));
            Add(new ElementMixInfo(ElementId.Fire, ElementId.Lightning,
                "Plasma Spark", "Плазменная искра",
                "Superheated arc.", "Сверхгорячая дуга.", "#F72585"));
            Add(new ElementMixInfo(ElementId.Fire, ElementId.Light,
                "Solar Flame", "Солнечное пламя",
                "Radiant fire.", "Сияющий огонь.", "#FFD60A"));
            Add(new ElementMixInfo(ElementId.Fire, ElementId.Earth,
                "Burning Stone", "Горящий камень",
                "Molten rock essence.", "Суть раскалённого камня.", "#DC2F02"));
            Add(new ElementMixInfo(ElementId.Fire, ElementId.Nature,
                "Wildfire Seed", "Семя пала",
                "Burning growth.", "Горящий рост.", "#E85D04"));

            // ── Lightning ───────────────────────────────────────────
            Add(new ElementMixInfo(ElementId.Lightning, ElementId.Shadow,
                "Dark Lightning", "Тёмная молния",
                "Void-charged bolt.", "Разряд пустоты.", "#3C096C"));
            Add(new ElementMixInfo(ElementId.Lightning, ElementId.Water,
                "Storm Surge", "Штормовой напор",
                "Shock through water.", "Удар через воду.", "#7209B7"));
            Add(new ElementMixInfo(ElementId.Lightning, ElementId.Ice,
                "Charged Frost", "Заряженный иней",
                "Static ice.", "Ледяной ток.", "#48CAE4"));
            Add(new ElementMixInfo(ElementId.Lightning, ElementId.Air,
                "Chain Spark", "Цепная искра",
                "Arcing winds.", "Дуга в ветре.", "#9D4EDD"));
            Add(new ElementMixInfo(ElementId.Lightning, ElementId.Light,
                "Holy Bolt", "Святой разряд",
                "Radiant shock.", "Сияющий шок.", "#B5179E"));
            Add(new ElementMixInfo(ElementId.Lightning, ElementId.Earth,
                "Quake Spark", "Искровой толчок",
                "Shock through stone.", "Ток сквозь камень.", "#6C584C"));
            Add(new ElementMixInfo(ElementId.Lightning, ElementId.Nature,
                "Living Current", "Живой ток",
                "Bioelectric spark.", "Биотоковая искра.", "#2D6A4F"));

            // ── Shadow ───────────────────────────────────────────────
            Add(new ElementMixInfo(ElementId.Shadow, ElementId.Water,
                "Black Tide", "Чёрный прилив",
                "Dark waters.", "Тёмные воды.", "#240046"));
            Add(new ElementMixInfo(ElementId.Shadow, ElementId.Ice,
                "Void Frost", "Мороз пустоты",
                "Absolute cold dark.", "Абсолютный тёмный холод.", "#3A0CA3"));
            Add(new ElementMixInfo(ElementId.Shadow, ElementId.Air,
                "Night Wind", "Ночной ветер",
                "Whispering dark gale.", "Тёмный шёпот ветра.", "#5A189A"));
            Add(new ElementMixInfo(ElementId.Shadow, ElementId.Light,
                "Eclipse Shard", "Осколок затмения",
                "Twilight clash.", "Стык сумерек.", "#3C096C"));
            Add(new ElementMixInfo(ElementId.Shadow, ElementId.Earth,
                "Grave Soil", "Могильная земля",
                "Cursed ground.", "Проклятая почва.", "#2B2D42"));
            Add(new ElementMixInfo(ElementId.Shadow, ElementId.Nature,
                "Blight Root", "Корень порчи",
                "Corrupted growth.", "Порченая поросль.", "#1B4332"));

            // ── Light ────────────────────────────────────────────────
            Add(new ElementMixInfo(ElementId.Light, ElementId.Water,
                "Holy Mist", "Святая дымка",
                "Radiant vapor.", "Сияющий пар.", "#CAF0F8"));
            Add(new ElementMixInfo(ElementId.Light, ElementId.Ice,
                "Crystal Dawn", "Кристальный рассвет",
                "Glittering cold light.", "Сверкающий холодный свет.", "#90E0EF"));
            Add(new ElementMixInfo(ElementId.Light, ElementId.Air,
                "Zephyr Glow", "Сияние зефира",
                "Bright wind.", "Светлый ветер.", "#ADE8F4"));
            Add(new ElementMixInfo(ElementId.Light, ElementId.Earth,
                "Sunstone", "Солнечный камень",
                "Earthen radiance.", "Земное сияние.", "#FFD60A"));
            Add(new ElementMixInfo(ElementId.Light, ElementId.Nature,
                "Bloom Light", "Свет цветения",
                "Life-giving glow.", "Животворящее сияние.", "#95D5B2"));

            // ── Ice ──────────────────────────────────────────────────
            Add(new ElementMixInfo(ElementId.Ice, ElementId.Water,
                "Frost Water", "Ледяная вода",
                "Deep chill liquid.", "Ледяная глубина.", "#48CAE4"));
            Add(new ElementMixInfo(ElementId.Ice, ElementId.Air,
                "Blizzard Breath", "Дыхание метели",
                "Freezing wind.", "Ледяной ветер.", "#90E0EF"));
            Add(new ElementMixInfo(ElementId.Ice, ElementId.Earth,
                "Permafrost", "Вечная мерзлота",
                "Frozen stone.", "Мёрзлый камень.", "#CAF0F8"));
            Add(new ElementMixInfo(ElementId.Ice, ElementId.Nature,
                "Winter Bloom", "Зимний цвет",
                "Frosted growth.", "Инеевый рост.", "#74C69D"));

            // ── Water ────────────────────────────────────────────────
            Add(new ElementMixInfo(ElementId.Water, ElementId.Air,
                "Storm Mist", "Штормовая мгла",
                "Wet wind.", "Сырой ветер.", "#0077B6"));
            Add(new ElementMixInfo(ElementId.Water, ElementId.Earth,
                "Mud Essence", "Суть грязи",
                "Heavy wet earth.", "Тяжёлая сырая земля.", "#6B705C"));
            Add(new ElementMixInfo(ElementId.Water, ElementId.Nature,
                "Mire Sap", "Сок трясины",
                "Wet living muck.", "Живая топь.", "#2D6A4F"));

            // ── Air ──────────────────────────────────────────────────
            Add(new ElementMixInfo(ElementId.Air, ElementId.Earth,
                "Sandstorm Dust", "Пыль самума",
                "Biting grit wind.", "Колючая пыль.", "#D4A373"));
            Add(new ElementMixInfo(ElementId.Air, ElementId.Nature,
                "Pollen Gale", "Пыльцевой шквал",
                "Living air.", "Живой воздух.", "#52B788"));

            // ── Earth + Nature ────────────────────────────────────────
            Add(new ElementMixInfo(ElementId.Earth, ElementId.Nature,
                "Living Soil", "Живая почва",
                "Rooted earth.", "Корневая земля.", "#40916C"));
        }
    }
}
