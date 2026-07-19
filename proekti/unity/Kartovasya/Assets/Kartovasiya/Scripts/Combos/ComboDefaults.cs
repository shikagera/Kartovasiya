// Seed combo recipes — expanded synergies / Расширенные синергии
// Chaos Spellweaver (Картовасия)

using System.Collections.Generic;
using UnityEngine;
using Kartovasiya.Core;

namespace Kartovasiya.Combos
{
    /// <summary>
    /// Default combo table. Any two basic mixes should feel rewarding.
    /// </summary>
    public static class ComboDefaults
    {
        public static List<ComboRecipe> CreateAll()
        {
            return new List<ComboRecipe>
            {
                // ── Core mixes (player examples + classics) ───────────
                Recipe("steam",
                    new[] { ElementId.Fire, ElementId.Water },
                    "Steam", "Пар",
                    "Scalding mist.", "Обжигающий пар.",
                    ComboComplexity.Simple, 10, 1.35f, 0f, "steam_burst", null, "#8ECAE6"),

                // Lightning + Steam (Fire+Water+Lightning)
                Recipe("electrified_steam",
                    new[] { ElementId.Fire, ElementId.Water, ElementId.Lightning },
                    "Electrified Steam", "Наэлектризованный пар",
                    "Conductive scald — heavy shock burn.",
                    "Проводящий пар — сильный шок и ожог.",
                    ComboComplexity.Complex, 22, 1.75f, 0f, "electro_steam", null, "#4CC9F0"),

                // Earth + Fire = burning boulder (was lava)
                Recipe("burning_boulder",
                    new[] { ElementId.Fire, ElementId.Earth },
                    "Burning Boulder", "Горящий булыжник",
                    "Molten rock smash — heavy impact burn.",
                    "Раскалённый камень — тяжёлый удар и ожог.",
                    ComboComplexity.Simple, 14, 1.45f, 0f, "lava_wave", null, "#DC2F02"),

                Recipe("lava",
                    new[] { ElementId.Fire, ElementId.Earth, ElementId.Air },
                    "Lava Storm", "Лавовая буря",
                    "Erupted magma rain.", "Дождь магмы.",
                    ComboComplexity.Complex, 20, 1.65f, 0f, "lava_storm", null, "#E85D04"),

                Recipe("mud",
                    new[] { ElementId.Water, ElementId.Earth },
                    "Mud", "Грязь",
                    "Slow and crush.", "Замедление и давление.",
                    ComboComplexity.Simple, 8, 1.25f, 0f, "mud_trap", null, "#6B705C"),

                Recipe("frostbite",
                    new[] { ElementId.Ice, ElementId.Water },
                    "Frostbite", "Обморожение",
                    "Deep chill.", "Глубокий холод.",
                    ComboComplexity.Simple, 11, 1.35f, 0f, "frostbite", null, "#48CAE4"),

                Recipe("chain_lightning",
                    new[] { ElementId.Air, ElementId.Lightning },
                    "Chain Lightning", "Цепная молния",
                    "Arcing shocks.", "Разряд прыгает.",
                    ComboComplexity.Simple, 12, 1.4f, 0f, "chain_lightning", null, "#9D4EDD"),

                Recipe("thunderclap",
                    new[] { ElementId.Lightning, ElementId.Water },
                    "Thunderclap", "Громовой хлопок",
                    "Shock through moisture.", "Удар через влагу.",
                    ComboComplexity.Simple, 13, 1.4f, 0f, "thunder_water", null, "#7209B7"),

                Recipe("plasma",
                    new[] { ElementId.Lightning, ElementId.Fire },
                    "Plasma", "Плазма",
                    "Superheated arc.", "Сверхгорячая дуга.",
                    ComboComplexity.Simple, 14, 1.45f, 0f, "plasma", null, "#F72585"),

                Recipe("magma_ice",
                    new[] { ElementId.Fire, ElementId.Ice },
                    "Thermal Shock", "Термошок",
                    "Crack and shatter.", "Треск и раскол.",
                    ComboComplexity.Simple, 13, 1.4f, 0f, "thermal_shock", null, "#FF6B35"),

                Recipe("sandstorm",
                    new[] { ElementId.Air, ElementId.Earth },
                    "Sandstorm", "Песчаная буря",
                    "Blind + chip.", "Ослепление и срез.",
                    ComboComplexity.Simple, 9, 1.3f, 0f, "sandstorm", null, "#D4A373"),

                Recipe("wildfire",
                    new[] { ElementId.Fire, ElementId.Nature },
                    "Wildfire", "Палы",
                    "Spreading flames.", "Распространяющийся огонь.",
                    ComboComplexity.Simple, 12, 1.4f, 0f, "wildfire", null, "#E85D04"),

                Recipe("eclipse",
                    new[] { ElementId.Shadow, ElementId.Light },
                    "Eclipse", "Затмение",
                    "Twilight cut — light vs shadow.", "Сумеречный удар — свет против тьмы.",
                    ComboComplexity.Simple, 16, 1.55f, 0f, "eclipse_slash", null, "#3C096C"),

                Recipe("blizzard",
                    new[] { ElementId.Ice, ElementId.Air },
                    "Blizzard", "Метель",
                    "Freezing winds.", "Ледяной ветер.",
                    ComboComplexity.Simple, 11, 1.35f, 0f, "blizzard", null, "#90E0EF"),

                Recipe("quake",
                    new[] { ElementId.Earth, ElementId.Lightning },
                    "Quake", "Землетрясение",
                    "Shock through stone.", "Сотрясение камня.",
                    ComboComplexity.Simple, 12, 1.4f, 0f, "quake", null, "#6C584C"),

                Recipe("mire",
                    new[] { ElementId.Nature, ElementId.Water },
                    "Mire", "Трясина",
                    "Grasping roots and wet.", "Корни и сырость.",
                    ComboComplexity.Simple, 9, 1.3f, 0f, "mire", null, "#2D6A4F"),

                Recipe("shadowflame",
                    new[] { ElementId.Fire, ElementId.Shadow },
                    "Dark Fire", "Тёмный огонь",
                    "Shadow-fed flames — bonus burn.",
                    "Пламя тьмы — усиленный ожог.",
                    ComboComplexity.Simple, 15, 1.5f, 0f, "shadowflame", null, "#5A189A"),

                Recipe("dark_lightning",
                    new[] { ElementId.Lightning, ElementId.Shadow },
                    "Dark Lightning", "Тёмная молния",
                    "Void-charged bolt.",
                    "Разряд пустоты.",
                    ComboComplexity.Simple, 15, 1.5f, 0f, "dark_lightning", null, "#3C096C"),

                Recipe("black_tide",
                    new[] { ElementId.Water, ElementId.Shadow },
                    "Black Tide", "Чёрный прилив",
                    "Dark crushing water.",
                    "Тёмная давящая вода.",
                    ComboComplexity.Simple, 12, 1.4f, 0f, "black_tide", null, "#240046"),

                Recipe("storm_call",
                    new[] { ElementId.Lightning, ElementId.Storm },
                    "Storm Call", "Зов бури",
                    "Amplified tempest.", "Усиленная буря.",
                    ComboComplexity.Simple, 16, 1.5f, 0f, "storm_call", null, "#4CC9F0"),

                Recipe("ice_shard",
                    new[] { ElementId.Ice, ElementId.Earth },
                    "Ice Shard", "Ледяной осколок",
                    "Frozen rock spears.", "Копья из льда и камня.",
                    ComboComplexity.Simple, 11, 1.35f, 0f, "ice_shard", null, "#CAF0F8"),

                Recipe("poison_cloud",
                    new[] { ElementId.Poison, ElementId.Air },
                    "Poison Cloud", "Ядовитое облако",
                    "Toxic haze.", "Токсичная дымка.",
                    ComboComplexity.Simple, 11, 1.35f, 0f, "poison_cloud", null, "#52B788"),

                Recipe("toxic_blaze",
                    new[] { ElementId.Fire, ElementId.Poison },
                    "Toxic Blaze", "Ядовитое пламя",
                    "Burn + toxin.", "Ожог и токсин.",
                    ComboComplexity.Complex, 16, 1.5f, 0f, "toxic_blaze", null, "#40916C"),

                Recipe("toxic_inferno",
                    new[] { ElementId.Fire, ElementId.Poison, ElementId.Earth },
                    "Toxic Inferno", "Токсичный ад",
                    "Heavy DoT inferno.", "Сильный DoT-ад.",
                    ComboComplexity.Complex, 24, 1.8f, 0f, "toxic_inferno", null, "#52B788"),

                Recipe("metal_spark",
                    new[] { ElementId.Metal, ElementId.Lightning },
                    "Metal Spark", "Металлическая искра",
                    "Conductive pierce.", "Проводящий пробой.",
                    ComboComplexity.Simple, 14, 1.45f, 0f, "metal_spark", null, "#ADB5BD"),

                Recipe("crystal_arc",
                    new[] { ElementId.Crystal, ElementId.Lightning },
                    "Crystal Arc", "Кристальная дуга",
                    "Prism lightning.", "Молния через призму.",
                    ComboComplexity.Complex, 18, 1.55f, 0f, "crystal_arc", null, "#B5179E"),

                Recipe("blood_storm",
                    new[] { ElementId.Blood, ElementId.Storm },
                    "Blood Storm", "Кровавая буря",
                    "Lifesteal tempest.", "Буря с вампиризмом.",
                    ComboComplexity.Complex, 18, 1.55f, 0f, "blood_storm", null, "#9B2226"),

                Recipe("divine_nova",
                    new[] { ElementId.Aether, ElementId.Arcane, ElementId.Light },
                    "Divine Nova", "Божественная Нова",
                    "Prismatic burst.", "Призматический взрыв.",
                    ComboComplexity.Complex, 28, 1.9f, 0f, "divine_nova", null, "#F72585"),

                // ── Risky high-power ──────────────────────────────────
                Recipe("void_rift",
                    new[] { ElementId.Aether, ElementId.Shadow },
                    "Void Rift", "Разлом пустоты",
                    "Huge damage — may backlash.",
                    "Огромный урон — может ударить по тебе.",
                    ComboComplexity.Risky, 22, 1.7f, 0.28f, "void_rift", "self_backlash", "#3A0CA3"),

                Recipe("soul_pact",
                    new[] { ElementId.Blood, ElementId.Arcane },
                    "Soul Pact", "Душевный пакт",
                    "Pay HP for power.",
                    "Платишь HP за силу.",
                    ComboComplexity.Risky, 24, 1.8f, 0.32f, "soul_pact", "corruption_spike", "#AE2012"),

                Recipe("overcharge",
                    new[] { ElementId.Lightning, ElementId.Metal },
                    "Overcharge", "Перегрузка",
                    "Piercing shock — self-stun risk.",
                    "Пробивающий шок — риск оглушить себя.",
                    ComboComplexity.Risky, 20, 1.65f, 0.22f, "overcharge", "self_stun", "#7B2CBF"),

                // Lightning + Steam-like via Ice+Fire+Lightning
                Recipe("charged_frost",
                    new[] { ElementId.Ice, ElementId.Lightning },
                    "Charged Frost", "Заряженный иней",
                    "Static ice needles.", "Ледяные иглы с током.",
                    ComboComplexity.Simple, 13, 1.4f, 0f, "charged_frost", null, "#48CAE4"),

                Recipe("scalding_mud",
                    new[] { ElementId.Fire, ElementId.Water, ElementId.Earth },
                    "Scalding Mud", "Кипящая грязь",
                    "Boiling earth trap.", "Кипящая земная ловушка.",
                    ComboComplexity.Complex, 18, 1.6f, 0f, "scalding_mud", null, "#BC6C25"),
            };
        }

        private static ComboRecipe Recipe(
            string id,
            ElementId[] ingredients,
            string nameEn,
            string nameRu,
            string descEn,
            string descRu,
            ComboComplexity complexity,
            int powerBonus,
            float powerMult,
            float risk,
            string effectKey,
            string riskKey,
            string tintHex)
        {
            ColorUtility.TryParseHtmlString(tintHex, out var tint);
            return new ComboRecipe
            {
                Id = id,
                Ingredients = ingredients,
                ResultNameEn = nameEn,
                ResultNameRu = nameRu,
                DescriptionEn = descEn,
                DescriptionRu = descRu,
                Complexity = complexity,
                PowerBonus = powerBonus,
                PowerMultiplier = powerMult,
                RiskChance = risk,
                EffectKey = effectKey,
                RiskEffectKey = riskKey ?? "self_backlash",
                ResultTint = tint
            };
        }
    }
}
