// Drop / click target for casting spells on enemy
// Цель для каста заклинаний на врага
// Chaos Spellweaver (Картовасия)

using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace Kartovasiya.Combat
{
    /// <summary>
    /// Raycast target covering the enemy panel (top of vertical layout).
    /// Drop a Spell here or tap after selecting — CombatManager resolves cast.
    /// </summary>
    [RequireComponent(typeof(Image))]
    public class EnemyDropZone : MonoBehaviour, IPointerClickHandler
    {
        public static EnemyDropZone Instance { get; private set; }

        [SerializeField] private Image _highlight;
        [SerializeField] private Color _normalColor = new Color(0.15f, 0.08f, 0.12f, 0.85f);
        [SerializeField] private Color _hoverColor = new Color(0.45f, 0.2f, 0.25f, 0.95f);

        public Image Background { get; private set; }

        private void Awake()
        {
            Instance = this;
            Background = GetComponent<Image>();
            Background.color = _normalColor;
            Background.raycastTarget = true;
        }

        private void OnDestroy()
        {
            if (Instance == this)
                Instance = null;
        }

        public void SetHighlight(bool on)
        {
            if (Background != null)
                Background.color = on ? _hoverColor : _normalColor;
        }

        public void OnPointerClick(PointerEventData eventData)
        {
            // Optional: double-purpose click cast if a spell is "armed" later
        }
    }
}
