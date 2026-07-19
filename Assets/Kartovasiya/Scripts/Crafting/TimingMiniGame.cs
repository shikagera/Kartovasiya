// Timing bar mini-game (craft / event / boss bonus)
// Мини-игра на тайминг (крафт / событие / босс)
// Chaos Spellweaver (Картовасия)

using System;
using System.Collections;
using UnityEngine;
using UnityEngine.UI;
using Kartovasiya.UI;

namespace Kartovasiya.Crafting
{
    public enum MiniGameContext
    {
        Craft = 0,
        GlobalEvent = 1,
        BossBonus = 2
    }

    /// <summary>
    /// Sliding marker over a green success zone. Tap / click to stop.
    /// Ползунок по зелёной зоне. Тап — стоп. Skill 0..1 by accuracy.
    /// </summary>
    public class TimingMiniGame : MonoBehaviour
    {
        public static TimingMiniGame Instance { get; private set; }

        [SerializeField] private float _speed = 1.35f;
        [SerializeField] private float _greenHalfWidth = 0.12f; // of 0..1 track

        private RectTransform _root;
        private Image _track;
        private Image _green;
        private Image _marker;
        private Text _title;
        private Text _hint;
        private Button _tapButton;
        private bool _running;
        private float _t; // 0..1 ping-pong
        private int _dir = 1;
        private Action<float> _onComplete; // skill 0..1
        private CanvasGroup _cg;

        private void Awake()
        {
            Instance = this;
        }

        private void OnDestroy()
        {
            if (Instance == this)
                Instance = null;
        }

        public void Build(Transform canvasParent)
        {
            var go = new GameObject("TimingMiniGame", typeof(RectTransform), typeof(CanvasGroup), typeof(Image));
            go.transform.SetParent(canvasParent, false);
            _root = go.GetComponent<RectTransform>();
            UiFactory.Stretch(_root);
            _cg = go.GetComponent<CanvasGroup>();
            var dim = go.GetComponent<Image>();
            dim.color = new Color(0f, 0f, 0f, 0.72f);
            dim.raycastTarget = true;

            var panel = new GameObject("Panel", typeof(RectTransform), typeof(Image));
            panel.transform.SetParent(go.transform, false);
            var pr = panel.GetComponent<RectTransform>();
            pr.anchorMin = new Vector2(0.1f, 0.35f);
            pr.anchorMax = new Vector2(0.9f, 0.65f);
            pr.offsetMin = Vector2.zero;
            pr.offsetMax = Vector2.zero;
            var pimg = panel.GetComponent<Image>();
            pimg.sprite = ProceduralSpriteFactory.RoundedRect(128, 64, 10, TarotPalette.VoidPurple, TarotPalette.Gold, 3);
            pimg.color = Color.white;
            pimg.raycastTarget = true;

            _title = UiFactory.Text(panel.transform, "Title", "Мини-игра", 26, TextAnchor.UpperCenter, TarotPalette.GoldBright, FontStyle.Bold);
            UiFactory.Anchor(_title.rectTransform, new Vector2(0.05f, 0.7f), new Vector2(0.95f, 0.95f));

            _hint = UiFactory.Text(panel.transform, "Hint", "Тапни в зелёной зоне!", 16, TextAnchor.MiddleCenter, TarotPalette.Parchment);
            UiFactory.Anchor(_hint.rectTransform, new Vector2(0.05f, 0.52f), new Vector2(0.95f, 0.7f));

            // Track
            var trackGo = new GameObject("Track", typeof(RectTransform), typeof(Image));
            trackGo.transform.SetParent(panel.transform, false);
            _track = trackGo.GetComponent<Image>();
            _track.color = new Color(0.15f, 0.12f, 0.18f, 1f);
            var tr = _track.rectTransform;
            tr.anchorMin = new Vector2(0.08f, 0.28f);
            tr.anchorMax = new Vector2(0.92f, 0.42f);
            tr.offsetMin = Vector2.zero;
            tr.offsetMax = Vector2.zero;

            // Green zone (centered, width changes by difficulty later)
            var greenGo = new GameObject("Green", typeof(RectTransform), typeof(Image));
            greenGo.transform.SetParent(trackGo.transform, false);
            _green = greenGo.GetComponent<Image>();
            _green.color = new Color(0.2f, 0.75f, 0.35f, 0.85f);
            var gr = _green.rectTransform;
            gr.anchorMin = new Vector2(0.5f - _greenHalfWidth, 0f);
            gr.anchorMax = new Vector2(0.5f + _greenHalfWidth, 1f);
            gr.offsetMin = Vector2.zero;
            gr.offsetMax = Vector2.zero;

            // Marker
            var markGo = new GameObject("Marker", typeof(RectTransform), typeof(Image));
            markGo.transform.SetParent(trackGo.transform, false);
            _marker = markGo.GetComponent<Image>();
            _marker.color = TarotPalette.GoldBright;
            var mr = _marker.rectTransform;
            mr.anchorMin = new Vector2(0f, -0.15f);
            mr.anchorMax = new Vector2(0f, 1.15f);
            mr.pivot = new Vector2(0.5f, 0.5f);
            mr.sizeDelta = new Vector2(10f, 0f);
            mr.anchorMin = new Vector2(0f, 0f);
            mr.anchorMax = new Vector2(0f, 1f);
            mr.anchoredPosition = Vector2.zero;

            // Full-panel tap
            _tapButton = panel.AddComponent<Button>();
            _tapButton.targetGraphic = pimg;
            _tapButton.onClick.AddListener(OnTap);

            Hide();
        }

        public void Play(MiniGameContext ctx, bool russian, Action<float> onComplete, float greenHalf = -1f)
        {
            if (_root == null)
                return;

            if (greenHalf > 0f)
                _greenHalfWidth = Mathf.Clamp(greenHalf, 0.06f, 0.25f);

            // Update green size
            if (_green != null)
            {
                var gr = _green.rectTransform;
                gr.anchorMin = new Vector2(0.5f - _greenHalfWidth, 0f);
                gr.anchorMax = new Vector2(0.5f + _greenHalfWidth, 1f);
            }

            _onComplete = onComplete;
            _t = 0f;
            _dir = 1;
            _running = true;
            _root.gameObject.SetActive(true);
            _root.SetAsLastSibling();
            _cg.alpha = 1f;
            _cg.blocksRaycasts = true;

            string title = ctx switch
            {
                MiniGameContext.Craft => russian ? "КУЗНИЦА ХАОСА" : "CHAOS FORGE",
                MiniGameContext.GlobalEvent => russian ? "ВОЛЯ СУДЬБЫ" : "FATE'S WHIM",
                MiniGameContext.BossBonus => russian ? "ДОБЫЧА БОССА" : "BOSS LOOT",
                _ => "MINI-GAME"
            };
            if (_title != null) _title.text = title;
            if (_hint != null)
                _hint.text = russian
                    ? "Тапни, когда ползунок в ЗЕЛЁНОЙ зоне"
                    : "Tap when the marker is in the GREEN zone";

            ProceduralSfx.Instance?.PlayUi();
            StopAllCoroutines();
            StartCoroutine(RunMarker());
        }

        public void Hide()
        {
            _running = false;
            if (_root != null)
            {
                _root.gameObject.SetActive(false);
                _cg.blocksRaycasts = false;
            }
        }

        private IEnumerator RunMarker()
        {
            while (_running)
            {
                _t += Time.unscaledDeltaTime * _speed * _dir;
                if (_t >= 1f) { _t = 1f; _dir = -1; }
                if (_t <= 0f) { _t = 0f; _dir = 1; }
                SetMarker(_t);
                yield return null;
            }
        }

        private void SetMarker(float t01)
        {
            if (_marker == null) return;
            var rt = _marker.rectTransform;
            // position along track using anchors
            rt.anchorMin = new Vector2(t01, 0f);
            rt.anchorMax = new Vector2(t01, 1f);
            rt.anchoredPosition = Vector2.zero;
            rt.sizeDelta = new Vector2(12f, 8f);
        }

        private void OnTap()
        {
            if (!_running)
                return;

            _running = false;
            StopAllCoroutines();

            // Distance from center (0.5). Green if within half width.
            float dist = Mathf.Abs(_t - 0.5f);
            float skill;
            if (dist <= _greenHalfWidth)
            {
                // 1 at center, ~0.5 at edge of green
                skill = 1f - (dist / _greenHalfWidth) * 0.5f;
                ProceduralSfx.Instance?.PlayCombo();
                CombatVfxService.Instance?.PlayScreenFlash(TarotPalette.Gold, 0.25f, 0.2f);
            }
            else
            {
                // Outside: poor skill, still some if close
                skill = Mathf.Clamp01(0.25f - (dist - _greenHalfWidth));
                ProceduralSfx.Instance?.PlayDropFail();
            }

            var cb = _onComplete;
            _onComplete = null;
            Hide();
            cb?.Invoke(Mathf.Clamp01(skill));
        }
    }
}
