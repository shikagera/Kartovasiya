// Tarot-styled card widget (2.5D / mystical) / Карта в стиле Таро
// Chaos Spellweaver (Картовасия)

using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;
using Kartovasiya.Cards;
using Kartovasiya.Core;
using Kartovasiya.UI;

namespace Kartovasiya.Hand
{
    /// <summary>
    /// Tarot card presentation: ornate frame, illustration plate, title banner, pips.
    /// Презентация карты-Таро: рамка, «иллюстрация», баннер, символы стихий.
    /// </summary>
    [RequireComponent(typeof(RectTransform))]
    public class CardView : MonoBehaviour, IPointerEnterHandler, IPointerExitHandler, IPointerClickHandler
    {
        [Header("UI refs")]
        [SerializeField] private Image _outerFrame;
        [SerializeField] private Image _innerMat;
        [SerializeField] private Image _artPlate;
        [SerializeField] private Image _glyphDisc;
        [SerializeField] private Image _rarityGem;
        [SerializeField] private Image _banner;
        [SerializeField] private Image _powerMedallion;
        [SerializeField] private Text _titleText;
        [SerializeField] private Text _typeText;
        [SerializeField] private Text _glyphText;
        [SerializeField] private Text _powerText;
        [SerializeField] private Text _socketText;
        [SerializeField] private Text _flagText;
        [SerializeField] private Text _descText;
        [SerializeField] private CanvasGroup _canvasGroup;
        [SerializeField] private CardAnimator _animator;

        public CardInstance Data { get; private set; }
        public RectTransform Rect { get; private set; }
        public CardDragHandler DragHandler { get; private set; }
        public CardDropTarget DropTarget { get; private set; }
        public CardAnimator Animator => _animator;

        public int HandIndex { get; set; } = -1;
        public bool IsSelected { get; private set; }

        private Color _frameBase = TarotPalette.Gold;
        private bool _highlighted;
        private bool _hover;
        private int _baseTitleSize = 16;
        private int _baseTypeSize = 11;
        private int _baseDescSize = 12;
        private int _basePowerSize = 18;
        private int _baseSocketSize = 11;

        private void Awake()
        {
            Rect = transform as RectTransform;
            DragHandler = GetComponent<CardDragHandler>();
            DropTarget = GetComponent<CardDropTarget>();
            if (_canvasGroup == null)
                _canvasGroup = GetComponent<CanvasGroup>();
            if (_animator == null)
                _animator = GetComponent<CardAnimator>();
        }

        public void Bind(CardInstance data)
        {
            Data = data;
            RefreshVisuals();
            ConfigureInteraction();
            _animator?.CaptureBase();
            _animator?.PlayDealIn(HandIndex >= 0 ? HandIndex * 0.04f : 0f);
        }

        public void RefreshVisuals()
        {
            if (Data == null)
                return;

            bool ru = true;
            string title = Data.GetDisplayName(ru);
            if (_titleText != null)
                _titleText.text = title;

            if (_typeText != null)
            {
                _typeText.text = Data.CardType switch
                {
                    CardType.Spell => ru ? "ЗАКЛИНАНИЕ" : "SPELL",
                    CardType.Element => ru ? "СТИХИЯ" : "ELEMENT",
                    CardType.Effect => ru ? "ЭФФЕКТ" : "EFFECT",
                    CardType.Material => ru ? "МАТЕРИАЛ" : "MATERIAL",
                    _ => Data.CardType.ToString().ToUpperInvariant()
                };
            }

            ElementId focus = Data.CardType == CardType.Element
                ? Data.ProvidedElement
                : Data.PrimaryAffinity;

            if (Data.CardType == CardType.Element)
            {
                var granted = Data.GetGrantedElements();
                if (granted.Count > 0)
                    focus = granted[granted.Count - 1];
            }

            if (!string.IsNullOrEmpty(Data.ActiveComboId) && Data.CardType == CardType.Spell)
                focus = Data.InfusedElements != null && Data.InfusedElements.Count > 0
                    ? Data.InfusedElements[Data.InfusedElements.Count - 1]
                    : focus;

            Color elementCol = TarotPalette.ForElement(focus);
            Color typeCol = TarotPalette.ForCardType(Data.CardType);
            Color rarityCol = TarotPalette.ForRarity(Data.Rarity);

            // Beautiful 2D illustration by element / type
            if (_artPlate != null)
            {
                _artPlate.sprite = ProceduralCardArt.ForCardType(Data.CardType, focus);
                _artPlate.color = Color.white;
                _artPlate.preserveAspect = true;
                _artPlate.type = Image.Type.Simple;
            }

            if (_innerMat != null)
                _innerMat.color = Color.Lerp(TarotPalette.Parchment, elementCol, 0.15f);

            _frameBase = Color.Lerp(TarotPalette.Gold, rarityCol, 0.45f);
            if (!string.IsNullOrEmpty(Data.ActiveComboId))
                _frameBase = Color.Lerp(TarotPalette.GoldBright, Data.ActiveComboTint, 0.5f);
            if (Data.HasFlag(CardFlags.Corrupted))
                _frameBase = Color.Lerp(_frameBase, TarotPalette.BloodRed, 0.5f);

            if (_outerFrame != null && !_highlighted)
                _outerFrame.color = _frameBase;

            if (_glyphDisc != null)
                _glyphDisc.color = Color.Lerp(TarotPalette.Night, elementCol, 0.65f);

            if (_glyphText != null)
            {
                _glyphText.text = TarotPalette.ElementGlyphAscii(focus);
                _glyphText.color = TarotPalette.Parchment;
            }

            if (_rarityGem != null)
                _rarityGem.color = rarityCol;

            if (_banner != null)
                _banner.color = Color.Lerp(TarotPalette.VoidPurple, elementCol, 0.35f);

            if (_powerMedallion != null)
                _powerMedallion.color = TarotPalette.Gold;

            if (_powerText != null)
            {
                if (Data.CardType == CardType.Element)
                    _powerText.text = TarotPalette.ElementGlyphAscii(Data.ProvidedElement);
                else if (Data.CardType == CardType.Effect)
                    _powerText.text = "✦";
                else
                {
                    int power = Data.GetFinalPower();
                    // Prefer combat combo power if available
                    var cm = Kartovasiya.Combat.CombatManager.Instance;
                    if (cm != null && Data.CardType == CardType.Spell)
                        power = cm.PeekFinalSpellPower(Data);
                    _powerText.text = power.ToString();
                }
            }

            if (_socketText != null)
            {
                if (Data.CardType == CardType.Spell)
                {
                    int filled = Data.InfusedElements?.Count ?? 0;
                    string pips = string.Empty;
                    if (Data.InfusedElements != null)
                    {
                        for (int i = 0; i < Data.InfusedElements.Count; i++)
                            pips += TarotPalette.ElementGlyphAscii(Data.InfusedElements[i]);
                    }

                    _socketText.text = string.IsNullOrEmpty(pips)
                        ? $"⬡ {filled}/{Data.ElementSockets}"
                        : $"{pips}  {filled}/{Data.ElementSockets}";
                }
                else if (Data.CardType == CardType.Effect)
                {
                    _socketText.text = BuildEffectSummary(Data, ru);
                }
                else if (Data.CardType == CardType.Element)
                {
                    var granted = Data.GetGrantedElements();
                    if (granted.Count > 1)
                    {
                        string pips = string.Empty;
                        for (int i = 0; i < granted.Count; i++)
                            pips += TarotPalette.ElementGlyphAscii(granted[i]);
                        _socketText.text = ru ? $"{pips} гибрид" : $"{pips} hybrid";
                    }
                    else
                    {
                        _socketText.text = ru ? "на стихию / закл." : "onto element / spell";
                    }
                }
                else
                {
                    _socketText.text = ru ? "перетащи на закл." : "drag onto spell";
                }
            }

            if (_flagText != null)
            {
                string flags = string.Empty;
                if (Data.HasFlag(CardFlags.Wild)) flags += ru ? "★ДИК " : "★WILD ";
                if (Data.HasFlag(CardFlags.Corrupted)) flags += ru ? "☠ПОРЧА" : "☠CORR";
                if (!string.IsNullOrEmpty(Data.ActiveComboId))
                    flags = (ru ? "✦КОМБО " : "✦COMBO ") + flags;
                _flagText.text = flags.Trim();
                _flagText.color = Data.HasFlag(CardFlags.Corrupted) ? TarotPalette.Risk : TarotPalette.GoldBright;
            }

            if (_descText != null)
            {
                _descText.text = BuildReadableDescription(Data, ru);
                _descText.gameObject.SetActive(IsSelected);
            }

            // Prefer description over socket line while inspecting
            if (_socketText != null && IsSelected)
                _socketText.gameObject.SetActive(false);
            else if (_socketText != null)
                _socketText.gameObject.SetActive(true);

            ApplyHighlightVisual();
        }

        private static string BuildReadableDescription(CardInstance data, bool ru)
        {
            if (data == null) return string.Empty;

            var lines = new System.Text.StringBuilder(160);
            string body = ru ? data.DescriptionRu : data.DescriptionEn;
            if (string.IsNullOrEmpty(body))
                body = data.BuildTooltip(ru);
            else
            {
                lines.AppendLine(body);
                if (data.CardType == CardType.Spell)
                {
                    int power = data.GetFinalPower();
                    var cm = Kartovasiya.Combat.CombatManager.Instance;
                    if (cm != null)
                        power = cm.PeekFinalSpellPower(data);
                    lines.AppendLine(ru ? $"Сила: {power}" : $"Power: {power}");
                    if (data.InfusedElements != null && data.InfusedElements.Count > 0)
                    {
                        lines.Append(ru ? "Стихии: " : "Elements: ");
                        lines.Append(data.PrimaryAffinity);
                        for (int i = 0; i < data.InfusedElements.Count; i++)
                            lines.Append(" + ").Append(data.InfusedElements[i]);
                        lines.AppendLine();
                    }
                }
                else if (data.CardType == CardType.Effect)
                {
                    lines.AppendLine(ru ? "Перетащи ВНИЗ на зелёную зону." : "Drag DOWN onto the green zone.");
                }
                else if (data.CardType == CardType.Element)
                {
                    var g = data.GetGrantedElements();
                    if (g.Count > 1)
                    {
                        lines.Append(ru ? "Гибрид: " : "Hybrid: ");
                        for (int i = 0; i < g.Count; i++)
                        {
                            if (i > 0) lines.Append(" + ");
                            lines.Append(g[i]);
                        }

                        lines.AppendLine();
                    }

                    lines.AppendLine(ru
                        ? "На другую стихию = смесь. На заклинание = влить всё."
                        : "Onto element = mix. Onto spell = infuse all parts.");
                }

                return lines.ToString().Trim();
            }

            return body;
        }

        public void SetSelected(bool selected)
        {
            IsSelected = selected;
            if (_descText != null)
            {
                _descText.gameObject.SetActive(selected);
                if (selected && Data != null)
                    _descText.text = BuildReadableDescription(Data, true);
            }

            if (_socketText != null)
                _socketText.gameObject.SetActive(!selected);

            ApplySelectedReadability(selected);

            if (selected)
            {
                transform.SetAsLastSibling();
                _animator?.PlaySelected(true);
            }
            else if (DragHandler == null || !DragHandler.IsDragging)
            {
                _animator?.PlaySelected(false);
            }
        }

        /// <summary>
        /// Crisp larger fonts while inspecting — avoids blurry best-fit when scaled.
        /// </summary>
        private void ApplySelectedReadability(bool selected)
        {
            void Style(Text t, int baseSize, int selectedSize)
            {
                if (t == null) return;
                t.resizeTextForBestFit = false;
                t.fontSize = selected ? selectedSize : baseSize;
                t.horizontalOverflow = HorizontalWrapMode.Wrap;
                t.verticalOverflow = VerticalWrapMode.Truncate;
                // Slight shadow via color pop when selected
                if (selected)
                    t.color = t == _descText ? TarotPalette.Ink : t.color;
            }

            Style(_titleText, _baseTitleSize, Mathf.Max(_baseTitleSize + 6, 22));
            Style(_typeText, _baseTypeSize, Mathf.Max(_baseTypeSize + 4, 16));
            Style(_descText, _baseDescSize, Mathf.Max(_baseDescSize + 8, 20));
            Style(_powerText, _basePowerSize, Mathf.Max(_basePowerSize + 4, 24));
            Style(_socketText, _baseSocketSize, _baseSocketSize);
            Style(_flagText, 11, 15);

            if (_descText != null)
            {
                _descText.fontStyle = selected ? FontStyle.Bold : FontStyle.Normal;
                _descText.lineSpacing = selected ? 1.05f : 1f;
            }

            if (_titleText != null && selected)
                _titleText.fontStyle = FontStyle.Bold;
        }

        public void OnPointerClick(PointerEventData eventData)
        {
            if (DragHandler != null && DragHandler.IsDragging)
                return;
            HandManager.Instance?.SelectCard(this);
        }

        private static string BuildEffectSummary(CardInstance data, bool ru)
        {
            string core;
            if (data.CardsPerTurnBonus > 0)
                core = ru ? $"+{data.CardsPerTurnBonus} карт/ход" : $"+{data.CardsPerTurnBonus} plays";
            else if (data.CardFloodBonus > 0)
                core = ru ? $"+{data.CardFloodBonus} дроп" : $"+{data.CardFloodBonus} flood";
            else if (data.CritChanceBonus > 0f)
                core = ru ? $"крит +{Mathf.RoundToInt(data.CritChanceBonus * 100)}%" : $"crit +{Mathf.RoundToInt(data.CritChanceBonus * 100)}%";
            else
                core = ru ? "бафф" : "buff";

            // Hint: drag DOWN to bottom green zone
            return ru ? $"{core} · ⬇ вниз" : $"{core} · ⬇ down";
        }

        public void SetDragging(bool dragging)
        {
            if (_canvasGroup != null)
            {
                _canvasGroup.blocksRaycasts = !dragging;
                _canvasGroup.alpha = dragging ? 0.92f : 1f;
            }

            _animator?.PlayDrag(dragging);
            if (dragging)
                ProceduralSfx.Instance?.PlayPickup();
        }

        public void SetDropHighlight(bool on)
        {
            _highlighted = on;
            ApplyHighlightVisual();
            if (on)
                _animator?.PlayHover(true);
            else if (!_hover)
                _animator?.PlayHover(false);
        }

        public void PlayFuseFeedback(Color tint)
        {
            _animator?.PlayFusePulse(tint);
        }

        public void OnPointerEnter(PointerEventData eventData)
        {
            if (DragHandler != null && DragHandler.IsDragging)
                return;
            if (IsSelected)
                return;
            _hover = true;
            _animator?.PlayHover(true);
        }

        public void OnPointerExit(PointerEventData eventData)
        {
            _hover = false;
            if (IsSelected)
                return;
            if (!_highlighted)
                _animator?.PlayHover(false);
        }

        private void ApplyHighlightVisual()
        {
            if (_outerFrame == null)
                return;
            _outerFrame.color = _highlighted ? TarotPalette.Highlight : _frameBase;
        }

        private void ConfigureInteraction()
        {
            if (DragHandler != null)
            {
                // Spell / Element / Effect (buffs with ✦) are all draggable
                bool drag = Data != null &&
                            (Data.CardType == CardType.Element ||
                             Data.CardType == CardType.Spell ||
                             Data.CardType == CardType.Effect);
                DragHandler.SetDraggable(drag);
            }

            if (DropTarget != null)
            {
                // Spells accept elements; elements accept other elements (hybrids)
                bool accept = Data != null &&
                              (Data.CardType == CardType.Spell || Data.CardType == CardType.Element);
                DropTarget.SetAcceptsDrops(accept);
            }
        }

        /// <summary>
        /// Build full tarot card at runtime (no prefab art needed).
        /// Сборка карты-Таро в runtime.
        /// </summary>
        public static CardView BuildRuntime(Transform parent, Vector2 size)
        {
            // Root
            var root = new GameObject("TarotCard", typeof(RectTransform), typeof(CanvasGroup), typeof(Image));
            root.transform.SetParent(parent, false);
            var rect = root.GetComponent<RectTransform>();
            rect.sizeDelta = size;

            // Invisible hit area (full card)
            var hit = root.GetComponent<Image>();
            hit.color = new Color(0, 0, 0, 0.01f);
            hit.raycastTarget = true;

            // Ornate outer frame — gold Tarot border
            var outer = UiFactory.Image(root.transform, "OuterFrame",
                ProceduralSpriteFactory.RoundedRect(96, 140, 12, TarotPalette.Night, TarotPalette.GoldBright, 6),
                Color.white);
            UiFactory.Stretch(outer.rectTransform, 0f);
            outer.raycastTarget = false;

            // Silver inner rim
            var silver = UiFactory.Image(root.transform, "SilverRim",
                ProceduralSpriteFactory.RoundedRect(88, 128, 10,
                    new Color(0.08f, 0.06f, 0.1f, 1f),
                    new Color(0.75f, 0.78f, 0.85f, 1f), 3),
                Color.white);
            UiFactory.Stretch(silver.rectTransform, 4f);
            silver.raycastTarget = false;

            // Inner parchment
            var inner = UiFactory.Image(root.transform, "InnerMat",
                ProceduralSpriteFactory.RoundedRect(72, 110, 8, TarotPalette.Parchment, new Color(0.55f, 0.42f, 0.22f), 2),
                Color.white);
            UiFactory.Stretch(inner.rectTransform, 9f);

            // Illustration plate — 2D procedural art (filled in RefreshVisuals)
            var art = UiFactory.Image(root.transform, "ArtPlate",
                ProceduralSpriteFactory.RoundedRect(64, 64, 8, TarotPalette.Night, TarotPalette.Gold, 2),
                Color.white);
            UiFactory.Anchor(art.rectTransform, new Vector2(0.08f, 0.50f), new Vector2(0.92f, 0.92f));
            art.preserveAspect = true;

            // Small glyph badge (top-left of art) — secondary to illustration
            var disc = UiFactory.Image(root.transform, "GlyphDisc",
                ProceduralSpriteFactory.Disc(64, TarotPalette.VoidPurple, TarotPalette.Gold, 4),
                Color.white);
            var discRt = disc.rectTransform;
            discRt.anchorMin = new Vector2(0.14f, 0.84f);
            discRt.anchorMax = new Vector2(0.14f, 0.84f);
            discRt.sizeDelta = new Vector2(size.x * 0.18f, size.x * 0.18f);

            var glyph = UiFactory.Text(root.transform, "Glyph", "★", Mathf.RoundToInt(size.x * 0.09f),
                TextAnchor.MiddleCenter, TarotPalette.Parchment, FontStyle.Bold);
            UiFactory.Anchor(glyph.rectTransform, new Vector2(0.06f, 0.78f), new Vector2(0.22f, 0.92f));
            glyph.resizeTextForBestFit = false;

            // Title banner
            var banner = UiFactory.Image(root.transform, "Banner",
                ProceduralSpriteFactory.RoundedRect(64, 24, 4, TarotPalette.VoidPurple, TarotPalette.Gold, 2),
                Color.white);
            UiFactory.Anchor(banner.rectTransform, new Vector2(0.08f, 0.38f), new Vector2(0.92f, 0.48f));

            int titleSize = Mathf.RoundToInt(size.x * 0.11f);
            var title = UiFactory.Text(root.transform, "Title", "Card", titleSize,
                TextAnchor.MiddleCenter, TarotPalette.Parchment, FontStyle.Bold);
            UiFactory.Anchor(title.rectTransform, new Vector2(0.1f, 0.38f), new Vector2(0.9f, 0.48f));
            title.resizeTextForBestFit = false;
            title.horizontalOverflow = HorizontalWrapMode.Wrap;

            int typeSize = Mathf.RoundToInt(size.x * 0.07f);
            var type = UiFactory.Text(root.transform, "Type", "TYPE", typeSize,
                TextAnchor.MiddleCenter, TarotPalette.Gold);
            UiFactory.Anchor(type.rectTransform, new Vector2(0.1f, 0.32f), new Vector2(0.9f, 0.38f));
            type.resizeTextForBestFit = false;

            // Description (shown enlarged when selected) — fixed size = sharp when zoomed
            int descSize = Mathf.RoundToInt(size.x * 0.09f);
            var desc = UiFactory.Text(root.transform, "Desc", "", descSize,
                TextAnchor.UpperLeft, TarotPalette.Ink);
            UiFactory.Anchor(desc.rectTransform, new Vector2(0.08f, 0.14f), new Vector2(0.92f, 0.32f));
            desc.horizontalOverflow = HorizontalWrapMode.Wrap;
            desc.verticalOverflow = VerticalWrapMode.Truncate;
            desc.resizeTextForBestFit = false;
            desc.gameObject.SetActive(false);

            // Power medallion bottom
            var medal = UiFactory.Image(root.transform, "PowerMedal",
                ProceduralSpriteFactory.Disc(48, TarotPalette.Gold, TarotPalette.GoldBright, 3),
                Color.white);
            var mrt = medal.rectTransform;
            mrt.anchorMin = new Vector2(0.5f, 0.07f);
            mrt.anchorMax = new Vector2(0.5f, 0.07f);
            mrt.sizeDelta = new Vector2(size.x * 0.26f, size.x * 0.26f);

            var power = UiFactory.Text(root.transform, "Power", "0", Mathf.RoundToInt(size.x * 0.13f),
                TextAnchor.MiddleCenter, TarotPalette.Ink, FontStyle.Bold);
            UiFactory.Anchor(power.rectTransform, new Vector2(0.35f, 0.01f), new Vector2(0.65f, 0.14f));
            power.resizeTextForBestFit = false;

            var sockets = UiFactory.Text(root.transform, "Sockets", "", Mathf.RoundToInt(size.x * 0.07f),
                TextAnchor.MiddleCenter, TarotPalette.Ink);
            UiFactory.Anchor(sockets.rectTransform, new Vector2(0.08f, 0.12f), new Vector2(0.92f, 0.20f));
            sockets.resizeTextForBestFit = false;

            var flags = UiFactory.Text(root.transform, "Flags", "", Mathf.RoundToInt(size.x * 0.07f),
                TextAnchor.UpperCenter, TarotPalette.GoldBright, FontStyle.Bold);
            UiFactory.Anchor(flags.rectTransform, new Vector2(0.08f, 0.90f), new Vector2(0.92f, 0.98f));
            flags.resizeTextForBestFit = false;

            // Rarity gem top-right
            var gem = UiFactory.Image(root.transform, "RarityGem",
                ProceduralSpriteFactory.Disc(24, TarotPalette.Gold, Color.white, 2),
                Color.white);
            var gr = gem.rectTransform;
            gr.anchorMin = new Vector2(0.82f, 0.9f);
            gr.anchorMax = new Vector2(0.82f, 0.9f);
            gr.sizeDelta = new Vector2(size.x * 0.14f, size.x * 0.14f);

            // Corner ornaments (tarot pips)
            AddCornerPip(root.transform, new Vector2(0.12f, 0.92f), size);
            AddCornerPip(root.transform, new Vector2(0.88f, 0.08f), size, flip: true);

            var view = root.AddComponent<CardView>();
            view.Rect = rect;
            view._outerFrame = outer;
            view._innerMat = inner;
            view._artPlate = art;
            view._glyphDisc = disc;
            view._rarityGem = gem;
            view._banner = banner;
            view._powerMedallion = medal;
            view._titleText = title;
            view._typeText = type;
            view._glyphText = glyph;
            view._powerText = power;
            view._socketText = sockets;
            view._flagText = flags;
            view._descText = desc;
            view._canvasGroup = root.GetComponent<CanvasGroup>();
            view._frameBase = TarotPalette.Gold;
            view._baseTitleSize = titleSize;
            view._baseTypeSize = typeSize;
            view._baseDescSize = descSize;
            view._basePowerSize = Mathf.RoundToInt(size.x * 0.13f);
            view._baseSocketSize = Mathf.RoundToInt(size.x * 0.07f);

            root.AddComponent<CardAnimator>();
            root.AddComponent<CardDragHandler>();
            root.AddComponent<CardDropTarget>();

            view._animator = root.GetComponent<CardAnimator>();
            view.DragHandler = root.GetComponent<CardDragHandler>();
            view.DropTarget = root.GetComponent<CardDropTarget>();

            return view;
        }

        private static void AddCornerPip(Transform parent, Vector2 anchor, Vector2 cardSize, bool flip = false)
        {
            var pip = UiFactory.Image(parent, flip ? "PipBL" : "PipTR",
                ProceduralSpriteFactory.Disc(16, TarotPalette.Gold, TarotPalette.GoldBright, 2),
                Color.white);
            var rt = pip.rectTransform;
            rt.anchorMin = anchor;
            rt.anchorMax = anchor;
            rt.sizeDelta = new Vector2(cardSize.x * 0.1f, cardSize.x * 0.1f);
            if (flip)
                rt.localRotation = Quaternion.Euler(0f, 0f, 180f);
        }
    }
}
