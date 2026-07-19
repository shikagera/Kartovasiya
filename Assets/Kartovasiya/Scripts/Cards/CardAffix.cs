// Runtime affix / mutation models / Runtime-аффиксы и мутации
// Chaos Spellweaver (Картовасия)

using System;
using UnityEngine;
using Kartovasiya.Core;

namespace Kartovasiya.Cards
{
    /// <summary>
    /// Procedural modifier rolled onto a card (stat or keyword).
    /// Процедурный модификатор на карте (стат или ключевое слово).
    /// </summary>
    [Serializable]
    public class CardAffix
    {
        public string Id;
        public string NameEn;
        public string NameRu;

        [Tooltip("Flat power added. / Плоский бонус силы.")]
        public int PowerFlat;

        [Tooltip("Additive percent power (0.1 = +10%). / Процентный бонус силы.")]
        public float PowerPercent;

        [Tooltip("Bonus to cards-per-turn while this Effect card is active. / Бонус лимита карт (Effect).")]
        public int CardsPerTurnBonus;

        [Tooltip("Crit chance bonus 0..1. / Бонус шанса крита.")]
        public float CritChanceBonus;

        [Tooltip("Card Flood: extra cards dropped next turn. / Card Flood: доп. карты в дропе.")]
        public int CardFloodBonus;

        public string GetName(bool russian = true) =>
            russian ? (string.IsNullOrEmpty(NameRu) ? NameEn : NameRu) : NameEn;

        public CardAffix Clone()
        {
            return new CardAffix
            {
                Id = Id,
                NameEn = NameEn,
                NameRu = NameRu,
                PowerFlat = PowerFlat,
                PowerPercent = PowerPercent,
                CardsPerTurnBonus = CardsPerTurnBonus,
                CritChanceBonus = CritChanceBonus,
                CardFloodBonus = CardFloodBonus
            };
        }
    }

    /// <summary>
    /// In-combat mutation (can be dual / corrupted).
    /// Мутация в бою (может быть двойственной / corrupted).
    /// </summary>
    [Serializable]
    public class CardMutation
    {
        public string Id;
        public string NameEn;
        public string NameRu;
        public string DescriptionEn;
        public string DescriptionRu;

        public int PowerFlat;
        public float PowerPercent;

        [Tooltip("If true, mutation has a downside. / Есть негативная сторона.")]
        public bool IsCorrupted;

        public string DownsideKey;

        public string GetName(bool russian = true) =>
            russian ? (string.IsNullOrEmpty(NameRu) ? NameEn : NameRu) : NameEn;

        public CardMutation Clone()
        {
            return new CardMutation
            {
                Id = Id,
                NameEn = NameEn,
                NameRu = NameRu,
                DescriptionEn = DescriptionEn,
                DescriptionRu = DescriptionRu,
                PowerFlat = PowerFlat,
                PowerPercent = PowerPercent,
                IsCorrupted = IsCorrupted,
                DownsideKey = DownsideKey
            };
        }
    }
}
