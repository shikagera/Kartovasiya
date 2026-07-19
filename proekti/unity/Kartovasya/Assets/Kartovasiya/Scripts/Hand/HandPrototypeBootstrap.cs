// Play Mode prototype: canvas + sample hand + drag-drop fusion
// Прототип: canvas, демо-рука, drag-drop комбо
// Chaos Spellweaver (Картовасия)

using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;
using Kartovasiya.Cards;
using Kartovasiya.Combos;
using Kartovasiya.Core;

namespace Kartovasiya.Hand
{
    /// <summary>
    /// Drop this on an empty scene object and press Play.
    /// Builds vertical combat stub: enemy zone (top) + hand (bottom) + demo cards.
    /// Повесь на пустой объект в сцене → Play: враг сверху, рука снизу, демо-карты.
    /// Try: drag Fire onto a spell, then Water → Steam combo.
    /// </summary>
    public class HandPrototypeBootstrap : MonoBehaviour
    {
        [Header("Demo / Демо")]
        [SerializeField] private bool _spawnOnAwake = true;
        [SerializeField] private bool _useRussianHints = true;

        [Header("Optional overrides")]
        [SerializeField] private ComboDatabaseSO _comboDatabase;

        private HandManager _hand;
        private Text _statusText;
        private Text _hintText;

        private void Awake()
        {
            if (_spawnOnAwake)
                Build();
        }

        [ContextMenu("Rebuild Prototype UI")]
        public void Build()
        {
            EnsureEventSystem();
            EnsureOrientationService();

            var canvas = CreateCanvas();
            CreateEnemyZone(canvas.transform);
            var handRect = CreateHandZone(canvas.transform);
            _statusText = CreateStatusBar(canvas.transform);
            _hintText = CreateHint(canvas.transform);

            var handGo = new GameObject("HandManager", typeof(RectTransform));
            handGo.transform.SetParent(handRect, false);
            var hmRect = handGo.GetComponent<RectTransform>();
            StretchFull(hmRect);

            _hand = handGo.AddComponent<HandManager>();
            _hand.SetCanvas(canvas);
            _hand.SetHandContainer(handRect);

            if (_comboDatabase != null)
                _hand.SetComboDatabase(_comboDatabase);
            else
            {
                var db = ScriptableObject.CreateInstance<ComboDatabaseSO>();
                db.LoadPrototypeDefaults();
                _hand.SetComboDatabase(db);
            }

            _hand.CardAdded += (_, __) => RefreshStatus();
            _hand.CardRemoved += _ => RefreshStatus();
            _hand.FusionSucceeded += OnFusion;
            _hand.HandChanged += RefreshStatus;

            SeedDemoHand();
            RefreshStatus();
        }

        private void SeedDemoHand()
        {
            // Spells with sockets
            _hand.AddCard(MakeSpell("Ember Bolt", "Уголь-болт", ElementId.Fire, 6, sockets: 2));
            _hand.AddCard(MakeSpell("Tide Slash", "Удар прилива", ElementId.Water, 5, sockets: 2));
            _hand.AddCard(MakeSpell("Blank Sigil", "Пустая печать", ElementId.Arcane, 4, sockets: 3));

            // Element catalysts for drag
            _hand.AddCard(MakeElement(ElementId.Fire, "Fire", "Огонь"));
            _hand.AddCard(MakeElement(ElementId.Water, "Water", "Вода"));
            _hand.AddCard(MakeElement(ElementId.Earth, "Earth", "Земля"));
            _hand.AddCard(MakeElement(ElementId.Lightning, "Lightning", "Молния"));
            _hand.AddCard(MakeElement(ElementId.Air, "Air", "Воздух"));
        }

        private static CardInstance MakeSpell(string en, string ru, ElementId affinity, int power, int sockets)
        {
            var c = CardInstance.CreateProcedural(CardType.Spell, CardRarity.Common, affinity, power, en, ru);
            c.ElementSockets = sockets;
            c.DescriptionEn = "Drag elements onto this spell.";
            c.DescriptionRu = "Перетащи элементы на это заклинание.";
            return c;
        }

        private static CardInstance MakeElement(ElementId id, string en, string ru)
        {
            return CardInstance.CreateProcedural(CardType.Element, CardRarity.Common, id, 0, en, ru);
        }

        private void OnFusion(CardInstance element, CardInstance spell, ComboResolveResult result)
        {
            if (_statusText == null)
                return;

            if (result.Success && result.Recipe != null)
            {
                string name = result.Recipe.GetResultName(_useRussianHints);
                string risk = result.RiskTriggered
                    ? (_useRussianHints ? " ⚠ РИСК!" : " ⚠ RISK!")
                    : string.Empty;
                _statusText.text = _useRussianHints
                    ? $"Комбо: {name}{risk}  |  сила {spell.GetFinalPower(result.AppliedPowerBonus, result.AppliedPowerMultiplier)}"
                    : $"Combo: {name}{risk}  |  power {spell.GetFinalPower(result.AppliedPowerBonus, result.AppliedPowerMultiplier)}";
            }
            else
            {
                _statusText.text = _useRussianHints
                    ? $"Влито: {element.ProvidedElement} → {spell.GetDisplayName(true)} ({spell.InfusedElements.Count}/{spell.ElementSockets})"
                    : $"Infused: {element.ProvidedElement} → {spell.GetDisplayName(false)} ({spell.InfusedElements.Count}/{spell.ElementSockets})";
            }
        }

        private void RefreshStatus()
        {
            if (_statusText == null || _hand == null)
                return;

            int spells = 0, elements = 0;
            for (int i = 0; i < _hand.Cards.Count; i++)
            {
                if (_hand.Cards[i].CardType == CardType.Spell) spells++;
                else if (_hand.Cards[i].CardType == CardType.Element) elements++;
            }

            if (_useRussianHints)
                _statusText.text = $"Рука: {_hand.Count}/{_hand.MaxHandSize}  |  Закл: {spells}  Элем: {elements}  |  Тяни элемент → на заклинание";
            else
                _statusText.text = $"Hand: {_hand.Count}/{_hand.MaxHandSize}  |  Spells: {spells}  Elements: {elements}  |  Drag element → spell";
        }

        // ── UI construction ───────────────────────────────────────────

        private static void EnsureEventSystem()
        {
            if (FindAnyObjectByType<EventSystem>() != null)
                return;

            // Project uses new Input System — prefer InputSystemUIInputModule for Android touch.
            // Проект на новом Input System — модуль для touch на Android.
            var esGo = new GameObject("EventSystem");
            esGo.AddComponent<EventSystem>();

#if ENABLE_INPUT_SYSTEM
            esGo.AddComponent<UnityEngine.InputSystem.UI.InputSystemUIInputModule>();
#else
            esGo.AddComponent<StandaloneInputModule>();
#endif
        }

        private static void EnsureOrientationService()
        {
            if (OrientationService.Instance != null)
                return;
            if (FindAnyObjectByType<OrientationService>() != null)
                return;

            var go = new GameObject("OrientationService");
            go.AddComponent<OrientationService>();
        }

        private static Canvas CreateCanvas()
        {
            var existing = FindAnyObjectByType<Canvas>();
            if (existing != null && existing.name == "KartovasiyaCanvas")
                return existing;

            var go = new GameObject("KartovasiyaCanvas", typeof(RectTransform), typeof(Canvas), typeof(CanvasScaler), typeof(GraphicRaycaster));
            var canvas = go.GetComponent<Canvas>();
            canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            canvas.sortingOrder = 10;

            var scaler = go.GetComponent<CanvasScaler>();
            scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
            scaler.referenceResolution = new Vector2(1080, 1920); // portrait mobile first
            scaler.matchWidthOrHeight = 0.5f;

            return canvas;
        }

        private void CreateEnemyZone(Transform canvas)
        {
            var go = new GameObject("EnemyZone", typeof(RectTransform), typeof(Image));
            go.transform.SetParent(canvas, false);
            var rect = go.GetComponent<RectTransform>();
            rect.anchorMin = new Vector2(0.1f, 0.62f);
            rect.anchorMax = new Vector2(0.9f, 0.92f);
            rect.offsetMin = Vector2.zero;
            rect.offsetMax = Vector2.zero;

            var img = go.GetComponent<Image>();
            img.color = new Color(0.15f, 0.08f, 0.12f, 0.85f);
            img.raycastTarget = false;

            var label = CreateUiText(go.transform, "EnemyLabel",
                _useRussianHints ? "ВРАГ (заглушка)\nблок 4 — Combat" : "ENEMY (stub)\nblock 4 — Combat",
                28, TextAnchor.MiddleCenter);
            StretchFull(label.rectTransform);
            label.color = new Color(0.9f, 0.5f, 0.55f);
        }

        private static RectTransform CreateHandZone(Transform canvas)
        {
            var go = new GameObject("HandZone", typeof(RectTransform), typeof(Image));
            go.transform.SetParent(canvas, false);
            var rect = go.GetComponent<RectTransform>();
            // Bottom band — vertical combat (StS-like)
            rect.anchorMin = new Vector2(0f, 0f);
            rect.anchorMax = new Vector2(1f, 0.42f);
            rect.offsetMin = Vector2.zero;
            rect.offsetMax = Vector2.zero;

            var img = go.GetComponent<Image>();
            img.color = new Color(0.06f, 0.06f, 0.1f, 0.55f);
            img.raycastTarget = false;
            return rect;
        }

        private Text CreateStatusBar(Transform canvas)
        {
            var go = new GameObject("StatusBar", typeof(RectTransform), typeof(Image));
            go.transform.SetParent(canvas, false);
            var rect = go.GetComponent<RectTransform>();
            rect.anchorMin = new Vector2(0f, 0.42f);
            rect.anchorMax = new Vector2(1f, 0.50f);
            rect.offsetMin = Vector2.zero;
            rect.offsetMax = Vector2.zero;
            go.GetComponent<Image>().color = new Color(0.05f, 0.05f, 0.08f, 0.9f);
            go.GetComponent<Image>().raycastTarget = false;

            var text = CreateUiText(go.transform, "StatusText", "", 20, TextAnchor.MiddleCenter);
            StretchFull(text.rectTransform);
            text.color = new Color(0.95f, 0.9f, 0.75f);
            return text;
        }

        private Text CreateHint(Transform canvas)
        {
            var go = new GameObject("Hint", typeof(RectTransform));
            go.transform.SetParent(canvas, false);
            var rect = go.GetComponent<RectTransform>();
            rect.anchorMin = new Vector2(0.05f, 0.50f);
            rect.anchorMax = new Vector2(0.95f, 0.60f);
            rect.offsetMin = Vector2.zero;
            rect.offsetMax = Vector2.zero;

            string msg = _useRussianHints
                ? "Попробуй: Огонь → заклинание, затем Вода → «Пар» (Steam)\nИли Воздух + Молния → Цепная молния"
                : "Try: Fire → spell, then Water → Steam\nOr Air + Lightning → Chain Lightning";

            var text = CreateUiText(go.transform, "HintText", msg, 18, TextAnchor.MiddleCenter);
            StretchFull(text.rectTransform);
            text.color = new Color(0.7f, 0.75f, 0.9f);
            return text;
        }

        private static Text CreateUiText(Transform parent, string name, string value, int size, TextAnchor anchor)
        {
            var go = new GameObject(name, typeof(RectTransform), typeof(Text));
            go.transform.SetParent(parent, false);
            var text = go.GetComponent<Text>();
            text.font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            if (text.font == null)
                text.font = Resources.GetBuiltinResource<Font>("Arial.ttf");
            text.text = value;
            text.fontSize = size;
            text.alignment = anchor;
            text.color = Color.white;
            text.horizontalOverflow = HorizontalWrapMode.Wrap;
            text.verticalOverflow = VerticalWrapMode.Overflow;
            text.raycastTarget = false;
            return text;
        }

        private static void StretchFull(RectTransform r)
        {
            r.anchorMin = Vector2.zero;
            r.anchorMax = Vector2.one;
            r.offsetMin = Vector2.zero;
            r.offsetMax = Vector2.zero;
        }
    }
}
