// Central balance knobs for the playable prototype
// Центральный баланс прототипа
// Chaos Spellweaver (Картовасия)

using UnityEngine;

namespace Kartovasiya.Core
{
    /// <summary>
    /// Tunable values (static for prototype; later → ScriptableObject).
    /// Настройки баланса (позже вынести в SO).
    /// </summary>
    public static class BalanceConfig
    {
        // ── Player ────────────────────────────────────────────────────
        public const int PlayerStartHp = 44;
        public const int CardsPerTurn = 5;
        public const int OpeningHand = 6;
        public const int CardsDroppedPerTurn = 3;
        public const int HealOnNormalKill = 3;
        public const int HealOnBossKill = 7;

        // ── Enemy curve ───────────────────────────────────────────────
        public const float EnemyHpBase = 18f;
        public const float EnemyHpPerLevel = 8.5f;
        public const float EnemyAtkBase = 3.5f;
        public const float EnemyAtkPerLevel = 1.7f;
        public const float BossHpMult = 1.9f;
        public const float BossAtkMult = 1.4f;
        public const int BossHpFlat = 18;
        public const int BossAtkFlat = 3;
        public const int BossEveryN = 5;

        // ── Loot ──────────────────────────────────────────────────────
        public const float ShardDropChance = 0.4f;
        public static int EssenceOnKill(int enemyIndex, bool boss) =>
            boss ? 4 : 1 + enemyIndex / 4;

        public static int ShardOnKill(bool boss, System.Random rng = null)
        {
            if (boss) return 2;
            float r = rng != null ? (float)rng.NextDouble() : Random.value;
            return r < ShardDropChance ? 1 : 0;
        }

        // ── Spell power ───────────────────────────────────────────────
        public const float CritMultiplier = 1.5f;
        public const float CorruptionBacklashChance = 0.18f;
        public const float CorruptionPowerBonus = 1.12f;

        // ── Craft ─────────────────────────────────────────────────────
        public const float CraftBaseChance = 0.38f;
        public const float CraftMaxChance = 0.70f;
        public const float CraftSkillBonusMax = 0.34f;
        public const float CraftFailCorruptChance = 0.28f;

        // ── Events ────────────────────────────────────────────────────
        public const int GlobalEventEveryTurns = 4;
        public const int EventSkipPenaltyHp = 2;
        public const int EventFailPenaltyHp = 3;

        // ── Mobile ────────────────────────────────────────────────────
        public const int TargetFps = 60;
        public const int LowEndFps = 30;
    }
}
