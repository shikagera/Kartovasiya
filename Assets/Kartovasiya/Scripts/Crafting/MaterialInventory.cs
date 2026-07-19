// Craft materials inventory / Инвентарь материалов крафта
// Chaos Spellweaver (Картовасия)

using System;
using UnityEngine;
using Kartovasiya.Core;

namespace Kartovasiya.Crafting
{
    public enum MaterialType
    {
        Essence = 0,    // Обычная эссенция (с врагов)
        Shard = 1,      // Осколок стихии
        ChaosDust = 2,  // Пыль хаоса (события)
        BossCore = 3    // Ядро босса
    }

    /// <summary>
    /// Runtime bag of craft mats (saved with run).
    /// Сумка материалов (сохраняется с run).
    /// </summary>
    [Serializable]
    public class MaterialInventory
    {
        public int Essence;
        public int Shard;
        public int ChaosDust;
        public int BossCore;

        public event Action Changed;

        public int Get(MaterialType t) => t switch
        {
            MaterialType.Essence => Essence,
            MaterialType.Shard => Shard,
            MaterialType.ChaosDust => ChaosDust,
            MaterialType.BossCore => BossCore,
            _ => 0
        };

        public void Add(MaterialType t, int amount)
        {
            if (amount == 0) return;
            switch (t)
            {
                case MaterialType.Essence: Essence = Mathf.Max(0, Essence + amount); break;
                case MaterialType.Shard: Shard = Mathf.Max(0, Shard + amount); break;
                case MaterialType.ChaosDust: ChaosDust = Mathf.Max(0, ChaosDust + amount); break;
                case MaterialType.BossCore: BossCore = Mathf.Max(0, BossCore + amount); break;
            }

            Changed?.Invoke();
        }

        public void NotifyChanged() => Changed?.Invoke();

        public bool CanAfford(int essence, int shard, int dust, int core) =>
            Essence >= essence && Shard >= shard && ChaosDust >= dust && BossCore >= core;

        public bool TrySpend(int essence, int shard, int dust, int core)
        {
            if (!CanAfford(essence, shard, dust, core))
                return false;
            Essence -= essence;
            Shard -= shard;
            ChaosDust -= dust;
            BossCore -= core;
            Changed?.Invoke();
            return true;
        }

        public void Clear()
        {
            Essence = Shard = ChaosDust = BossCore = 0;
            Changed?.Invoke();
        }

        public string Summary(bool ru = true)
        {
            return ru
                ? $"Эсс:{Essence}  Оск:{Shard}  Пыль:{ChaosDust}  Ядро:{BossCore}"
                : $"Ess:{Essence}  Shard:{Shard}  Dust:{ChaosDust}  Core:{BossCore}";
        }
    }

    public static class CraftCosts
    {
        /// <summary>Cost scales with craft level of the card. / Стоимость растёт с уровнем крафта.</summary>
        public static void GetUpgradeCost(int craftLevel, out int essence, out int shard, out int dust, out int core)
        {
            essence = 2 + craftLevel;
            shard = craftLevel >= 1 ? 1 + craftLevel / 2 : 0;
            dust = craftLevel >= 2 ? 1 : 0;
            core = craftLevel >= 3 ? 1 : 0;
        }

        public static float SuccessChance(int craftLevel, float miniGameSkill01)
        {
            float baseChance = BalanceConfig.CraftBaseChance + craftLevel * -0.03f; // harder each level
            float skillBonus = Mathf.Clamp01(miniGameSkill01) * BalanceConfig.CraftSkillBonusMax;
            return Mathf.Clamp(baseChance + skillBonus, 0.05f, BalanceConfig.CraftMaxChance);
        }
    }
}
