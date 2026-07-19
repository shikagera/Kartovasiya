// Portrait / landscape layout service / Сервис ориентации экрана
// Chaos Spellweaver (Картовасия)

using System;
using UnityEngine;

namespace Kartovasiya.Core
{
    /// <summary>
    /// Detects screen orientation changes for UI reflow (portrait & landscape).
    /// Отслеживает смену ориентации для перестройки UI.
    /// Attach to a bootstrap GameObject or call from UI root.
    /// </summary>
    public sealed class OrientationService : MonoBehaviour
    {
        public static OrientationService Instance { get; private set; }

        [SerializeField] private float _pollInterval = 0.25f;

        public ScreenLayoutMode Current { get; private set; }

        /// <summary>Fired when layout mode changes. / Событие смены portrait/landscape.</summary>
        public event Action<ScreenLayoutMode> LayoutChanged;

        private float _timer;
        private ScreenOrientation _lastDeviceOrientation;

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }

            Instance = this;
            DontDestroyOnLoad(gameObject);
            Current = Detect();
            _lastDeviceOrientation = Screen.orientation;
        }

        private void OnDestroy()
        {
            if (Instance == this)
                Instance = null;
        }

        private void Update()
        {
            _timer += Time.unscaledDeltaTime;
            if (_timer < _pollInterval)
                return;

            _timer = 0f;
            RefreshIfNeeded();
        }

        public void RefreshIfNeeded()
        {
            var next = Detect();
            if (next == Current && Screen.orientation == _lastDeviceOrientation)
                return;

            Current = next;
            _lastDeviceOrientation = Screen.orientation;
            LayoutChanged?.Invoke(Current);
        }

        /// <summary>
        /// Aspect-based detection is more reliable on Android multi-window.
        /// Определение по aspect ratio надёжнее на Android.
        /// </summary>
        public static ScreenLayoutMode Detect()
        {
            // Safe for edit-mode tests / safe area independent
            float w = Screen.width;
            float h = Screen.height;
            if (w <= 0f || h <= 0f)
                return ScreenLayoutMode.Portrait;

            return w > h ? ScreenLayoutMode.Landscape : ScreenLayoutMode.Portrait;
        }
    }
}
