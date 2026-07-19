// Touch / mouse drag: Element→Spell, Spell→Enemy, Effect→Player buff
// Drag: элемент на закл., заклинание на врага, бафф на себя
// Chaos Spellweaver (Картовасия)

using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using Kartovasiya.Combat;
using Kartovasiya.Core;
using Kartovasiya.UI;

namespace Kartovasiya.Hand
{
    /// <summary>
    /// uGUI drag for fusion, casting and buff activation.
    /// </summary>
    [RequireComponent(typeof(CardView))]
    public class CardDragHandler : MonoBehaviour,
        IBeginDragHandler, IDragHandler, IEndDragHandler, IPointerDownHandler
    {
        [SerializeField] private bool _draggable = true;
        [SerializeField] private float _dragScale = 1.08f;
        [SerializeField] private Canvas _canvas;

        private CardView _view;
        private RectTransform _rect;
        private Transform _originalParent;
        private int _originalSiblingIndex;
        private Vector3 _originalScale;
        private bool _dragging;
        private CardDropTarget _currentSpellHighlight;
        private bool _enemyHighlighted;
        private bool _playerHighlighted;

        public bool IsDragging => _dragging;
        public CardView View => _view;

        private void Awake()
        {
            _view = GetComponent<CardView>();
            _rect = transform as RectTransform;
            if (_canvas == null)
                _canvas = GetComponentInParent<Canvas>();
        }

        public void SetDraggable(bool value) => _draggable = value;

        public void SetCanvas(Canvas canvas) => _canvas = canvas;

        public void OnPointerDown(PointerEventData eventData)
        {
            if (_draggable)
                transform.SetAsLastSibling();
        }

        public void OnBeginDrag(PointerEventData eventData)
        {
            if (!_draggable || _view?.Data == null)
                return;
            if (!IsDragAllowed(_view.Data.CardType))
                return;
            if (_view.Data.HasFlag(CardFlags.Exhausted))
                return;

            var combat = CombatManager.Instance;
            if (combat != null && combat.IsCombatActive)
            {
                // Need player-action phase; Effect can still be inspected/played if plays left
                // or just inspected (snap back) when no plays.
                if (combat.Phase != TurnPhase.PlayerAction)
                    return;

                // Spells/elements require remaining plays; effects can always be dragged to view/self
                if (_view.Data.CardType != CardType.Effect && !combat.CanPlayerAct)
                    return;
            }

            _dragging = true;
            _originalParent = transform.parent;
            _originalSiblingIndex = transform.GetSiblingIndex();
            _originalScale = transform.localScale;

            if (_canvas != null)
            {
                transform.SetParent(_canvas.transform, true);
                transform.SetAsLastSibling();
            }

            transform.localScale = _originalScale * _dragScale;
            _view.SetDragging(true);

            if (HandManager.Instance != null)
                HandManager.Instance.NotifyDragStarted(_view);
        }

        public void OnDrag(PointerEventData eventData)
        {
            if (!_dragging)
                return;

            if (_canvas == null)
            {
                _rect.position = eventData.position;
            }
            else
            {
                RectTransformUtility.ScreenPointToLocalPointInRectangle(
                    _canvas.transform as RectTransform,
                    eventData.position,
                    _canvas.renderMode == RenderMode.ScreenSpaceOverlay ? null : _canvas.worldCamera,
                    out var localPoint);
                _rect.localPosition = localPoint;
            }

            UpdateHighlights(eventData);
        }

        public void OnEndDrag(PointerEventData eventData)
        {
            if (!_dragging)
                return;

            _dragging = false;
            ClearHighlights();

            bool resolved = false;
            var data = _view.Data;

            if (data != null && data.CardType == CardType.Element)
            {
                // Onto spell (sockets) OR onto another element (hybrid: Dark Fire, etc.)
                var drop = FindFuseDrop(eventData);
                if (drop != null && HandManager.Instance != null)
                {
                    resolved = HandManager.Instance.TryFuseCards(_view, drop.View);
                    if (!resolved)
                        ProceduralSfx.Instance?.PlayDropFail();
                    else
                        ProceduralSfx.Instance?.PlayDropOk();
                }
                else if (IsOverEnemy(eventData))
                {
                    ProceduralSfx.Instance?.PlayDropFail();
                    CombatManager.Instance?.EmitPlayerHint(
                        "Стихию нельзя кинуть во врага — смешай с другой стихией или влей в заклинание.",
                        "Elements can't hit the enemy — mix with another element or infuse a spell.");
                }
                else if (drop == null)
                    ProceduralSfx.Instance?.PlayDropFail();
            }
            else if (data != null && data.CardType == CardType.Spell)
            {
                if (IsOverEnemy(eventData) && CombatManager.Instance != null)
                {
                    resolved = CombatManager.Instance.TryCastSpell(_view);
                    if (!resolved)
                        ProceduralSfx.Instance?.PlayDropFail();
                }
                else
                    ProceduralSfx.Instance?.PlayDropFail();
            }
            else if (data != null && data.CardType == CardType.Effect)
            {
                if (IsOverEnemy(eventData))
                {
                    // Buffs are not attacks
                    ProceduralSfx.Instance?.PlayDropFail();
                    CombatManager.Instance?.EmitPlayerHint(
                        "Бафф (✦) — на себя, не на врага.",
                        "Buff (✦) — drop on yourself, not the enemy.");
                }
                else if (IsOverPlayer(eventData) && CombatManager.Instance != null)
                {
                    resolved = CombatManager.Instance.TryPlayEffect(_view);
                    if (!resolved)
                        ProceduralSfx.Instance?.PlayDropFail();
                    else
                        ProceduralSfx.Instance?.PlayDropOk();
                }
                // else: snap back — card still grants passive while in hand
            }

            if (_view != null)
            {
                _view.SetDragging(false);
                transform.localScale = _originalScale;
            }

            if (!resolved && _view != null)
            {
                if (_originalParent != null)
                {
                    transform.SetParent(_originalParent, false);
                    transform.SetSiblingIndex(_originalSiblingIndex);
                }

                if (HandManager.Instance != null)
                    HandManager.Instance.Relayout();
            }

            if (HandManager.Instance != null && _view != null)
                HandManager.Instance.NotifyDragEnded(_view, resolved);
        }

        private static bool IsDragAllowed(CardType type)
        {
            return type == CardType.Element || type == CardType.Spell || type == CardType.Effect;
        }

        private void UpdateHighlights(PointerEventData eventData)
        {
            if (_view?.Data == null)
                return;

            if (_view.Data.CardType == CardType.Element)
            {
                var target = FindFuseDrop(eventData);
                if (target == _currentSpellHighlight)
                    return;
                ClearSpellHighlight();
                _currentSpellHighlight = target;
                _currentSpellHighlight?.View?.SetDropHighlight(true);
            }
            else if (_view.Data.CardType == CardType.Spell)
            {
                bool over = IsOverEnemy(eventData);
                if (over == _enemyHighlighted)
                    return;
                _enemyHighlighted = over;
                EnemyDropZone.Instance?.SetHighlight(over);
            }
            else if (_view.Data.CardType == CardType.Effect)
            {
                bool overP = IsOverPlayer(eventData);
                if (overP == _playerHighlighted)
                    return;
                _playerHighlighted = overP;
                PlayerDropZone.Instance?.SetHighlight(overP);
            }
        }

        private void ClearHighlights()
        {
            ClearSpellHighlight();
            if (_enemyHighlighted)
            {
                EnemyDropZone.Instance?.SetHighlight(false);
                _enemyHighlighted = false;
            }

            if (_playerHighlighted)
            {
                PlayerDropZone.Instance?.SetHighlight(false);
                _playerHighlighted = false;
            }
        }

        private void ClearSpellHighlight()
        {
            if (_currentSpellHighlight != null)
            {
                _currentSpellHighlight.View?.SetDropHighlight(false);
                _currentSpellHighlight = null;
            }
        }

        /// <summary>Spell with free sockets, or Element that can still mix.</summary>
        private CardDropTarget FindFuseDrop(PointerEventData eventData)
        {
            var results = new List<RaycastResult>();
            if (EventSystem.current == null)
                return null;
            EventSystem.current.RaycastAll(eventData, results);

            for (int i = 0; i < results.Count; i++)
            {
                var go = results[i].gameObject;
                if (go == gameObject || go.transform.IsChildOf(transform))
                    continue;

                var drop = go.GetComponentInParent<CardDropTarget>();
                if (drop == null || !drop.AcceptsDrops)
                    continue;
                if (drop.View == null || drop.View == _view)
                    continue;
                if (drop.View.Data == null)
                    continue;

                if (drop.View.Data.CardType == CardType.Spell && drop.View.Data.CanAcceptElement())
                    return drop;
                if (drop.View.Data.CardType == CardType.Element && drop.View.Data.CanAcceptElementMix())
                    return drop;
            }

            return null;
        }

        private bool IsOverEnemy(PointerEventData eventData)
        {
            if (EnemyDropZone.Instance == null || EventSystem.current == null)
                return false;

            var results = new List<RaycastResult>();
            EventSystem.current.RaycastAll(eventData, results);
            for (int i = 0; i < results.Count; i++)
            {
                if (results[i].gameObject.GetComponentInParent<EnemyDropZone>() != null)
                    return true;
            }

            return false;
        }

        private bool IsOverPlayer(PointerEventData eventData)
        {
            if (PlayerDropZone.Instance == null || EventSystem.current == null)
                return false;

            var results = new List<RaycastResult>();
            EventSystem.current.RaycastAll(eventData, results);
            for (int i = 0; i < results.Count; i++)
            {
                if (results[i].gameObject.GetComponentInParent<PlayerDropZone>() != null)
                    return true;
            }

            return false;
        }
    }
}
