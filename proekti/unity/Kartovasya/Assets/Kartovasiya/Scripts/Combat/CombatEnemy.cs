// Runtime enemy / encounter unit (solo or swarm pack)
// Runtime-враг: одиночка или стая с несколькими юнитами
// Chaos Spellweaver (Картовасия)

using System;
using UnityEngine;
using Kartovasiya.Core;

namespace Kartovasiya.Combat
{
    /// <summary>
    /// Arcade enemy. Swarm types track UnitsAlive with reduced per-unit stats.
    /// </summary>
    [Serializable]
    public class CombatEnemy
    {
        public string NameEn = "Wisp";
        public string NameRu = "Огонёк";
        public int MaxHp = 30;
        public int CurrentHp = 30;
        public int AttackDamage = 5;
        public int Level = 1;
        public bool IsBoss;

        public string ArchetypeId = "";
        public EnemyVisual Visual = EnemyVisual.Goblin;
        public EnemyCategory Category = EnemyCategory.Fantasy;
        public ElementId Affinity = ElementId.None;

        /// <summary>Swarm pack: several weaker bodies sharing one HP bar (sum).</summary>
        public bool IsSwarm;
        public int UnitCount = 1;
        public int UnitsAlive = 1;
        /// <summary>HP per one unit at spawn (for kill-count display).</summary>
        public int HpPerUnit = 0;
        /// <summary>Attack contributed by each living unit.</summary>
        public int AttackPerUnit = 0;

        public bool IsDead => CurrentHp <= 0 || UnitsAlive <= 0;

        public string GetName(bool russian = true)
        {
            string baseName = russian ? NameRu : NameEn;
            if (IsSwarm && UnitCount > 1)
                return $"{baseName} ×{UnitsAlive}";
            return baseName;
        }

        public string GetSubtitle(bool russian = true)
        {
            if (IsSwarm && UnitCount > 1)
            {
                return russian
                    ? $"Стая: {UnitsAlive}/{UnitCount} · ATK {GetAttackDamage()}"
                    : $"Pack: {UnitsAlive}/{UnitCount} · ATK {GetAttackDamage()}";
            }

            return russian
                ? $"ATK {GetAttackDamage()}" + (IsBoss ? " · БОСС" : "")
                : $"ATK {GetAttackDamage()}" + (IsBoss ? " · BOSS" : "");
        }

        public int GetAttackDamage()
        {
            if (IsSwarm && AttackPerUnit > 0)
                return AttackPerUnit * Mathf.Max(0, UnitsAlive);
            return AttackDamage;
        }

        public event Action<int, int> HpChanged; // current, max
        public event Action Died;

        public void Reset(
            int maxHp, int attack, int level,
            string nameEn, string nameRu, bool isBoss = false)
        {
            Level = level;
            MaxHp = maxHp;
            CurrentHp = maxHp;
            AttackDamage = attack;
            AttackPerUnit = attack;
            NameEn = nameEn;
            NameRu = nameRu;
            IsBoss = isBoss;
            IsSwarm = false;
            UnitCount = 1;
            UnitsAlive = 1;
            HpPerUnit = maxHp;
            ArchetypeId = "";
            Visual = EnemyVisual.Goblin;
            Category = EnemyCategory.Fantasy;
            Affinity = ElementId.None;
            HpChanged?.Invoke(CurrentHp, MaxHp);
        }

        /// <summary>Full spawn from catalog definition.</summary>
        public void ResetFromDef(
            EnemyDef def, int level, int soloHp, int soloAtk, int unitCount, bool forceBoss)
        {
            if (def == null)
            {
                Reset(soloHp, soloAtk, level, "Enemy", "Враг", forceBoss);
                return;
            }

            Level = level;
            ArchetypeId = def.Id;
            Visual = def.Visual;
            Category = def.Category;
            Affinity = def.Affinity;
            IsBoss = forceBoss || def.IsNamedBoss;
            NameEn = def.NameEn;
            NameRu = def.NameRu;
            IsSwarm = def.IsSwarm && unitCount > 1;
            UnitCount = Mathf.Max(1, unitCount);
            UnitsAlive = UnitCount;

            if (IsSwarm)
            {
                HpPerUnit = Mathf.Max(3, Mathf.RoundToInt(soloHp * def.SwarmHpFactor * def.HpMult));
                AttackPerUnit = Mathf.Max(1, Mathf.RoundToInt(soloAtk * def.SwarmAtkFactor * def.AtkMult));
                MaxHp = HpPerUnit * UnitCount;
                CurrentHp = MaxHp;
                AttackDamage = AttackPerUnit * UnitCount; // max pack atk (display baseline)
            }
            else
            {
                MaxHp = Mathf.Max(5, Mathf.RoundToInt(soloHp * def.HpMult));
                CurrentHp = MaxHp;
                AttackPerUnit = Mathf.Max(1, Mathf.RoundToInt(soloAtk * def.AtkMult));
                AttackDamage = AttackPerUnit;
                HpPerUnit = MaxHp;
                UnitCount = 1;
                UnitsAlive = 1;
            }

            if (IsBoss && !IsSwarm)
            {
                // Named/wave bosses already scaled outside; tiny polish
                MaxHp = Mathf.RoundToInt(MaxHp * 1.05f);
                CurrentHp = MaxHp;
            }

            HpChanged?.Invoke(CurrentHp, MaxHp);
        }

        public void SetHp(int hp)
        {
            CurrentHp = Mathf.Clamp(hp, 0, MaxHp);
            RefreshUnitsFromHp();
            HpChanged?.Invoke(CurrentHp, MaxHp);
            if (CurrentHp <= 0)
                Died?.Invoke();
        }

        public int TakeDamage(int amount)
        {
            amount = Mathf.Max(0, amount);
            int before = UnitsAlive;
            CurrentHp = Mathf.Max(0, CurrentHp - amount);
            RefreshUnitsFromHp();
            HpChanged?.Invoke(CurrentHp, MaxHp);
            if (CurrentHp <= 0 || UnitsAlive <= 0)
                Died?.Invoke();
            return amount;
        }

        /// <summary>How many units died from last HP change (for log).</summary>
        public int LastUnitsKilled { get; private set; }

        private void RefreshUnitsFromHp()
        {
            if (!IsSwarm || HpPerUnit <= 0)
            {
                UnitsAlive = CurrentHp > 0 ? 1 : 0;
                LastUnitsKilled = 0;
                return;
            }

            int prev = UnitsAlive;
            UnitsAlive = CurrentHp <= 0 ? 0 : Mathf.CeilToInt(CurrentHp / (float)HpPerUnit);
            UnitsAlive = Mathf.Clamp(UnitsAlive, 0, UnitCount);
            LastUnitsKilled = Mathf.Max(0, prev - UnitsAlive);
        }

        public int Heal(int amount)
        {
            amount = Mathf.Max(0, amount);
            CurrentHp = Mathf.Min(MaxHp, CurrentHp + amount);
            RefreshUnitsFromHp();
            HpChanged?.Invoke(CurrentHp, MaxHp);
            return amount;
        }
    }
}
