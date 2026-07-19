// Card upgrade craft resolve / Резолв крафта/улучшения карты
// Chaos Spellweaver (Картовасия)

using System;
using UnityEngine;
using Kartovasiya.Cards;
using Kartovasiya.Core;

namespace Kartovasiya.Crafting
{
    public readonly struct CraftResult
    {
        public readonly bool Success;
        public readonly bool MaterialsSpent;
        public readonly float ChanceUsed;
        public readonly string MessageRu;
        public readonly string MessageEn;
        public readonly CardInstance Card;

        public CraftResult(bool success, bool spent, float chance, string ru, string en, CardInstance card)
        {
            Success = success;
            MaterialsSpent = spent;
            ChanceUsed = chance;
            MessageRu = ru;
            MessageEn = en;
            Card = card;
        }
    }

    /// <summary>
    /// Applies craft upgrade or failure side-effects (cap success ≤70%).
    /// Улучшение карты или провал (потолок успеха 70%).
    /// </summary>
    public static class CraftService
    {
        public static bool CanCraft(CardInstance card, MaterialInventory inv, out string failRu)
        {
            failRu = null;
            if (card == null)
            {
                failRu = "Нет карты.";
                return false;
            }

            if (card.CardType != CardType.Spell && card.CardType != CardType.Effect)
            {
                failRu = "Крафтить можно заклинания и эффекты.";
                return false;
            }

            CraftCosts.GetUpgradeCost(card.CraftLevel, out int e, out int s, out int d, out int c);
            if (!inv.CanAfford(e, s, d, c))
            {
                failRu = $"Мало материалов (нужно Э{e}/О{s}/П{d}/Я{c}).";
                return false;
            }

            return true;
        }

        /// <summary>
        /// Spend mats + roll with mini-game skill (0..1).
        /// Списать материалы + бросок с учётом мини-игры.
        /// </summary>
        public static CraftResult TryCraft(CardInstance card, MaterialInventory inv, float miniGameSkill01, System.Random rng = null)
        {
            if (!CanCraft(card, inv, out string fail))
                return new CraftResult(false, false, 0f, fail, fail, card);

            CraftCosts.GetUpgradeCost(card.CraftLevel, out int e, out int s, out int d, out int c);
            if (!inv.TrySpend(e, s, d, c))
                return new CraftResult(false, false, 0f, "Ошибка списания.", "Spend failed.", card);

            float chance = CraftCosts.SuccessChance(card.CraftLevel, miniGameSkill01);
            float roll = rng != null ? (float)rng.NextDouble() : UnityEngine.Random.value;
            bool ok = roll < chance;

            if (ok)
            {
                ApplySuccess(card);
                return new CraftResult(true, true, chance,
                    $"Успех! «{card.GetDisplayName(true)}» +1 крафт (lvl {card.CraftLevel}). Шанс был {chance:P0}.",
                    $"Success! «{card.GetDisplayName(false)}» craft lvl {card.CraftLevel}. Chance was {chance:P0}.",
                    card);
            }

            // Fail: materials gone, risk corruption
            bool corrupt = (rng != null ? rng.NextDouble() : UnityEngine.Random.value) < BalanceConfig.CraftFailCorruptChance;
            if (corrupt)
            {
                card.AddFlag(CardFlags.Corrupted);
                card.Mutations.Add(new CardMutation
                {
                    Id = "craft_fail",
                    NameEn = "Failed Weave",
                    NameRu = "Сорванный узор",
                    PowerPercent = 0.05f,
                    IsCorrupted = true,
                    DownsideKey = "self_backlash"
                });
            }

            return new CraftResult(false, true, chance,
                corrupt
                    ? $"Провал крафта ({chance:P0}). Карта повреждена порчей."
                    : $"Провал крафта ({chance:P0}). Материалы сгорели.",
                corrupt
                    ? $"Craft failed ({chance:P0}). Card corrupted."
                    : $"Craft failed ({chance:P0}). Materials lost.",
                card);
        }

        private static void ApplySuccess(CardInstance card)
        {
            card.CraftLevel++;
            card.AddFlag(CardFlags.Crafted);
            card.BasePower += 2 + card.CraftLevel;
            card.RemoveFlag(CardFlags.Exhausted);

            // Light rarity bump chance
            if (card.Rarity < CardRarity.Legendary && UnityEngine.Random.value < 0.2f)
                card.Rarity = (CardRarity)((int)card.Rarity + 1);

            card.Affixes.Add(new CardAffix
            {
                Id = "craft_" + card.CraftLevel,
                NameEn = "Forged",
                NameRu = "Ковка",
                PowerFlat = 1,
                PowerPercent = 0.03f * card.CraftLevel
            });

            // Name polish
            if (!card.DisplayNameRu.Contains("★"))
            {
                card.DisplayNameRu = "★ " + card.DisplayNameRu;
                card.DisplayNameEn = "★ " + card.DisplayNameEn;
            }
        }
    }
}
