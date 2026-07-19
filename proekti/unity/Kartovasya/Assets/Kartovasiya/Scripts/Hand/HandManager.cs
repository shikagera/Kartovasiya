// Hand: hold cards, layout, fuse via drag-drop
// Рука: карты, раскладка, fusion через drag-drop
// Chaos Spellweaver (Картовасия)

using System;
using System.Collections.Generic;
using UnityEngine;
using Kartovasiya.Cards;
using Kartovasiya.Combos;
using Kartovasiya.Core;

namespace Kartovasiya.Hand
{
    /// <summary>
    /// Owns the player's hand for vertical combat UI (enemy top / hand bottom).
    /// Владеет рукой: низ экрана в вертикальном боевом UI.
    /// Drag Element onto Spell → CardFusionService → instant combo name/tint.
    /// </summary>
    public class HandManager : MonoBehaviour
    {
        public static HandManager Instance { get; private set; }

        [Header("References / Ссылки")]
        [SerializeField] private RectTransform _handContainer;
        [SerializeField] private Canvas _canvas;
        [SerializeField] private ComboDatabaseSO _comboDatabase;
        [SerializeField] private CardView _cardPrefab;

        [Header("Layout / Раскладка")]
        [SerializeField] private HandLayoutStyle _layoutStyle = HandLayoutStyle.BottomRow;
        [SerializeField] private Vector2 _cardSize = new Vector2(148f, 228f); // tarot ~2:3 — fits bottom fan
        [SerializeField] private float _spacing = 10f;
        [SerializeField] private float _fanAngleMax = 10f;
        [SerializeField] private bool _buildRuntimeCardsIfNoPrefab = true;

        [Header("Limits / Лимиты")]
        [SerializeField] private int _maxHandSize = GameConstants.MaxHandSize;

        private readonly List<CardInstance> _cards = new List<CardInstance>(GameConstants.MaxHandSize);
        private readonly List<CardView> _views = new List<CardView>(GameConstants.MaxHandSize);
        private CardFusionService _fusion;
        private ScreenLayoutMode _screenMode = ScreenLayoutMode.Portrait;
        private CardView _selected;

        public IReadOnlyList<CardInstance> Cards => _cards;
        public IReadOnlyList<CardView> Views => _views;
        public int Count => _cards.Count;
        public int MaxHandSize => _maxHandSize;
        public CardFusionService Fusion => _fusion;
        public ComboDatabaseSO ComboDatabase => _comboDatabase;
        public CardView Selected => _selected;

        public event Action<CardInstance, CardView> CardAdded;
        public event Action<CardInstance> CardRemoved;
        public event Action<CardInstance, CardInstance, ComboResolveResult> FusionSucceeded;
        public event Action<CardView, CardView, string> FusionFailed;
        public event Action HandChanged;

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Debug.LogWarning("[HandManager] Duplicate instance destroyed.", this);
                Destroy(gameObject);
                return;
            }

            Instance = this;

            if (_canvas == null)
                _canvas = GetComponentInParent<Canvas>();

            if (_handContainer == null)
                _handContainer = transform as RectTransform;

            EnsureFusionService();
            _screenMode = OrientationService.Detect();
        }

        private void OnEnable()
        {
            if (OrientationService.Instance != null)
                OrientationService.Instance.LayoutChanged += OnLayoutModeChanged;
        }

        private void OnDisable()
        {
            if (OrientationService.Instance != null)
                OrientationService.Instance.LayoutChanged -= OnLayoutModeChanged;
        }

        private void OnDestroy()
        {
            if (Instance == this)
                Instance = null;
        }

        private void EnsureFusionService()
        {
            if (_comboDatabase == null)
            {
                // Runtime prototype DB
                _comboDatabase = ScriptableObject.CreateInstance<ComboDatabaseSO>();
                _comboDatabase.LoadPrototypeDefaults();
            }
            else if (_comboDatabase.Recipes == null || _comboDatabase.Recipes.Count == 0)
            {
                _comboDatabase.LoadPrototypeDefaults();
            }

            _fusion = new CardFusionService(_comboDatabase);
            _fusion.Fused += OnFusionServiceFused;
            _fusion.FusionFailed += OnFusionServiceFailed;
        }

        /// <summary>Inject combo DB from bootstrap / combat. / Подставить базу комбо.</summary>
        public void SetComboDatabase(ComboDatabaseSO db)
        {
            if (_fusion != null)
            {
                _fusion.Fused -= OnFusionServiceFused;
                _fusion.FusionFailed -= OnFusionServiceFailed;
            }

            _comboDatabase = db;
            EnsureFusionService();
        }

        public void SetLayoutStyle(HandLayoutStyle style)
        {
            _layoutStyle = style;
            Relayout();
        }

        public void SetHandContainer(RectTransform container)
        {
            _handContainer = container;
            Relayout();
        }

        public void SetCanvas(Canvas canvas)
        {
            _canvas = canvas;
            for (int i = 0; i < _views.Count; i++)
            {
                if (_views[i] != null && _views[i].DragHandler != null)
                    _views[i].DragHandler.SetCanvas(canvas);
            }
        }

        // ── Hand mutations ────────────────────────────────────────────

        public bool CanAddCard() => _cards.Count < _maxHandSize;

        public CardView AddCard(CardInstance card)
        {
            if (card == null)
                return null;

            if (!CanAddCard())
            {
                Debug.Log("[HandManager] Hand full.", this);
                return null;
            }

            _cards.Add(card);
            var view = CreateView(card);
            _views.Add(view);
            Relayout();
            CardAdded?.Invoke(card, view);
            HandChanged?.Invoke();
            return view;
        }

        public bool RemoveCard(CardInstance card, bool destroyView = true)
        {
            if (card == null)
                return false;

            int index = _cards.IndexOf(card);
            if (index < 0)
                return false;

            _cards.RemoveAt(index);
            var view = _views[index];
            _views.RemoveAt(index);

            if (_selected == view)
                _selected = null;

            if (destroyView && view != null)
                Destroy(view.gameObject);

            Relayout();
            CardRemoved?.Invoke(card);
            HandChanged?.Invoke();
            return true;
        }

        public bool RemoveCard(CardView view, bool destroyView = true)
        {
            if (view == null)
                return false;
            return RemoveCard(view.Data, destroyView);
        }

        /// <summary>
        /// Remove card from hand but keep the CardView alive (cast fly animation).
        /// Убрать из руки, не уничтожая view (анимация полёта каста).
        /// </summary>
        public bool DetachCard(CardInstance card, out CardView view)
        {
            view = null;
            if (card == null)
                return false;

            int index = _cards.IndexOf(card);
            if (index < 0)
                return false;

            _cards.RemoveAt(index);
            view = _views[index];
            _views.RemoveAt(index);
            Relayout();
            CardRemoved?.Invoke(card);
            HandChanged?.Invoke();
            return true;
        }

        public void Clear()
        {
            for (int i = _views.Count - 1; i >= 0; i--)
            {
                if (_views[i] != null)
                    Destroy(_views[i].gameObject);
            }

            _views.Clear();
            _cards.Clear();
            HandChanged?.Invoke();
        }

        public CardView FindView(CardInstance card)
        {
            for (int i = 0; i < _cards.Count; i++)
            {
                if (_cards[i] == card)
                    return _views[i];
            }

            return null;
        }

        // ── Fusion (called from CardDragHandler) ──────────────────────

        /// <summary>
        /// Fuse source Element onto Spell (sockets) or Element (hybrid mix).
        /// Стихия → заклинание или стихия → стихия (тёмный огонь и т.п.).
        /// </summary>
        public bool TryFuseCards(CardView sourceView, CardView targetView)
        {
            if (sourceView == null || targetView == null)
                return false;
            if (sourceView.Data == null || targetView.Data == null)
                return false;

            var combat = Kartovasiya.Combat.CombatManager.Instance;
            if (combat != null && combat.IsCombatActive)
            {
                if (!combat.CanPlayerAct)
                {
                    FusionFailed?.Invoke(sourceView, targetView, "no_plays");
                    return false;
                }
            }

            if (_fusion == null)
                EnsureFusionService();

            bool ok = _fusion.TryFuse(sourceView.Data, targetView.Data, out var result);
            if (!ok)
            {
                FusionFailed?.Invoke(sourceView, targetView, "fuse_rejected");
                return false;
            }

            bool ontoSpell = targetView.Data.CardType == CardType.Spell;
            Color fuseTint = result.Recipe != null
                ? result.Recipe.ResultTint
                : (targetView.Data.ActiveComboTint.a > 0.1f
                    ? targetView.Data.ActiveComboTint
                    : Kartovasiya.UI.TarotPalette.ForElement(sourceView.Data.ProvidedElement));

            Vector3 sparkPos = targetView.transform.position;
            Kartovasiya.UI.CombatVfxService.Instance?.PlayFuseSpark(sparkPos, fuseTint);

            if (ontoSpell && result.Success && result.Recipe != null)
            {
                string comboName = result.Recipe.GetResultName(true);
                Kartovasiya.UI.CombatVfxService.Instance?.PlayComboBurst(
                    sparkPos, fuseTint, comboName, result.RiskTriggered);
            }
            else if (!ontoSpell)
            {
                // Element hybrid pop
                Kartovasiya.UI.CombatVfxService.Instance?.PlayComboBurst(
                    sparkPos, fuseTint, targetView.Data.GetDisplayName(true), false);
            }

            targetView.PlayFuseFeedback(fuseTint);

            // Consume source element
            RemoveCard(sourceView.Data, destroyView: true);

            targetView.RefreshVisuals();

            if (targetView.transform.parent != _handContainer)
                targetView.transform.SetParent(_handContainer, false);

            Relayout();
            targetView.Animator?.CaptureBase();
            FusionSucceeded?.Invoke(sourceView.Data, targetView.Data, result);

            if (ontoSpell && result.Success && result.Recipe != null)
            {
                Debug.Log($"[HandManager] Combo: {result.Recipe.GetResultName(true)}", this);
            }
            else if (!ontoSpell)
            {
                Debug.Log($"[HandManager] Element mix → {targetView.Data.GetDisplayName(true)}", this);
            }

            return true;
        }

        // ── Selection (enlarge + read description) ────────────────────

        /// <summary>
        /// Tap a card to enlarge (~2×) and show description. Tap again / another to deselect.
        /// </summary>
        public void SelectCard(CardView view)
        {
            if (view == null)
            {
                ClearSelection();
                return;
            }

            if (_selected == view)
            {
                ClearSelection();
                return;
            }

            if (_selected != null)
                _selected.SetSelected(false);

            _selected = view;
            _selected.SetSelected(true);
        }

        public void ClearSelection()
        {
            if (_selected != null)
            {
                _selected.SetSelected(false);
                _selected = null;
            }
        }

        // ── Layout ────────────────────────────────────────────────────

        public void Relayout()
        {
            if (_handContainer == null)
                return;

            // Compact null holes (safety)
            for (int i = _views.Count - 1; i >= 0; i--)
            {
                if (_views[i] == null)
                {
                    if (_selected == _views[i])
                        _selected = null;
                    _views.RemoveAt(i);
                    if (i < _cards.Count)
                        _cards.RemoveAt(i);
                }
            }

            // Re-parent any stray cards back to hand (except actively dragging)
            for (int i = 0; i < _views.Count; i++)
            {
                var v = _views[i];
                if (v == null) continue;
                if (v.DragHandler != null && v.DragHandler.IsDragging)
                    continue;
                if (v.transform.parent != _handContainer)
                    v.transform.SetParent(_handContainer, false);
            }

            var arr = _views.ToArray();
            HandLayout.Apply(
                _handContainer,
                arr,
                arr.Length,
                ResolveLayoutStyle(),
                _screenMode,
                _cardSize,
                _spacing,
                _fanAngleMax);

            // Keep inspect mode after relayout
            if (_selected != null && _selected.gameObject != null)
                _selected.SetSelected(true);
        }

        private HandLayoutStyle ResolveLayoutStyle()
        {
            // Auto: bottom row in portrait combat; side stack optional in landscape
            if (_layoutStyle == HandLayoutStyle.SideStack)
                return HandLayoutStyle.SideStack;
            return HandLayoutStyle.BottomRow;
        }

        private void OnLayoutModeChanged(ScreenLayoutMode mode)
        {
            _screenMode = mode;
            // Landscape → slightly smaller cards for width
            if (mode == ScreenLayoutMode.Landscape)
                _cardSize = new Vector2(128f, 196f);
            else
                _cardSize = new Vector2(148f, 228f);
            Relayout();
        }

        // ── View factory ──────────────────────────────────────────────

        private CardView CreateView(CardInstance card)
        {
            CardView view;

            if (_cardPrefab != null)
            {
                view = Instantiate(_cardPrefab, _handContainer);
            }
            else if (_buildRuntimeCardsIfNoPrefab)
            {
                view = CardView.BuildRuntime(_handContainer, _cardSize);
            }
            else
            {
                Debug.LogError("[HandManager] No card prefab and runtime build disabled.", this);
                return null;
            }

            if (view.DragHandler != null)
                view.DragHandler.SetCanvas(_canvas);

            view.Bind(card);
            return view;
        }

        // ── Drag notifications (hooks for VFX later) ──────────────────

        public void NotifyDragStarted(CardView view)
        {
            // Drop inspect mode while dragging
            if (_selected != null && _selected != view)
                ClearSelection();
            else if (_selected == view)
            {
                _selected.SetSelected(false);
                _selected = null;
            }

            // Highlight valid fuse targets when dragging an Element
            if (view?.Data == null || view.Data.CardType != CardType.Element)
                return;

            for (int i = 0; i < _views.Count; i++)
            {
                var v = _views[i];
                if (v == null || v == view || v.Data == null)
                    continue;
                if (v.Data.CardType == CardType.Spell && v.Data.CanAcceptElement())
                    v.SetDropHighlight(true);
                else if (v.Data.CardType == CardType.Element && v.Data.CanAcceptElementMix())
                    v.SetDropHighlight(true);
            }
        }

        public void NotifyDragEnded(CardView view, bool fused)
        {
            for (int i = 0; i < _views.Count; i++)
                _views[i]?.SetDropHighlight(false);
        }

        private void OnFusionServiceFused(CardInstance element, CardInstance spell, ComboResolveResult result)
        {
            // Already handled in TryFuseCards; event kept for external systems
        }

        private void OnFusionServiceFailed(CardInstance element, CardInstance spell, string reason)
        {
            Debug.Log($"[HandManager] Fusion failed: {reason}", this);
        }
    }
}
