// Drag-and-drop fusion: Element→Spell and Element→Element
// Слияние: элемент→заклинание и элемент→элемент (гибриды)
// Chaos Spellweaver (Картовасия)

using System;
using System.Collections.Generic;
using UnityEngine;
using Kartovasiya.Core;
using Kartovasiya.Combos;

namespace Kartovasiya.Cards
{
    /// <summary>
    /// Fusion service: element onto spell (sockets) or element onto element (hybrid).
    /// </summary>
    public sealed class CardFusionService
    {
        private readonly ComboDatabaseSO _combos;
        private readonly System.Random _rng;

        public CardFusionService(ComboDatabaseSO combos, int? seed = null)
        {
            _combos = combos;
            _rng = seed.HasValue ? new System.Random(seed.Value) : new System.Random();
        }

        public event Action<CardInstance, CardInstance, ComboResolveResult> Fused;
        public event Action<CardInstance, CardInstance, string> FusionFailed;
        public event Action<CardInstance, CardInstance, CardInstance> ElementsMixed;

        /// <summary>
        /// Fuse source (must be Element) into target Spell or Element.
        /// </summary>
        public bool TryFuse(CardInstance source, CardInstance target, out ComboResolveResult comboResult)
        {
            comboResult = ComboResolveResult.Fail();

            if (source == null || target == null)
            {
                FusionFailed?.Invoke(source, target, "null_card");
                return false;
            }

            if (source.CardType != CardType.Element)
            {
                FusionFailed?.Invoke(source, target, "source_not_element");
                return false;
            }

            if (target.CardType == CardType.Spell)
                return FuseIntoSpell(source, target, out comboResult);

            if (target.CardType == CardType.Element)
                return FuseIntoElement(source, target, out comboResult);

            FusionFailed?.Invoke(source, target, "bad_target");
            return false;
        }

        private bool FuseIntoSpell(CardInstance elementCard, CardInstance spellCard, out ComboResolveResult comboResult)
        {
            comboResult = ComboResolveResult.Fail();

            var granted = elementCard.GetGrantedElements();
            if (granted.Count == 0)
            {
                // Wild with no element
                ElementId element = elementCard.ProvidedElement;
                if (element == ElementId.None && elementCard.HasFlag(CardFlags.Wild))
                {
                    element = spellCard.PrimaryAffinity != ElementId.None
                        ? spellCard.PrimaryAffinity
                        : ElementId.Arcane;
                }

                if (!ElementUtil.IsValid(element))
                {
                    FusionFailed?.Invoke(elementCard, spellCard, "invalid_element");
                    return false;
                }

                granted = new List<ElementId> { element };
            }

            if (spellCard.FreeSockets() <= 0)
            {
                FusionFailed?.Invoke(elementCard, spellCard, "no_socket");
                return false;
            }

            int infused = spellCard.TryInfuseGranted(granted);
            if (infused <= 0)
            {
                FusionFailed?.Invoke(elementCard, spellCard, "no_socket");
                return false;
            }

            elementCard.AddFlag(CardFlags.Exhausted);

            if (_combos != null)
                comboResult = spellCard.TryResolveCombo(_combos, _rng);

            Fused?.Invoke(elementCard, spellCard, comboResult);
            return true;
        }

        private bool FuseIntoElement(CardInstance source, CardInstance target, out ComboResolveResult comboResult)
        {
            comboResult = ComboResolveResult.Fail();

            if (!target.CanAcceptElementMix())
            {
                FusionFailed?.Invoke(source, target, "element_full");
                return false;
            }

            var parts = new List<ElementId>(CardInstance.MaxCompoundElements);
            AppendUnique(parts, target.GetGrantedElements());
            AppendUnique(parts, source.GetGrantedElements());

            if (parts.Count < 2)
            {
                FusionFailed?.Invoke(source, target, "same_or_empty");
                return false;
            }

            if (parts.Count > CardInstance.MaxCompoundElements)
            {
                // Keep first MaxCompound, drop extras
                parts.RemoveRange(CardInstance.MaxCompoundElements, parts.Count - CardInstance.MaxCompoundElements);
            }

            // Apply hybrid presentation onto target
            target.CompoundElements = new List<ElementId>(parts);
            target.ProvidedElement = parts[0];

            ApplyMixName(target, parts);

            // Slight power flavor on hybrid element (shown as glyph score)
            target.BasePower = Mathf.Max(target.BasePower, parts.Count);

            source.AddFlag(CardFlags.Exhausted);

            // Pseudo "success" for combat power path when later on spell
            comboResult = ComboResolveResult.GenericSynergy(parts.Count);
            ElementsMixed?.Invoke(source, target, target);
            Fused?.Invoke(source, target, comboResult);
            return true;
        }

        private static void AppendUnique(List<ElementId> dst, List<ElementId> src)
        {
            if (src == null) return;
            for (int i = 0; i < src.Count; i++)
            {
                var e = src[i];
                if (!ElementUtil.IsValid(e)) continue;
                if (dst.Contains(e)) continue;
                dst.Add(e);
            }
        }

        private static void ApplyMixName(CardInstance target, List<ElementId> parts)
        {
            if (parts.Count == 2 && ElementMixCatalog.TryGet(parts[0], parts[1], out var mix))
            {
                target.DisplayNameEn = mix.NameEn;
                target.DisplayNameRu = mix.NameRu;
                target.DescriptionEn = mix.DescEn + " Drag onto a spell to infuse both.";
                target.DescriptionRu = mix.DescRu + " Перетащи на заклинание — вольются обе стихии.";
                target.ActiveComboTint = mix.Tint;
                return;
            }

            // 3-part or unlisted pair
            if (parts.Count >= 2 && ElementMixCatalog.TryGet(parts[0], parts[1], out var baseMix))
            {
                string extra = parts.Count > 2 ? " + " + ElementMixCatalog.Ru(parts[2]) : "";
                string extraEn = parts.Count > 2 ? " + " + parts[2] : "";
                target.DisplayNameRu = baseMix.NameRu + extra;
                target.DisplayNameEn = baseMix.NameEn + extraEn;
                target.DescriptionRu = baseMix.DescRu + " (гибрид ×" + parts.Count + "). На заклинание.";
                target.DescriptionEn = baseMix.DescEn + " (hybrid ×" + parts.Count + "). Onto spell.";
                target.ActiveComboTint = baseMix.Tint;
                return;
            }

            ElementMixCatalog.GetGenericName(parts[0], parts[1], out string en, out string ru);
            if (parts.Count > 2)
            {
                en += " + " + parts[2];
                ru += " + " + ElementMixCatalog.Ru(parts[2]);
            }

            target.DisplayNameEn = en;
            target.DisplayNameRu = ru;
            target.DescriptionRu = "Гибрид стихий. Перетащи на заклинание.";
            target.DescriptionEn = "Element hybrid. Drag onto a spell.";
        }
    }
}
