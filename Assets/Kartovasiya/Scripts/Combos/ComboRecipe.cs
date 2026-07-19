// Single combo recipe data / Данные одного рецепта комбо
// Chaos Spellweaver (Картовасия)

using System;
using System.Collections.Generic;
using UnityEngine;
using Kartovasiya.Core;

namespace Kartovasiya.Combos
{
    /// <summary>
    /// Design-time recipe: ordered-agnostic set of elements → named combo result.
    /// Рецепт: набор стихий (порядок не важен) → именованный результат комбо.
    /// Examples: Fire+Water=Steam, Aether+Arcane+Light=Divine Nova.
    /// </summary>
    [Serializable]
    public class ComboRecipe
    {
        [Tooltip("Stable string id for saves / analytics. / Стабильный id.")]
        public string Id = "combo_unnamed";

        [Tooltip("Ingredients (2–3 typically). Order ignored when matching. / Ингредиенты, порядок не важен.")]
        public ElementId[] Ingredients = Array.Empty<ElementId>();

        [Header("Names / Имена")]
        public string ResultNameEn = "Combo";
        public string ResultNameRu = "Комбо";

        [TextArea(2, 4)]
        public string DescriptionEn;
        [TextArea(2, 4)]
        public string DescriptionRu;

        [Header("Combat / Бой")]
        [Tooltip("Simple / Complex / Risky.")]
        public ComboComplexity Complexity = ComboComplexity.Simple;

        [Tooltip("Power bonus applied to the host spell. / Бонус силы к заклинанию.")]
        public int PowerBonus = 5;

        [Tooltip("Optional multiplier on spell power (1 = none). / Множитель силы.")]
        public float PowerMultiplier = 1f;

        [Tooltip("0..1 chance of negative side-effect (Risky). / Шанс негативного эффекта.")]
        [Range(0f, 1f)]
        public float RiskChance;

        [Tooltip("Effect key resolved by combat system later. / Ключ эффекта для Combat.")]
        public string EffectKey = "damage_burst";

        [Tooltip("Effect key if risk triggers. / Ключ эффекта при провале риска.")]
        public string RiskEffectKey = "self_backlash";

        [Header("Presentation / Презентация")]
        public Color ResultTint = Color.white;
        public Sprite ResultIcon;

        public string GetResultName(bool russian = true) =>
            russian ? ResultNameRu : ResultNameEn;

        public string GetDescription(bool russian = true) =>
            russian ? DescriptionRu : DescriptionEn;

        /// <summary>
        /// True if bag of elements matches this recipe (multiset equality).
        /// True, если мультимножество стихий совпадает с рецептом.
        /// </summary>
        public bool Matches(IReadOnlyList<ElementId> elements)
        {
            if (Ingredients == null || Ingredients.Length == 0)
                return false;
            if (elements == null || elements.Count != Ingredients.Length)
                return false;

            // Count ingredients
            var need = new Dictionary<ElementId, int>(Ingredients.Length);
            for (int i = 0; i < Ingredients.Length; i++)
            {
                var e = Ingredients[i];
                if (e == ElementId.None) return false;
                need.TryGetValue(e, out int c);
                need[e] = c + 1;
            }

            for (int i = 0; i < elements.Count; i++)
            {
                var e = elements[i];
                if (!need.TryGetValue(e, out int c) || c <= 0)
                    return false;
                need[e] = c - 1;
            }

            foreach (var kv in need)
            {
                if (kv.Value != 0)
                    return false;
            }

            return true;
        }

        /// <summary>
        /// Canonical key for dictionary lookup (sorted element ids joined).
        /// Канонический ключ для словаря (отсортированные id).
        /// </summary>
        public static string BuildCanonicalKey(IReadOnlyList<ElementId> elements)
        {
            if (elements == null || elements.Count == 0)
                return string.Empty;

            var arr = new int[elements.Count];
            for (int i = 0; i < elements.Count; i++)
                arr[i] = (int)elements[i];

            Array.Sort(arr);
            return string.Join("+", arr);
        }

        public string GetCanonicalKey() => BuildCanonicalKey(Ingredients);
    }

    /// <summary>
    /// Result of attempting a combo (success + optional risk roll).
    /// Результат попытки комбо (успех + бросок риска).
    /// </summary>
    public readonly struct ComboResolveResult
    {
        public readonly bool Success;
        public readonly ComboRecipe Recipe;
        public readonly bool RiskTriggered;
        public readonly string AppliedEffectKey;
        public readonly int AppliedPowerBonus;
        public readonly float AppliedPowerMultiplier;

        public ComboResolveResult(
            bool success,
            ComboRecipe recipe,
            bool riskTriggered,
            string effectKey,
            int powerBonus,
            float powerMultiplier)
        {
            Success = success;
            Recipe = recipe;
            RiskTriggered = riskTriggered;
            AppliedEffectKey = effectKey;
            AppliedPowerBonus = powerBonus;
            AppliedPowerMultiplier = powerMultiplier;
        }

        public static ComboResolveResult Fail() =>
            new ComboResolveResult(false, null, false, null, 0, 1f);

        /// <summary>
        /// Fallback when no named recipe: still reward mixing elements.
        /// Сырая смесь без имени — всё равно даёт урон.
        /// </summary>
        public static ComboResolveResult GenericSynergy(int elementCount)
        {
            int n = Mathf.Max(2, elementCount);
            int bonus = 4 + n * 5;           // 2 els → 14, 3 → 19
            float mult = 1.15f + (n - 2) * 0.12f;
            return new ComboResolveResult(true, null, false, "raw_synergy", bonus, mult);
        }
    }
}
