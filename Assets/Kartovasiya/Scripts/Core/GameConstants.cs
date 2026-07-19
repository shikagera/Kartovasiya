// Tunable global constants / Глобальные константы баланса
// Chaos Spellweaver (Картовасия)

namespace Kartovasiya.Core
{
    /// <summary>
    /// Central place for prototype balance knobs (concept defaults).
    /// Единая точка настройки баланса по концепту (позже — CombatConfigSO).
    /// </summary>
    public static class GameConstants
    {
        // ── Combat / Бой (NO mana — only card limit) ───────────────────

        /// <summary>Base cards playable per turn (concept: 5–6). / Базовый лимит карт за ход.</summary>
        public const int BaseCardsPerTurn = BalanceConfig.CardsPerTurn;

        /// <summary>Alternate starter for easier runs. / Альтернативный старт.</summary>
        public const int AlternateCardsPerTurn = 6;

        /// <summary>Soft cap for cards-per-turn bonuses. / Мягкий потолок лимита карт.</summary>
        public const int MaxCardsPerTurn = 12;

        /// <summary>Default hand size after auto-drop. / Размер руки после авто-дропа.</summary>
        public const int DefaultHandSize = BalanceConfig.OpeningHand;

        /// <summary>Hard cap of cards held in hand. / Жёсткий лимит карт в руке.</summary>
        public const int MaxHandSize = 10;

        /// <summary>How many new cards drop at turn start (fills toward hand size). / Сколько карт дропается.</summary>
        public const int CardsDroppedPerTurn = BalanceConfig.CardsDroppedPerTurn;

        // ── Crafting / Крафт ───────────────────────────────────────────

        /// <summary>Maximum craft success chance (concept: ≤70%). / Макс. шанс успеха крафта.</summary>
        public const float MaxCraftSuccessChance = BalanceConfig.CraftMaxChance;

        /// <summary>Base craft success without bonuses. / Базовый шанс крафта.</summary>
        public const float BaseCraftSuccessChance = BalanceConfig.CraftBaseChance;

        // ── Elements / Стихии ──────────────────────────────────────────

        public const int ElementCount = 16;
        public const int BasicElementCount = 8;
        public const int ExtendedElementCount = 8;

        /// <summary>Max element sockets on a single spell (drag targets). / Макс. слотов элементов на заклинании.</summary>
        public const int MaxElementSocketsOnSpell = 3;

        /// <summary>Relative drop weight for Extended elements vs Basic. / Относительный вес редких стихий.</summary>
        public const float ExtendedElementDropWeight = 0.22f;

        // ── Combos / Комбо ─────────────────────────────────────────────

        /// <summary>Default negative-effect chance for Risky combos. / Базовый шанс негатива у рискованных комбо.</summary>
        public const float DefaultRiskyComboFailChance = 0.25f;

        // ── Procedural generation / Генерация ──────────────────────────

        public const int MaxAffixesPerCard = 4;
        public const int MaxMutationsPerCard = 2;

        /// <summary>Global random event every N turns (prototype). / Глобальное событие каждые N ходов.</summary>
        public const int GlobalEventIntervalTurns = BalanceConfig.GlobalEventEveryTurns;

        /// <summary>Boss every N enemies (1-based wave). / Босс каждые N врагов.</summary>
        public const int BossEveryNEnemies = BalanceConfig.BossEveryN;

        /// <summary>Player starting HP. / Стартовое HP игрока.</summary>
        public const int PlayerStartHp = BalanceConfig.PlayerStartHp;

        // ── Product ────────────────────────────────────────────────────

        public const string ProductName = "Chaos Spellweaver";
        public const string ProductNameRu = "Картовасия";
    }

    /// <summary>
    /// Helpers tied to element identity (tier, validity).
    /// Хелперы стихий: тир, валидность.
    /// </summary>
    public static class ElementUtil
    {
        public static bool IsValid(ElementId id) =>
            id != ElementId.None && (int)id >= 1 && (int)id <= GameConstants.ElementCount;

        public static ElementTier GetTier(ElementId id)
        {
            if (!IsValid(id))
                return ElementTier.Basic;

            // 1..8 Basic, 9..16 Extended
            return (int)id <= GameConstants.BasicElementCount
                ? ElementTier.Basic
                : ElementTier.Extended;
        }

        public static bool IsBasic(ElementId id) => GetTier(id) == ElementTier.Basic;
        public static bool IsExtended(ElementId id) => GetTier(id) == ElementTier.Extended;
    }
}
