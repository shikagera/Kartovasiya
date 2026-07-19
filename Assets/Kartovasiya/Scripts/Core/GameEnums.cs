// Core game enumerations / Базовые перечисления игры
// Chaos Spellweaver (Картовасия) — full concept alignment

namespace Kartovasiya.Core
{
    /// <summary>
    /// 16 elemental affinities used for cards, combos and crafting.
    /// 16 стихий: основа комбинаций, карт и крафта.
    /// Basic (1–8) drop often; Extended (9–16) are rarer unlocks.
    /// Базовые (1–8) — частые; расширенные (9–16) — редкие/открываемые.
    /// </summary>
    public enum ElementId
    {
        None = 0,

        // ── Basic / Базовые ───────────────────────────────────────────
        Fire = 1,       // Огонь
        Water = 2,      // Вода
        Earth = 3,      // Земля
        Air = 4,        // Воздух
        Lightning = 5,  // Молния
        Ice = 6,        // Лёд
        Nature = 7,     // Природа
        Shadow = 8,     // Тень

        // ── Extended (rare) / Расширенные (редкие) ────────────────────
        Metal = 9,      // Металл
        Light = 10,     // Свет
        Poison = 11,    // Яд / Кислота (Poison/Acid)
        Aether = 12,    // Эфир / Пустота (Aether/Void)
        Blood = 13,     // Кровь / Душа (Blood/Soul)
        Storm = 14,     // Буря
        Crystal = 15,   // Кристалл
        Arcane = 16     // Аркана
    }

    /// <summary>
    /// Progression tier of an element (drop weight / unlocks).
    /// Уровень стихии: влияет на вес дропа и прогрессию открытий.
    /// </summary>
    public enum ElementTier
    {
        Basic = 0,      // 8 базовых
        Extended = 1    // 8 расширенных (редкие)
    }

    /// <summary>
    /// Card rarity tier for procedural generation and drop tables.
    /// Редкость карты: влияет на генерацию и лут.
    /// </summary>
    public enum CardRarity
    {
        Common = 0,     // Обычная
        Uncommon = 1,   // Необычная
        Rare = 2,       // Редкая
        Epic = 3,       // Эпическая
        Legendary = 4,  // Легендарная
        Mythic = 5      // Мифическая (высокий мутационный потолок)
    }

    /// <summary>
    /// Concept card roles:
    /// Spell — main weapon; Element — drag modifier; Effect — passives (limit, crits, Card Flood).
    /// Типы карт по концепту: заклинания / элементы / эффекты (пассивки).
    /// </summary>
    public enum CardType
    {
        Spell = 0,      // Заклинание — основное оружие
        Element = 1,    // Элемент — модификатор (drag source)
        Effect = 2,     // Эффект — пассивка (лимит карт, криты, Card Flood…)
        Material = 3    // Материал крафта (лут с боссов)
    }

    /// <summary>
    /// How complex a combo recipe is (UI + risk scaling).
    /// Сложность комбо: UI и масштабирование риска.
    /// </summary>
    public enum ComboComplexity
    {
        Simple = 0,     // 2 элемента, стабильно
        Complex = 1,    // 3+ элементов / сильный эффект
        Risky = 2       // шанс негативного эффекта
    }

    /// <summary>
    /// Combat turn phases (player-centric arcade loop).
    /// Фазы хода (аркадный цикл от лица игрока). Нет маны — только лимит карт.
    /// </summary>
    public enum TurnPhase
    {
        None = 0,
        StartOfTurn,    // Начало хода (триггеры, DoT, global events)
        DropCards,      // Авто-дроп новых карт в руку
        PlayerAction,   // Игрок играет / комбинирует (лимит карт)
        Resolve,        // Разрешение очереди эффектов
        EnemyAction,    // Действия врагов
        EndOfTurn,      // Конец хода (мутации, corruption ticks)
        Victory,        // Победа в ноде
        Defeat,         // Поражение
        Exited          // Игрок вышел (бесконечная аркада)
    }

    /// <summary>
    /// Screen orientation helper for UI layout switching.
    /// Ориентация экрана: portrait / landscape UI.
    /// </summary>
    public enum ScreenLayoutMode
    {
        Portrait = 0,
        Landscape = 1
    }

    /// <summary>
    /// Runtime flags for procedural / chaos systems.
    /// Флаги хаоса: Wild, Corruption, combat mutations.
    /// </summary>
    [System.Flags]
    public enum CardFlags
    {
        None = 0,
        Wild = 1 << 0,          // Wild Card — подстраивается под комбо
        Corrupted = 1 << 1,     // Двойственный / рискованный эффект
        Mutated = 1 << 2,       // Мутация в бою
        Crafted = 1 << 3,       // Улучшена крафтом
        Exhausted = 1 << 4      // Сыграна / недоступна до конца боя (опционально)
    }
}
