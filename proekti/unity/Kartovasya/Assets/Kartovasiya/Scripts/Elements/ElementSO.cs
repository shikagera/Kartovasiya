// Element definition (ScriptableObject) / Определение стихии
// Chaos Spellweaver (Картовасия)

using UnityEngine;
using Kartovasiya.Core;

namespace Kartovasiya.Elements
{
    /// <summary>
    /// Design-time data for one of the 16 elements (Basic or Extended).
    /// Данные одной стихии: базовой или расширенной (редкой).
    /// Create via: Assets → Create → Kartovasiya → Element
    /// </summary>
    [CreateAssetMenu(
        fileName = "Element_",
        menuName = "Kartovasiya/Elements/Element",
        order = 0)]
    public class ElementSO : ScriptableObject
    {
        [Header("Identity / Идентификация")]
        [Tooltip("Stable enum id used in code and combos. / Стабильный id для кода и комбо.")]
        [SerializeField] private ElementId _id = ElementId.None;

        [Tooltip("Basic (common) or Extended (rare unlock). / Базовая или расширенная (редкая).")]
        [SerializeField] private ElementTier _tier = ElementTier.Basic;

        [Tooltip("English display name.")]
        [SerializeField] private string _displayNameEn = "Element";

        [Tooltip("Russian display name. / Отображаемое имя (RU).")]
        [SerializeField] private string _displayNameRu = "Стихия";

        [Header("Presentation / Презентация (Tarot 2.5D)")]
        [Tooltip("Primary tint for card frames / VFX. / Основной цвет рамки и VFX.")]
        [SerializeField] private Color _primaryColor = Color.white;

        [Tooltip("Secondary / accent color. / Акцентный цвет.")]
        [SerializeField] private Color _accentColor = Color.gray;

        [Tooltip("Element glyph / suit icon. / Символ стихии.")]
        [SerializeField] private Sprite _icon;

        [Tooltip("Optional ornate border for tarot-style cards. / Орнамент рамки в стиле Таро.")]
        [SerializeField] private Sprite _frameOverlay;

        [Header("Lore / Лор")]
        [TextArea(2, 5)]
        [SerializeField] private string _descriptionEn;

        [TextArea(2, 5)]
        [SerializeField] private string _descriptionRu;

        [Header("Relations / Связи")]
        [Tooltip("Optional opposing element (e.g. Fire vs Water). / Противоположная стихия.")]
        [SerializeField] private ElementId _opposedElement = ElementId.None;

        [Tooltip("Sort order in UI palettes (0..15). / Порядок в UI.")]
        [SerializeField] private int _sortOrder;

        // ── Public API ────────────────────────────────────────────────

        public ElementId Id => _id;
        public ElementTier Tier => _tier;
        public string DisplayNameEn => _displayNameEn;
        public string DisplayNameRu => _displayNameRu;
        public Color PrimaryColor => _primaryColor;
        public Color AccentColor => _accentColor;
        public Sprite Icon => _icon;
        public Sprite FrameOverlay => _frameOverlay;
        public string DescriptionEn => _descriptionEn;
        public string DescriptionRu => _descriptionRu;
        public ElementId OpposedElement => _opposedElement;
        public int SortOrder => _sortOrder;

        public bool IsBasic => _tier == ElementTier.Basic;
        public bool IsExtended => _tier == ElementTier.Extended;

        public string GetDisplayName(bool russian = true) =>
            russian ? _displayNameRu : _displayNameEn;

        public string GetDescription(bool russian = true) =>
            russian ? _descriptionRu : _descriptionEn;

#if UNITY_EDITOR
        public void EditorApply(
            ElementId id,
            ElementTier tier,
            string nameEn,
            string nameRu,
            Color primary,
            Color accent,
            ElementId opposed,
            int sortOrder,
            string descEn,
            string descRu)
        {
            _id = id;
            _tier = tier;
            _displayNameEn = nameEn;
            _displayNameRu = nameRu;
            _primaryColor = primary;
            _accentColor = accent;
            _opposedElement = opposed;
            _sortOrder = sortOrder;
            _descriptionEn = descEn;
            _descriptionRu = descRu;
        }
#endif
    }
}
