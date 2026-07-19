// Serializable run snapshot / Снимок run для JSON
// Chaos Spellweaver (Картовасия)

using System;
using System.Collections.Generic;
using Kartovasiya.Cards;
using Kartovasiya.Core;
using UnityEngine;

namespace Kartovasiya.Save
{
    /// <summary>
    /// Full offline run state written to disk.
    /// Полное состояние run (оффлайн-сейв).
    /// JsonUtility-friendly: arrays only, no Dictionary.
    /// </summary>
    [Serializable]
    public class RunSaveData
    {
        public const int CurrentVersion = 3;

        public int version = CurrentVersion;
        public long savedUtcTicks;
        public string product = GameConstants.ProductName;

        // RNG / meta
        public int seed;
        public int turnNumber;
        public int enemyIndex;

        // Player
        public int playerHp;
        public int playerMaxHp;
        public int cardsRemaining;
        public int cardsPerTurnLimit;
        public int phase; // TurnPhase as int

        // Enemy
        public EnemySaveData enemy = new EnemySaveData();

        // Hand
        public CardSaveData[] hand = Array.Empty<CardSaveData>();

        // Combo power cache on spells
        public ComboPowerSave[] comboPowers = Array.Empty<ComboPowerSave>();

        // Craft materials (v2)
        public int matEssence;
        public int matShard;
        public int matChaosDust;
        public int matBossCore;

        // Committed Effect buffs (v3) — locked in by playing buff on self
        public int committedCardsPerTurnBonus;
        public float committedCritBonus;
        public int committedFloodBonus;
    }

    [Serializable]
    public class EnemySaveData
    {
        public string nameEn;
        public string nameRu;
        public int maxHp;
        public int currentHp;
        public int attackDamage;
        public int level;
        public bool isBoss;
        public string archetypeId;
        public int visual;
        public bool isSwarm;
        public int unitCount = 1;
        public int unitsAlive = 1;
        public int hpPerUnit;
        public int attackPerUnit;
        public int affinity;
    }

    [Serializable]
    public class ComboPowerSave
    {
        public string instanceId;
        public int bonus;
        public float mult = 1f;
    }

    /// <summary>
    /// Flattened CardInstance for JsonUtility.
    /// Плоский DTO карты (Color → 4 float, lists → arrays).
    /// </summary>
    [Serializable]
    public class CardSaveData
    {
        public string instanceId;
        public string templateId;
        public string displayNameEn;
        public string displayNameRu;
        public int cardType;
        public int rarity;
        public int flags;

        public int providedElement;
        public int primaryAffinity;
        public int elementSockets;
        public int[] infusedElements = Array.Empty<int>();
        public int[] compoundElements = Array.Empty<int>();

        public string activeComboId;
        public string activeComboNameEn;
        public string activeComboNameRu;
        public float comboTintR = 1f;
        public float comboTintG = 1f;
        public float comboTintB = 1f;
        public float comboTintA = 1f;

        public int basePower;
        public int costCards = 1;
        public int craftLevel;

        public int cardsPerTurnBonus;
        public float critChanceBonus;
        public int cardFloodBonus;

        public CardAffix[] affixes = Array.Empty<CardAffix>();
        public CardMutation[] mutations = Array.Empty<CardMutation>();

        public string descriptionEn;
        public string descriptionRu;

        public static CardSaveData FromInstance(CardInstance c)
        {
            if (c == null) return null;

            var dto = new CardSaveData
            {
                instanceId = c.InstanceId,
                templateId = c.TemplateId,
                displayNameEn = c.DisplayNameEn,
                displayNameRu = c.DisplayNameRu,
                cardType = (int)c.CardType,
                rarity = (int)c.Rarity,
                flags = (int)c.Flags,
                providedElement = (int)c.ProvidedElement,
                primaryAffinity = (int)c.PrimaryAffinity,
                elementSockets = c.ElementSockets,
                activeComboId = c.ActiveComboId,
                activeComboNameEn = c.ActiveComboNameEn,
                activeComboNameRu = c.ActiveComboNameRu,
                comboTintR = c.ActiveComboTint.r,
                comboTintG = c.ActiveComboTint.g,
                comboTintB = c.ActiveComboTint.b,
                comboTintA = c.ActiveComboTint.a,
                basePower = c.BasePower,
                costCards = c.CostCards,
                craftLevel = c.CraftLevel,
                cardsPerTurnBonus = c.CardsPerTurnBonus,
                critChanceBonus = c.CritChanceBonus,
                cardFloodBonus = c.CardFloodBonus,
                descriptionEn = c.DescriptionEn,
                descriptionRu = c.DescriptionRu
            };

            if (c.InfusedElements != null && c.InfusedElements.Count > 0)
            {
                dto.infusedElements = new int[c.InfusedElements.Count];
                for (int i = 0; i < c.InfusedElements.Count; i++)
                    dto.infusedElements[i] = (int)c.InfusedElements[i];
            }

            if (c.CompoundElements != null && c.CompoundElements.Count > 0)
            {
                dto.compoundElements = new int[c.CompoundElements.Count];
                for (int i = 0; i < c.CompoundElements.Count; i++)
                    dto.compoundElements[i] = (int)c.CompoundElements[i];
            }

            if (c.Affixes != null && c.Affixes.Count > 0)
            {
                dto.affixes = new CardAffix[c.Affixes.Count];
                for (int i = 0; i < c.Affixes.Count; i++)
                    dto.affixes[i] = c.Affixes[i]?.Clone();
            }

            if (c.Mutations != null && c.Mutations.Count > 0)
            {
                dto.mutations = new CardMutation[c.Mutations.Count];
                for (int i = 0; i < c.Mutations.Count; i++)
                    dto.mutations[i] = c.Mutations[i]?.Clone();
            }

            return dto;
        }

        public CardInstance ToInstance()
        {
            var c = new CardInstance
            {
                InstanceId = string.IsNullOrEmpty(instanceId)
                    ? Guid.NewGuid().ToString("N")
                    : instanceId,
                TemplateId = templateId,
                DisplayNameEn = displayNameEn,
                DisplayNameRu = displayNameRu,
                CardType = (CardType)cardType,
                Rarity = (CardRarity)rarity,
                Flags = (CardFlags)flags,
                ProvidedElement = (ElementId)providedElement,
                PrimaryAffinity = (ElementId)primaryAffinity,
                ElementSockets = elementSockets,
                ActiveComboId = activeComboId,
                ActiveComboNameEn = activeComboNameEn,
                ActiveComboNameRu = activeComboNameRu,
                ActiveComboTint = new Color(comboTintR, comboTintG, comboTintB, comboTintA),
                BasePower = basePower,
                CostCards = costCards > 0 ? costCards : 1,
                CraftLevel = craftLevel,
                CardsPerTurnBonus = cardsPerTurnBonus,
                CritChanceBonus = critChanceBonus,
                CardFloodBonus = cardFloodBonus,
                DescriptionEn = descriptionEn,
                DescriptionRu = descriptionRu
            };

            c.InfusedElements = new List<ElementId>();
            if (infusedElements != null)
            {
                for (int i = 0; i < infusedElements.Length; i++)
                    c.InfusedElements.Add((ElementId)infusedElements[i]);
            }

            c.CompoundElements = new List<ElementId>();
            if (compoundElements != null)
            {
                for (int i = 0; i < compoundElements.Length; i++)
                    c.CompoundElements.Add((ElementId)compoundElements[i]);
            }

            c.Affixes = new List<CardAffix>();
            if (affixes != null)
            {
                for (int i = 0; i < affixes.Length; i++)
                {
                    if (affixes[i] != null)
                        c.Affixes.Add(affixes[i].Clone());
                }
            }

            c.Mutations = new List<CardMutation>();
            if (mutations != null)
            {
                for (int i = 0; i < mutations.Length; i++)
                {
                    if (mutations[i] != null)
                        c.Mutations.Add(mutations[i].Clone());
                }
            }

            return c;
        }
    }
}
