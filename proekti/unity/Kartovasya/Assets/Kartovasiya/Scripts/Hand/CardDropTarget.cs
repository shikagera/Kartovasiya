// Drop zone on Spell / Element cards
// Зона drop: на заклинание или на другую стихию
// Chaos Spellweaver (Картовасия)

using UnityEngine;

namespace Kartovasiya.Hand
{
    /// <summary>
    /// Marker for Element → Spell or Element → Element fusion drops.
    /// </summary>
    [RequireComponent(typeof(CardView))]
    public class CardDropTarget : MonoBehaviour
    {
        [SerializeField] private bool _acceptsDrops = true;

        public CardView View { get; private set; }
        public bool AcceptsDrops => _acceptsDrops;

        private void Awake()
        {
            View = GetComponent<CardView>();
        }

        public void SetAcceptsDrops(bool value) => _acceptsDrops = value;
    }
}
