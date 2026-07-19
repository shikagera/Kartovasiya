// First-run tutorial overlay / Оверлей туториала
// Chaos Spellweaver (Картовасия)

using System;
using UnityEngine;
using UnityEngine.UI;
using Kartovasiya.Core;

namespace Kartovasiya.UI
{
    /// <summary>
    /// Multi-step onboarding shown once per device (PlayerPrefs).
    /// Пошаговый туториал один раз на устройство.
    /// </summary>
    public class TutorialOverlay : MonoBehaviour
    {
        public const string PrefsKey = "kartovasiya_tutorial_done_v1";

        public static TutorialOverlay Instance { get; private set; }

        private GameObject _root;
        private Text _title;
        private Text _body;
        private Text _stepLabel;
        private int _step;
        private bool _russian = true;
        private Action _onFinished;

        private static readonly string[] TitlesRu =
        {
            "Добро пожаловать",
            "Комбо стихий",
            "Атака",
            "Кузня",
            "События и боссы"
        };

        private static readonly string[] BodiesRu =
        {
            "Ты — хаотичный маг.\nНет маны — только лимит карт за ход.\nЦель: выживать в бесконечных волнах.",
            "Перетащи ЭЛЕМЕНТ на ЗАКЛИНАНИЕ.\nОгонь + Вода = Пар (Steam).\nЧем безумнее комбо — тем сильнее удар.",
            "Перетащи ЗАКЛИНАНИЕ на врага сверху.\nКаждая сыгранная карта тратит лимит хода.\nКнопка «Ход →» — ход врага.",
            "«Кузня» улучшает карты.\nСобери материалы с врагов и боссов.\nМини-игра: тап в зелёной зоне (шанс до 70%).",
            "Каждые 4 хода — глобальное событие.\nКаждая 5-я волна — БОСС.\nМожно выйти в любой момент — run сохранится.\n\nУдачи, ткач хаоса!"
        };

        private static readonly string[] TitlesEn =
        {
            "Welcome",
            "Element combos",
            "Attack",
            "Forge",
            "Events & bosses"
        };

        private static readonly string[] BodiesEn =
        {
            "You are a chaos mage.\nNo mana — only cards-per-turn.\nSurvive endless waves.",
            "Drag an ELEMENT onto a SPELL.\nFire + Water = Steam.\nWilder combos hit harder.",
            "Drag a SPELL onto the enemy above.\nEach played card spends your turn limit.\n«End» lets the enemy act.",
            "«Forge» upgrades cards.\nLoot materials from enemies and bosses.\nMini-game: tap the green zone (up to 70% success).",
            "Every 4 turns — a global event.\nEvery 5th wave — a BOSS.\nExit anytime — your run is saved.\n\nGood luck, spellweaver!"
        };

        private void Awake() => Instance = this;

        private void OnDestroy()
        {
            if (Instance == this) Instance = null;
        }

        public static bool IsCompleted => PlayerPrefs.GetInt(PrefsKey, 0) == 1;

        public static void ResetTutorial()
        {
            PlayerPrefs.DeleteKey(PrefsKey);
            PlayerPrefs.Save();
        }

        public void Build(Transform canvas)
        {
            _root = ModalPanelFactory.CreateDimRoot(canvas, "TutorialOverlay", 0.82f);
            var (panel, title, body) = ModalPanelFactory.CreateCardPanel(
                _root.transform, "TutPanel",
                new Vector2(0.08f, 0.22f), new Vector2(0.92f, 0.78f));
            _title = title;
            _body = body;

            _stepLabel = UiFactory.Text(panel.transform, "Step", "1/5", 14, TextAnchor.UpperRight, TarotPalette.Gold);
            UiFactory.Anchor(_stepLabel.rectTransform, new Vector2(0.7f, 0.88f), new Vector2(0.95f, 0.96f));

            ModalPanelFactory.CreateButton(panel.transform, "BtnSkip",
                "Skip",
                new Vector2(0.08f, 0.06f), new Vector2(0.36f, 0.16f),
                new Color(0.35f, 0.2f, 0.22f), Skip);

            ModalPanelFactory.CreateButton(panel.transform, "BtnNext",
                "Next →",
                new Vector2(0.4f, 0.06f), new Vector2(0.92f, 0.16f),
                new Color(0.3f, 0.18f, 0.45f), Next);
        }

        /// <summary>
        /// Show if not completed. onFinished always called (even if skipped/already done).
        /// </summary>
        public void TryShow(bool russian, Action onFinished)
        {
            _russian = russian;
            _onFinished = onFinished;

            if (IsCompleted)
            {
                Hide();
                onFinished?.Invoke();
                return;
            }

            _step = 0;
            RefreshStep();
            if (_root != null)
            {
                _root.SetActive(true);
                _root.transform.SetAsLastSibling();
            }
        }

        public void ForceShow(bool russian, Action onFinished = null)
        {
            _russian = russian;
            _onFinished = onFinished;
            _step = 0;
            RefreshStep();
            if (_root != null)
            {
                _root.SetActive(true);
                _root.transform.SetAsLastSibling();
            }
        }

        private void RefreshStep()
        {
            int n = TitlesRu.Length;
            _step = Mathf.Clamp(_step, 0, n - 1);

            if (_stepLabel != null)
                _stepLabel.text = $"{_step + 1}/{n}";

            if (_title != null)
                _title.text = _russian ? TitlesRu[_step] : TitlesEn[_step];
            if (_body != null)
                _body.text = _russian ? BodiesRu[_step] : BodiesEn[_step];

            // Localize buttons if present
            var next = _root != null ? _root.transform.Find("TutPanel/BtnNext/L") : null;
            var skip = _root != null ? _root.transform.Find("TutPanel/BtnSkip/L") : null;
            if (next != null)
            {
                var t = next.GetComponent<Text>();
                if (t != null)
                {
                    bool last = _step >= n - 1;
                    t.text = last
                        ? (_russian ? "В бой!" : "Play!")
                        : (_russian ? "Далее →" : "Next →");
                }
            }

            if (skip != null)
            {
                var t = skip.GetComponent<Text>();
                if (t != null)
                    t.text = _russian ? "Пропуск" : "Skip";
            }
        }

        private void Next()
        {
            ProceduralSfx.Instance?.PlayUi();
            if (_step < TitlesRu.Length - 1)
            {
                _step++;
                RefreshStep();
            }
            else
            {
                Complete();
            }
        }

        private void Skip()
        {
            ProceduralSfx.Instance?.PlayUi();
            Complete();
        }

        private void Complete()
        {
            PlayerPrefs.SetInt(PrefsKey, 1);
            PlayerPrefs.Save();
            Hide();
            var cb = _onFinished;
            _onFinished = null;
            cb?.Invoke();
        }

        private void Hide()
        {
            if (_root != null)
                _root.SetActive(false);
        }
    }
}
