// Card template (ScriptableObject) / Шаблон карты
// Chaos Spellweaver (Картовасия)

using UnityEngine;
using Kartovasiya.Core;

namespace Kartovasiya.Cards
{
    /// <summary>
    /// Design-time base for a card archetype.
    /// Most runtime cards are procedural clones of a thin template (or fully generated).
    /// Design-time архетип. Большинство runtime-карт — procedural поверх шаблона.
    /// Types: Spell | Element | Effect | Material (concept §5).
    /// </summary>
    [CreateAssetMenu(
        fileName = "Card_",
        menuName = "Kartovasiya/Cards/Card Template",
        order = 20)]
    public class CardSO : ScriptableObject
    {
        [Header("Identity / Идентификация")]
        [SerializeField] private string _templateId = "card_base";
        [SerializeField] private string _displayNameEn = "Card";
        [SerializeField] private string _displayNameRu = "Карта";
        [SerializeField] private CardType _cardType = CardType.Spell;
        [SerializeField] private CardRarity _baseRarity = CardRarity.Common;

        [Header("Element / Стихия")]
        [Tooltip("For Element cards: which element this grants on drag. / Для Element: какую стихию даёт.")]
        [SerializeField] private ElementId _providedElement = ElementId.None;

        [Tooltip("For Spells: optional primary affinity for art / weights. / Для Spell: основная стихия.")]
        [SerializeField] private ElementId _primaryAffinity = ElementId.None;

        [Header("Spell sockets / Слоты заклинания")]
        [Tooltip("How many element cards can be fused into this spell. / Сколько элементов можно влить.")]
        [Range(0, 3)]
        [SerializeField] private int _elementSockets = 2;

        [Header("Stats / Статы")]
        [SerializeField] private int _basePower = 5;
        [SerializeField] private int _baseCostCards = 1;

        [Header("Effect card / Карта-эффект (пассивка)")]
        [SerializeField] private int _cardsPerTurnBonus;
        [SerializeField] private float _critChanceBonus;
        [SerializeField] private int _cardFloodBonus;

        [Header("Presentation / Таро 2.5D")]
        [SerializeField] private Sprite _art;
        [SerializeField] private Sprite _frame;
        [SerializeField] private Color _frameTint = Color.white;

        [Header("Lore / Лор")]
        [TextArea(2, 5)]
        [SerializeField] private string _descriptionEn;
        [TextArea(2, 5)]
        [SerializeField] private string _descriptionRu;

        [Header("Proc gen weights / Веса генерации")]
        [Tooltip("Relative weight in drop tables. / Вес в таблицах дропа.")]
        [SerializeField] private float _dropWeight = 1f;

        [Tooltip("Allow procedural affixes/mutations on instances. / Разрешить аффиксы/мутации.")]
        [SerializeField] private bool _allowProceduralMods = true;

        // ── Public API ────────────────────────────────────────────────

        public string TemplateId => _templateId;
        public string DisplayNameEn => _displayNameEn;
        public string DisplayNameRu => _displayNameRu;
        public CardType CardType => _cardType;
        public CardRarity BaseRarity => _baseRarity;
        public ElementId ProvidedElement => _providedElement;
        public ElementId PrimaryAffinity => _primaryAffinity;
        public int ElementSockets => _elementSockets;
        public int BasePower => _basePower;
        public int BaseCostCards => Mathf.Max(1, _baseCostCards);
        public int CardsPerTurnBonus => _cardsPerTurnBonus;
        public float CritChanceBonus => _critChanceBonus;
        public int CardFloodBonus => _cardFloodBonus;
        public Sprite Art => _art;
        public Sprite Frame => _frame;
        public Color FrameTint => _frameTint;
        public string DescriptionEn => _descriptionEn;
        public string DescriptionRu => _descriptionRu;
        public float DropWeight => _dropWeight;
        public bool AllowProceduralMods => _allowProceduralMods;

        public string GetDisplayName(bool russian = true) =>
            russian ? _displayNameRu : _displayNameEn;

        public string GetDescription(bool russian = true) =>
            russian ? _descriptionRu : _descriptionEn;

        /// <summary>
        /// Create a clean runtime instance from this template (no affixes yet).
        /// Чистый runtime-инстанс без аффиксов (генератор добавит сам).
        /// </summary>
        public CardInstance CreateInstance()
        {
            return CardInstance.FromTemplate(this);
        }
    }
}
