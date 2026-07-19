// Combo recipe registry / Каталог рецептов комбо
// Chaos Spellweaver (Картовасия)

using System.Collections.Generic;
using UnityEngine;
using Kartovasiya.Core;

namespace Kartovasiya.Combos
{
    /// <summary>
    /// ScriptableObject database of elemental fusion recipes.
    /// SO-каталог слияний стихий (Simple / Complex / Risky).
    /// </summary>
    [CreateAssetMenu(
        fileName = "ComboDatabase",
        menuName = "Kartovasiya/Combos/Combo Database",
        order = 10)]
    public class ComboDatabaseSO : ScriptableObject
    {
        [SerializeField] private List<ComboRecipe> _recipes = new List<ComboRecipe>();

        private Dictionary<string, ComboRecipe> _byKey;

        public IReadOnlyList<ComboRecipe> Recipes => _recipes;

        /// <summary>
        /// Runtime / prototype: load recipes without Editor (e.g. ComboDefaults.CreateAll()).
        /// Runtime: загрузить рецепты без Editor (из ComboDefaults).
        /// </summary>
        public void SetRecipes(List<ComboRecipe> recipes)
        {
            _recipes = recipes ?? new List<ComboRecipe>();
            _byKey = null;
        }

        /// <summary>Seed prototype database from ComboDefaults. / Сид из ComboDefaults.</summary>
        public void LoadPrototypeDefaults()
        {
            SetRecipes(ComboDefaults.CreateAll());
        }

        public void EnsureCache()
        {
            if (_byKey != null && _byKey.Count == _recipes.Count)
                return;

            _byKey = new Dictionary<string, ComboRecipe>(_recipes.Count);
            for (int i = 0; i < _recipes.Count; i++)
            {
                var r = _recipes[i];
                if (r == null || r.Ingredients == null || r.Ingredients.Length == 0)
                    continue;

                string key = r.GetCanonicalKey();
                if (string.IsNullOrEmpty(key))
                    continue;

                if (_byKey.ContainsKey(key))
                {
                    Debug.LogWarning($"[ComboDatabase] Duplicate recipe key '{key}' ({r.Id}).", this);
                    continue;
                }

                _byKey.Add(key, r);
            }
        }

        /// <summary>
        /// Find a recipe matching the multiset of elements (any order).
        /// Найти рецепт по мультимножеству стихий (порядок не важен).
        /// </summary>
        public bool TryFind(IReadOnlyList<ElementId> elements, out ComboRecipe recipe)
        {
            EnsureCache();
            recipe = null;
            if (elements == null || elements.Count == 0)
                return false;

            string key = ComboRecipe.BuildCanonicalKey(elements);
            return _byKey.TryGetValue(key, out recipe);
        }

        /// <summary>
        /// Resolve combo: exact match → best subset → generic synergy fallback.
        /// Точный рецепт → лучший поднабор → сырая синергия (всегда есть смысл мешать).
        /// </summary>
        public ComboResolveResult Resolve(IReadOnlyList<ElementId> elements, System.Random rng = null)
        {
            if (elements == null || elements.Count < 2)
                return ComboResolveResult.Fail();

            // 1) Exact multiset
            if (TryFind(elements, out var recipe))
                return FinishResolve(recipe, rng);

            // 2) Best subset recipe (e.g. Fire+Water+Lightning still finds Steam, then prefer 3-part)
            EnsureCache();
            ComboRecipe best = null;
            int bestLen = 0;
            int bestPower = -1;
            for (int i = 0; i < _recipes.Count; i++)
            {
                var r = _recipes[i];
                if (r?.Ingredients == null || r.Ingredients.Length < 2)
                    continue;
                if (!IsSubsetOf(r.Ingredients, elements))
                    continue;

                int len = r.Ingredients.Length;
                int power = r.PowerBonus;
                if (len > bestLen || (len == bestLen && power > bestPower))
                {
                    best = r;
                    bestLen = len;
                    bestPower = power;
                }
            }

            if (best != null)
            {
                // Extra power for leftover elements beyond the recipe
                int extra = Mathf.Max(0, elements.Count - best.Ingredients.Length);
                var core = FinishResolve(best, rng);
                if (extra > 0)
                {
                    return new ComboResolveResult(
                        true,
                        best,
                        core.RiskTriggered,
                        core.AppliedEffectKey,
                        core.AppliedPowerBonus + extra * 4,
                        core.AppliedPowerMultiplier + extra * 0.08f);
                }

                return core;
            }

            // 3) Generic synergy — any mix still boosts damage
            return ComboResolveResult.GenericSynergy(elements.Count);
        }

        private static ComboResolveResult FinishResolve(ComboRecipe recipe, System.Random rng)
        {
            bool risk = false;
            string effect = recipe.EffectKey;

            if (recipe.Complexity == ComboComplexity.Risky || recipe.RiskChance > 0f)
            {
                float roll = rng != null
                    ? (float)rng.NextDouble()
                    : UnityEngine.Random.value;

                float chance = recipe.RiskChance > 0f
                    ? recipe.RiskChance
                    : GameConstants.DefaultRiskyComboFailChance;

                if (roll < chance)
                {
                    risk = true;
                    effect = string.IsNullOrEmpty(recipe.RiskEffectKey)
                        ? "self_backlash"
                        : recipe.RiskEffectKey;
                }
            }

            return new ComboResolveResult(
                true,
                recipe,
                risk,
                effect,
                recipe.PowerBonus,
                recipe.PowerMultiplier);
        }

        /// <summary>True if every ingredient appears in bag (multiset). / Рецепт ⊆ bag.</summary>
        private static bool IsSubsetOf(ElementId[] ingredients, IReadOnlyList<ElementId> bag)
        {
            var have = new Dictionary<ElementId, int>(bag.Count);
            for (int i = 0; i < bag.Count; i++)
            {
                var e = bag[i];
                if (e == ElementId.None) continue;
                have.TryGetValue(e, out int c);
                have[e] = c + 1;
            }

            for (int i = 0; i < ingredients.Length; i++)
            {
                var e = ingredients[i];
                if (e == ElementId.None) return false;
                if (!have.TryGetValue(e, out int c) || c <= 0)
                    return false;
                have[e] = c - 1;
            }

            return true;
        }

        /// <summary>
        /// Partial match helpers for UI hints (e.g. one element already on spell).
        /// Частичные совпадения для UI-подсказок.
        /// </summary>
        public List<ComboRecipe> FindRecipesContaining(ElementId element)
        {
            var result = new List<ComboRecipe>();
            for (int i = 0; i < _recipes.Count; i++)
            {
                var r = _recipes[i];
                if (r?.Ingredients == null) continue;
                for (int j = 0; j < r.Ingredients.Length; j++)
                {
                    if (r.Ingredients[j] == element)
                    {
                        result.Add(r);
                        break;
                    }
                }
            }

            return result;
        }

#if UNITY_EDITOR
        public void EditorSetRecipes(List<ComboRecipe> recipes)
        {
            _recipes = recipes ?? new List<ComboRecipe>();
            _byKey = null;
            UnityEditor.EditorUtility.SetDirty(this);
        }
#endif

        private void OnEnable() => _byKey = null;
        private void OnValidate() => _byKey = null;
    }
}
