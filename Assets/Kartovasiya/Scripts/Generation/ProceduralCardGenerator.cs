// Procedural card factory / Процедурная фабрика карт
// Chaos Spellweaver (Картовасия)

using System;
using System.Collections.Generic;
using UnityEngine;
using Kartovasiya.Cards;
using Kartovasiya.Core;

namespace Kartovasiya.Generation
{
    /// <summary>
    /// High-variance offline generator: base card + affixes + rare wild/corruption.
    /// Высокий procedural рандом: база + аффиксы + wild/corruption.
    /// </summary>
    public class ProceduralCardGenerator
    {
        private readonly System.Random _rng;
        private readonly int _runDepth;

        private static readonly ElementId[] BasicElements =
        {
            ElementId.Fire, ElementId.Water, ElementId.Earth, ElementId.Air,
            ElementId.Lightning, ElementId.Ice, ElementId.Nature, ElementId.Shadow
        };

        private static readonly ElementId[] ExtendedElements =
        {
            ElementId.Metal, ElementId.Light, ElementId.Poison, ElementId.Aether,
            ElementId.Blood, ElementId.Storm, ElementId.Crystal, ElementId.Arcane
        };

        private static readonly string[] SpellNamesEn =
        {
            "Sigil", "Bolt", "Wave", "Shard", "Hex", "Rift", "Brand", "Pulse"
        };

        private static readonly string[] SpellNamesRu =
        {
            "Печать", "Болт", "Волна", "Осколок", "Проклятие", "Разлом", "Клеймо", "Пульс"
        };

        public ProceduralCardGenerator(int? seed = null, int runDepth = 1)
        {
            _rng = seed.HasValue ? new System.Random(seed.Value) : new System.Random();
            _runDepth = Mathf.Max(1, runDepth);
        }

        /// <summary>Drop one random card for combat hand. / Одна карта для авто-дропа.</summary>
        public CardInstance GenerateDrop()
        {
            float roll = (float)_rng.NextDouble();
            // Weight: Element heavy (fusion fun), then Spell, then rare Effect
            if (roll < 0.48f)
                return GenerateElementCard();
            if (roll < 0.88f)
                return GenerateSpellCard();
            return GenerateEffectCard();
        }

        public List<CardInstance> GenerateDrops(int count)
        {
            var list = new List<CardInstance>(count);
            for (int i = 0; i < count; i++)
                list.Add(GenerateDrop());
            return list;
        }

        /// <summary>Opening hand for a fight. / Стартовая рука боя.</summary>
        public List<CardInstance> GenerateOpeningHand(int size)
        {
            var list = new List<CardInstance>(size);
            // Tutorial: Fire spell + Water element → Steam (base affinity counts!)
            // Огонь-заклинание + Вода → Пар
            var fireSpell = CardInstance.CreateProcedural(
                CardType.Spell, CardRarity.Common, ElementId.Fire, 6,
                "Ember Bolt", "Уголь-болт");
            fireSpell.ElementSockets = 2;
            fireSpell.DescriptionRu = "База: Огонь. Перетащи Воду → Пар (+урон). Землю → Горящий булыжник.";
            fireSpell.DescriptionEn = "Base: Fire. Drop Water → Steam. Earth → Burning Boulder.";
            list.Add(fireSpell);

            var water = CardInstance.CreateProcedural(CardType.Element, CardRarity.Common, ElementId.Water, 0, "Water", "Вода");
            water.DescriptionRu = "На стихию = смесь. На Огонь-закл. → Пар.";
            water.DescriptionEn = "Onto element = mix. Onto Fire spell → Steam.";
            list.Add(water);

            var earth = CardInstance.CreateProcedural(CardType.Element, CardRarity.Common, ElementId.Earth, 0, "Earth", "Земля");
            earth.DescriptionRu = "На Огонь-стихию / закл. → Горящий камень.";
            earth.DescriptionEn = "Onto Fire element/spell → Burning stone.";
            list.Add(earth);

            var shadow = CardInstance.CreateProcedural(CardType.Element, CardRarity.Common, ElementId.Shadow, 0, "Shadow", "Тьма");
            shadow.DescriptionRu = "На Огонь = Тёмный огонь. На Молнию = Тёмная молния.";
            shadow.DescriptionEn = "Onto Fire = Dark Fire. Onto Lightning = Dark Lightning.";
            list.Add(shadow);

            var lightningSpell = CardInstance.CreateProcedural(
                CardType.Spell, CardRarity.Common, ElementId.Lightning, 6,
                "Spark Lance", "Искровой клин");
            lightningSpell.ElementSockets = 3;
            lightningSpell.DescriptionRu = "База: Молния. Вода → Громовой хлопок. Огонь+Вода → Наэлектризованный пар.";
            lightningSpell.DescriptionEn = "Base: Lightning. Water → Thunderclap. Fire+Water → Electrified Steam.";
            list.Add(lightningSpell);

            while (list.Count < size)
                list.Add(GenerateDrop());
            return list;
        }

        public CardInstance GenerateSpellCard()
        {
            var element = RollElement();
            var rarity = RollRarity();
            int power = 4 + (int)rarity * 2 + _rng.Next(0, 3) + _runDepth / 3;
            int sockets = rarity >= CardRarity.Rare ? 3 : 2;

            string en = $"{element} {SpellNamesEn[_rng.Next(SpellNamesEn.Length)]}";
            string ru = $"{RuElement(element)}-{SpellNamesRu[_rng.Next(SpellNamesRu.Length)]}";

            var card = CardInstance.CreateProcedural(CardType.Spell, rarity, element, power, en, ru);
            card.ElementSockets = sockets;
            card.DescriptionRu = $"База: {RuElement(element)}. Вливай стихии в слоты — смеси дают больше урона.";
            card.DescriptionEn = $"Base: {element}. Infuse elements into sockets — mixes deal more damage.";
            ApplyAffixes(card, rarity);
            MaybeWildOrCorrupt(card, rarity);
            return card;
        }

        public CardInstance GenerateElementCard()
        {
            var element = RollElement();
            var rarity = ElementUtil.IsExtended(element) ? CardRarity.Uncommon : CardRarity.Common;
            if (_rng.NextDouble() < 0.1)
                rarity = CardRarity.Rare;

            string en = element.ToString();
            string ru = RuElement(element);
            var card = CardInstance.CreateProcedural(CardType.Element, rarity, element, 0, en, ru);
            card.DescriptionRu = $"Стихия {ru}. На другую стихию = гибрид. На заклинание = влить.";
            card.DescriptionEn = $"Element {en}. Onto element = hybrid. Onto spell = infuse.";
            MaybeWildOrCorrupt(card, rarity);
            return card;
        }

        public CardInstance GenerateEffectCard()
        {
            var rarity = RollRarity();
            var card = CardInstance.CreateProcedural(
                CardType.Effect, rarity, ElementId.Arcane,
                0, "Chaos Charm", "Оберег хаоса");

            // Passives matching concept: limit, crits, Card Flood
            int kind = _rng.Next(0, 3);
            switch (kind)
            {
                case 0:
                    card.CardsPerTurnBonus = 1;
                    card.DisplayNameEn = "Extra Weave";
                    card.DisplayNameRu = "Лишний узел";
                    card.DescriptionRu = "+1 карта/ход. Тапни для просмотра. Перетащи ВНИЗ на зелёную зону.";
                    card.DescriptionEn = "+1 play/turn. Tap to inspect. Drag DOWN onto green zone.";
                    break;
                case 1:
                    card.CritChanceBonus = 0.1f + (int)rarity * 0.03f;
                    card.DisplayNameEn = "Crit Mantra";
                    card.DisplayNameRu = "Мантра крита";
                    card.DescriptionRu = "Крит, пока в руке. Тапни · перетащи ВНИЗ чтобы закрепить.";
                    card.DescriptionEn = "Crit while in hand. Tap · drag DOWN to lock in.";
                    break;
                default:
                    card.CardFloodBonus = 1;
                    card.DisplayNameEn = "Card Flood";
                    card.DisplayNameRu = "Поток карт";
                    card.DescriptionRu = "+1 авто-дроп. Тапни · ВНИЗ на зелёную зону.";
                    card.DescriptionEn = "+1 auto-drop. Tap · DOWN onto green zone.";
                    break;
            }

            ApplyAffixes(card, rarity);
            return card;
        }

        private void ApplyAffixes(CardInstance card, CardRarity rarity)
        {
            int max = Mathf.Min(GameConstants.MaxAffixesPerCard, (int)rarity);
            int count = _rng.Next(0, max + 1);
            for (int i = 0; i < count; i++)
            {
                card.Affixes.Add(new CardAffix
                {
                    Id = "aff_" + _rng.Next(10000, 99999),
                    NameEn = "Chaos Edge",
                    NameRu = "Грань хаоса",
                    PowerFlat = _rng.Next(0, 3),
                    PowerPercent = _rng.NextDouble() < 0.3 ? 0.05f : 0f
                });
            }
        }

        private void MaybeWildOrCorrupt(CardInstance card, CardRarity rarity)
        {
            float wildChance = 0.06f + (int)rarity * 0.02f;
            if (_rng.NextDouble() < wildChance)
            {
                card.AddFlag(CardFlags.Wild);
                card.DisplayNameEn = "Wild " + card.DisplayNameEn;
                card.DisplayNameRu = "Дикий " + card.DisplayNameRu;
            }

            float corrChance = 0.04f + _runDepth * 0.005f;
            if (_rng.NextDouble() < corrChance)
            {
                card.AddFlag(CardFlags.Corrupted);
                card.Mutations.Add(new CardMutation
                {
                    Id = "mut_corr",
                    NameEn = "Corruption",
                    NameRu = "Порча",
                    PowerPercent = 0.2f,
                    IsCorrupted = true,
                    DownsideKey = "self_backlash"
                });
            }
        }

        private ElementId RollElement()
        {
            bool extended = _rng.NextDouble() < GameConstants.ExtendedElementDropWeight;
            // Unlock extended gradually with depth
            if (extended && _runDepth < 3)
                extended = _rng.NextDouble() < 0.35f;

            var pool = extended ? ExtendedElements : BasicElements;
            return pool[_rng.Next(pool.Length)];
        }

        private CardRarity RollRarity()
        {
            float r = (float)_rng.NextDouble();
            float depthBias = Mathf.Min(0.25f, _runDepth * 0.01f);
            if (r < 0.55f - depthBias) return CardRarity.Common;
            if (r < 0.82f) return CardRarity.Uncommon;
            if (r < 0.94f) return CardRarity.Rare;
            if (r < 0.985f) return CardRarity.Epic;
            return CardRarity.Legendary;
        }

        private static string RuElement(ElementId id)
        {
            return id switch
            {
                ElementId.Fire => "Огонь",
                ElementId.Water => "Вода",
                ElementId.Earth => "Земля",
                ElementId.Air => "Воздух",
                ElementId.Lightning => "Молния",
                ElementId.Ice => "Лёд",
                ElementId.Nature => "Природа",
                ElementId.Shadow => "Тень",
                ElementId.Metal => "Металл",
                ElementId.Light => "Свет",
                ElementId.Poison => "Яд",
                ElementId.Aether => "Эфир",
                ElementId.Blood => "Кровь",
                ElementId.Storm => "Буря",
                ElementId.Crystal => "Кристалл",
                ElementId.Arcane => "Аркана",
                _ => id.ToString()
            };
        }
    }
}
