// Drop target for Effect / buff cards on the player
// Цель для баффов: перетащить карту-эффект на себя
// Chaos Spellweaver (Картовасия)

using UnityEngine;
using UnityEngine.UI;

namespace Kartovasiya.Combat
{
    /// <summary>
    /// Raycast target over the player strip — drop Effect cards here to activate.
    /// </summary>
    [RequireComponent(typeof(Image))]
    public class PlayerDropZone : MonoBehaviour
    {
        public static PlayerDropZone Instance { get; private set; }

        [SerializeField] private Color _normalColor = new Color(0.05f, 0.12f, 0.08f, 0.55f);
        [SerializeField] private Color _hoverColor = new Color(0.15f, 0.45f, 0.28f, 0.85f);

        public Image Background { get; private set; }

        private void Awake()
        {
            Instance = this;
            Background = GetComponent<Image>();
            if (Background != null)
            {
                Background.color = _normalColor;
                Background.raycastTarget = true;
            }
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
    }
}
