// Registry of all 16 elements / Каталог всех 16 стихий
// Chaos Spellweaver (Картовасия)

using System.Collections.Generic;
using UnityEngine;
using Kartovasiya.Core;

namespace Kartovasiya.Elements
{
    /// <summary>
    /// Single source of truth for ElementSO lookup by ElementId.
    /// Единый источник ElementSO по ElementId (инжектить в менеджеры / генератор).
    /// </summary>
    [CreateAssetMenu(
        fileName = "ElementDatabase",
        menuName = "Kartovasiya/Elements/Element Database",
        order = 1)]
    public class ElementDatabaseSO : ScriptableObject
    {
        [Tooltip("All 16 element definitions. / Все 16 определений стихий.")]
        [SerializeField] private List<ElementSO> _elements = new List<ElementSO>(GameConstants.ElementCount);

        private Dictionary<ElementId, ElementSO> _lookup;

        /// <summary>Read-only list of registered elements. / Список зарегистрированных стихий.</summary>
        public IReadOnlyList<ElementSO> Elements => _elements;

        /// <summary>
        /// Build dictionary cache (safe to call multiple times).
        /// Строит кэш словаря (можно вызывать повторно).
        /// </summary>
        public void EnsureCache()
        {
            if (_lookup != null && _lookup.Count == _elements.Count)
                return;

            _lookup = new Dictionary<ElementId, ElementSO>(_elements.Count);
            for (int i = 0; i < _elements.Count; i++)
            {
                var el = _elements[i];
                if (el == null || el.Id == ElementId.None)
                    continue;

                if (_lookup.ContainsKey(el.Id))
                {
                    Debug.LogWarning($"[ElementDatabase] Duplicate ElementId {el.Id} on '{el.name}'.", this);
                    continue;
                }

                _lookup.Add(el.Id, el);
            }
        }

        public bool TryGet(ElementId id, out ElementSO element)
        {
            EnsureCache();
            return _lookup.TryGetValue(id, out element);
        }

        public ElementSO Get(ElementId id)
        {
            if (TryGet(id, out var element))
                return element;

            Debug.LogError($"[ElementDatabase] Missing element: {id}", this);
            return null;
        }

        public bool Contains(ElementId id)
        {
            EnsureCache();
            return _lookup.ContainsKey(id);
        }

        /// <summary>
        /// Returns elements sorted by SortOrder for UI palettes.
        /// Стихии, отсортированные для UI-палитры.
        /// </summary>
        public List<ElementSO> GetSortedForUi()
        {
            var list = new List<ElementSO>(_elements.Count);
            for (int i = 0; i < _elements.Count; i++)
            {
                if (_elements[i] != null)
                    list.Add(_elements[i]);
            }

            list.Sort((a, b) => a.SortOrder.CompareTo(b.SortOrder));
            return list;
        }

        /// <summary>
        /// Filter by Basic vs Extended (concept progression / drop tables).
        /// Фильтр Basic / Extended (прогрессия и таблицы дропа).
        /// </summary>
        public List<ElementSO> GetByTier(ElementTier tier)
        {
            var list = new List<ElementSO>();
            for (int i = 0; i < _elements.Count; i++)
            {
                var el = _elements[i];
                if (el != null && el.Tier == tier)
                    list.Add(el);
            }

            list.Sort((a, b) => a.SortOrder.CompareTo(b.SortOrder));
            return list;
        }

#if UNITY_EDITOR
        public void EditorSetElements(List<ElementSO> elements)
        {
            _elements = elements ?? new List<ElementSO>();
            _lookup = null;
            UnityEditor.EditorUtility.SetDirty(this);
        }
#endif

        private void OnEnable()
        {
            _lookup = null;
        }

        private void OnValidate()
        {
            _lookup = null;
        }
    }
}
