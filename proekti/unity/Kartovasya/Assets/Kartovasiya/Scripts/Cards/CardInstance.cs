// Runtime card instance / Runtime-экземпляр карты
// Chaos Spellweaver (Картовасия)

using System;
using System.Collections.Generic;
using System.Text;
using UnityEngine;
using Kartovasiya.Core;
using Kartovasiya.Combos;

namespace Kartovasiya.Cards
{
    /// <summary>
    /// Unique runtime card: template base + procedural affixes/mutations + combo sockets.
    /// Уникальная runtime-карта: шаблон + аффиксы/мутации + слоты комбо.
    /// Almost every dropped card should feel unique (concept §6).
    /// </summary>
    [Serializable]
    public class CardInstance
    {
        // ── Identity ──────────────────────────────────────────────────

        public string InstanceId;
        public string TemplateId;
        public string DisplayNameEn;
        public string DisplayNameRu;
        public CardType CardType;
        public CardRarity Rarity;
        public CardFlags Flags;

        // ── Element / combo ───────────────────────────────────────────

        /// <summary>Element cards: primary element glyph. / Карта-элемент: основная стихия.</summary>
        public ElementId ProvidedElement;

        /// <summary>
        /// Hybrid element stack (Fire+Shadow = Dark Fire). Empty = pure ProvidedElement.
        /// Стек гибрида стихий; пусто = чистая ProvidedElement.
        /// </summary>
        public List<ElementId> CompoundElements = new List<ElementId>(3);

        /// <summary>Spell primary affinity (art / default). / Основная стихия заклинания.</summary>
        public ElementId PrimaryAffinity;

        /// <summary>Max sockets for drag-fusion. / Макс. слотов для drag-fusion.</summary>
        public int ElementSockets;

        /// <summary>Elements currently fused into this spell. / Влитые в заклинание стихии.</summary>
        public List<ElementId> InfusedElements = new List<ElementId>(GameConstants.MaxElementSocketsOnSpell);

        public const int MaxCompoundElements = 3;

        /// <summary>Last successful combo id (if any). / Id последнего комбо.</summary>
        public string ActiveComboId;

        public string ActiveComboNameEn;
        public string ActiveComboNameRu;
        public Color ActiveComboTint = Color.white;

        // ── Stats ─────────────────────────────────────────────────────

        public int BasePower;
        public int CostCards = 1;

        /// <summary>Craft upgrade level (0 = none). / Уровень крафта.</summary>
        public int CraftLevel;

        // ── Effect card passives ──────────────────────────────────────

        public int CardsPerTurnBonus;
        public float CritChanceBonus;
        public int CardFloodBonus;

        // ── Procedural layers ─────────────────────────────────────────

        public List<CardAffix> Affixes = new List<CardAffix>(GameConstants.MaxAffixesPerCard);
        public List<CardMutation> Mutations = new List<CardMutation>(GameConstants.MaxMutationsPerCard);

        // ── Presentation (optional overrides) ─────────────────────────

        public string DescriptionEn;
        public string DescriptionRu;

        // ── Factory ───────────────────────────────────────────────────

        public static CardInstance FromTemplate(CardSO template)
        {
            if (template == null)
                throw new ArgumentNullException(nameof(template));

            return new CardInstance
            {
                InstanceId = Guid.NewGuid().ToString("N"),
                TemplateId = template.TemplateId,
                DisplayNameEn = template.DisplayNameEn,
                DisplayNameRu = template.DisplayNameRu,
                CardType = template.CardType,
                Rarity = template.BaseRarity,
                Flags = CardFlags.None,
                ProvidedElement = template.ProvidedElement,
                PrimaryAffinity = template.PrimaryAffinity,
                ElementSockets = template.CardType == CardType.Spell
                    ? Mathf.Clamp(template.ElementSockets, 0, GameConstants.MaxElementSocketsOnSpell)
                    : 0,
                BasePower = template.BasePower,
                CostCards = template.BaseCostCards,
                CardsPerTurnBonus = template.CardsPerTurnBonus,
                CritChanceBonus = template.CritChanceBonus,
                CardFloodBonus = template.CardFloodBonus,
                DescriptionEn = template.DescriptionEn,
                DescriptionRu = template.DescriptionRu
            };
        }

        /// <summary>
        /// Fully procedural spell/element/effect without a SO template.
        /// Полностью procedural карта без SO-шаблона.
        /// </summary>
        public static CardInstance CreateProcedural(
            CardType type,
            CardRarity rarity,
            ElementId elementOrAffinity,
            int basePower,
            string nameEn,
            string nameRu)
        {
            var card = new CardInstance
            {
                InstanceId = Guid.NewGuid().ToString("N"),
                TemplateId = "proc_" + type.ToString().ToLowerInvariant(),
                DisplayNameEn = nameEn,
                DisplayNameRu = nameRu,
                CardType = type,
                Rarity = rarity,
                BasePower = basePower,
                CostCards = 1,
                ElementSockets = type == CardType.Spell ? 2 : 0,
                ProvidedElement = type == CardType.Element ? elementOrAffinity : ElementId.None,
                PrimaryAffinity = type != CardType.Element ? elementOrAffinity : ElementId.None
            };
            return card;
        }

        // ── Derived stats ─────────────────────────────────────────────

        /// <summary>
        /// Final power after affixes, mutations, combo bonus and craft.
        /// Итоговая сила: аффиксы + мутации + комбо + крафт.
        /// </summary>
        public int GetFinalPower(int comboPowerBonus = 0, float comboPowerMultiplier = 1f)
        {
            float power = BasePower + comboPowerBonus + CraftLevel * 2;

            for (int i = 0; i < Affixes.Count; i++)
            {
                var a = Affixes[i];
                if (a == null) continue;
                power += a.PowerFlat;
                power *= 1f + a.PowerPercent;
            }

            for (int i = 0; i < Mutations.Count; i++)
            {
                var m = Mutations[i];
                if (m == null) continue;
                power += m.PowerFlat;
                power *= 1f + m.PowerPercent;
            }

            power *= comboPowerMultiplier;

            if (HasFlag(CardFlags.Corrupted))
                power *= BalanceConfig.CorruptionPowerBonus;

            return Mathf.Max(0, Mathf.RoundToInt(power));
        }

        public int GetTotalCardsPerTurnBonus()
        {
            int sum = CardsPerTurnBonus;
            for (int i = 0; i < Affixes.Count; i++)
            {
                if (Affixes[i] != null)
                    sum += Affixes[i].CardsPerTurnBonus;
            }

            return sum;
        }

        public float GetTotalCritChanceBonus()
        {
            float sum = CritChanceBonus;
            for (int i = 0; i < Affixes.Count; i++)
            {
                if (Affixes[i] != null)
                    sum += Affixes[i].CritChanceBonus;
            }

            return sum;
        }

        public int GetTotalCardFloodBonus()
        {
            int sum = CardFloodBonus;
            for (int i = 0; i < Affixes.Count; i++)
            {
                if (Affixes[i] != null)
                    sum += Affixes[i].CardFloodBonus;
            }

            return sum;
        }

        // ── Element grant / compound ──────────────────────────────────

        /// <summary>
        /// All elements this Element card injects into a spell when fused.
        /// Все стихии, которые карта-элемент вливает в заклинание.
        /// </summary>
        public List<ElementId> GetGrantedElements()
        {
            var list = new List<ElementId>(MaxCompoundElements);
            if (CardType != CardType.Element)
                return list;

            if (CompoundElements != null && CompoundElements.Count > 0)
            {
                for (int i = 0; i < CompoundElements.Count; i++)
                {
                    if (ElementUtil.IsValid(CompoundElements[i]))
                        list.Add(CompoundElements[i]);
                }

                if (list.Count > 0)
                    return list;
            }

            if (ElementUtil.IsValid(ProvidedElement))
                list.Add(ProvidedElement);
            return list;
        }

        public bool CanAcceptElementMix()
        {
            if (CardType != CardType.Element)
                return false;
            if (HasFlag(CardFlags.Exhausted))
                return false;
            return GetGrantedElements().Count < MaxCompoundElements;
        }

        // ── Combo sockets (drag & drop) ───────────────────────────────

        public bool CanAcceptElement()
        {
            if (CardType != CardType.Spell)
                return false;
            if (HasFlag(CardFlags.Exhausted))
                return false;
            return InfusedElements.Count < ElementSockets;
        }

        /// <summary>
        /// How many free spell sockets remain.
        /// </summary>
        public int FreeSockets()
        {
            if (CardType != CardType.Spell) return 0;
            return Mathf.Max(0, ElementSockets - InfusedElements.Count);
        }

        /// <summary>
        /// Infuse an element into this spell (does not consume combo resolve).
        /// Влить стихию в заклинание (без резолва комбо).
        /// </summary>
        public bool TryInfuseElement(ElementId element)
        {
            if (!CanAcceptElement() || !ElementUtil.IsValid(element))
                return false;

            InfusedElements.Add(element);
            return true;
        }

        /// <summary>
        /// Infuse every granted part of an element card (hybrid injects all parts).
        /// Влить все части гибридной стихии (сколько влезает в слоты).
        /// </summary>
        public int TryInfuseGranted(IReadOnlyList<ElementId> granted)
        {
            if (granted == null || granted.Count == 0)
                return 0;
            int n = 0;
            for (int i = 0; i < granted.Count; i++)
            {
                if (!CanAcceptElement())
                    break;
                if (TryInfuseElement(granted[i]))
                    n++;
            }

            return n;
        }

        /// <summary>
        /// Elements used for combo matching: base affinity + infused sockets.
        /// Базовая стихия заклинания + влитые слоты (иначе Fire+Water никогда не матчится).
        /// </summary>
        public List<ElementId> BuildComboIngredientList()
        {
            var list = new List<ElementId>(ElementSockets + 1);
            if (CardType == CardType.Spell && ElementUtil.IsValid(PrimaryAffinity))
                list.Add(PrimaryAffinity);
            if (InfusedElements != null)
            {
                for (int i = 0; i < InfusedElements.Count; i++)
                {
                    if (ElementUtil.IsValid(InfusedElements[i]))
                        list.Add(InfusedElements[i]);
                }
            }

            return list;
        }

        /// <summary>
        /// After infusion, try to match a combo and apply presentation + flags.
        /// После вливания — попытка сматчить комбо и применить презентацию.
        /// </summary>
        public ComboResolveResult TryResolveCombo(ComboDatabaseSO database, System.Random rng = null)
        {
            if (database == null)
                return ComboResolveResult.Fail();

            var ingredients = BuildComboIngredientList();
            // Need at least affinity + 1 infusion (or 2 infusions if no affinity)
            if (ingredients.Count < 2)
                return ComboResolveResult.Fail();

            var result = database.Resolve(ingredients, rng);
            if (!result.Success)
                return result;

            if (result.Recipe != null)
            {
                ActiveComboId = result.Recipe.Id;
                ActiveComboNameEn = result.Recipe.ResultNameEn;
                ActiveComboNameRu = result.Recipe.ResultNameRu;
                ActiveComboTint = result.Recipe.ResultTint;
            }
            else
            {
                // Generic raw mix — still rename slightly so player sees progress
                ActiveComboId = "raw_mix";
                ActiveComboNameEn = "Chaos Mix";
                ActiveComboNameRu = "Смесь хаоса";
                ActiveComboTint = TarotPaletteSafeGold();
            }

            if (result.RiskTriggered)
                AddFlag(CardFlags.Corrupted);

            return result;
        }

        private static Color TarotPaletteSafeGold()
        {
            // Avoid UI dependency cycle issues — soft gold
            return new Color(0.95f, 0.78f, 0.3f, 1f);
        }

        public void ClearInfusions()
        {
            InfusedElements.Clear();
            ActiveComboId = null;
            ActiveComboNameEn = null;
            ActiveComboNameRu = null;
            ActiveComboTint = Color.white;
        }

        // ── Flags ─────────────────────────────────────────────────────

        public bool HasFlag(CardFlags flag) => (Flags & flag) != 0;

        public void AddFlag(CardFlags flag) => Flags |= flag;

        public void RemoveFlag(CardFlags flag) => Flags &= ~flag;

        // ── Display ───────────────────────────────────────────────────

        public string GetDisplayName(bool russian = true)
        {
            if (!string.IsNullOrEmpty(ActiveComboId))
            {
                string combo = russian ? ActiveComboNameRu : ActiveComboNameEn;
                if (!string.IsNullOrEmpty(combo))
                    return combo;
            }

            return russian ? DisplayNameRu : DisplayNameEn;
        }

        public string BuildTooltip(bool russian = true)
        {
            var sb = new StringBuilder(160);
            sb.AppendLine(GetDisplayName(russian));
            sb.AppendLine($"[{CardType}] {Rarity}");
            sb.AppendLine(russian
                ? $"Сила: {GetFinalPower()}"
                : $"Power: {GetFinalPower()}");

            if (CardType == CardType.Spell && ElementUtil.IsValid(PrimaryAffinity))
                sb.AppendLine(russian
                    ? $"База: {PrimaryAffinity}"
                    : $"Base: {PrimaryAffinity}");

            if (InfusedElements.Count > 0)
            {
                sb.Append(russian ? "Влито: " : "Infused: ");
                for (int i = 0; i < InfusedElements.Count; i++)
                {
                    if (i > 0) sb.Append(" + ");
                    sb.Append(InfusedElements[i]);
                }

                sb.AppendLine();
            }

            if (!string.IsNullOrEmpty(DescriptionRu) || !string.IsNullOrEmpty(DescriptionEn))
                sb.AppendLine(russian ? DescriptionRu : DescriptionEn);

            if (HasFlag(CardFlags.Wild))
                sb.AppendLine(russian ? "★ Wild Card" : "★ Wild Card");
            if (HasFlag(CardFlags.Corrupted))
                sb.AppendLine(russian ? "☠ Corruption" : "☠ Corruption");

            return sb.ToString().Trim();
        }

        public CardInstance Clone()
        {
            var copy = (CardInstance)MemberwiseClone();
            copy.InstanceId = Guid.NewGuid().ToString("N");
            copy.InfusedElements = new List<ElementId>(InfusedElements);
            copy.CompoundElements = new List<ElementId>(CompoundElements ?? new List<ElementId>());
            copy.Affixes = new List<CardAffix>(Affixes.Count);
            for (int i = 0; i < Affixes.Count; i++)
                copy.Affixes.Add(Affixes[i]?.Clone());
            copy.Mutations = new List<CardMutation>(Mutations.Count);
            for (int i = 0; i < Mutations.Count; i++)
                copy.Mutations.Add(Mutations[i]?.Clone());
            return copy;
        }
    }
}
