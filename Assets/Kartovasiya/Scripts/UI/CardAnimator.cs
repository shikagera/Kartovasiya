// Card motion feedback: hover, drag, fuse, cast
// Анимации карт: hover, drag, fuse, cast
// Chaos Spellweaver (Картовасия)

using System.Collections;
using UnityEngine;

namespace Kartovasiya.UI
{
    /// <summary>
    /// Lightweight coroutine animator (no DOTween). Mobile-friendly.
    /// Лёгкий аниматор на корутинах — без внешних зависимостей.
    /// </summary>
    public class CardAnimator : MonoBehaviour
    {
        [SerializeField] private float _hoverScale = 1.14f;
        [SerializeField] private float _dragScale = 1.2f;
        [SerializeField] private float _selectedScale = 2.75f;
        [SerializeField] private float _selectedLift = 200f;

        private RectTransform _rect;
        private Vector3 _baseScale = Vector3.one;
        private Quaternion _baseRot = Quaternion.identity;
        private Vector2 _basePos;
        private bool _baseCaptured;
        private Coroutine _punchRoutine;
        private Coroutine _moveRoutine;

        public bool IsAnimating { get; private set; }

        private void Awake()
        {
            _rect = transform as RectTransform;
        }

        public void CaptureBase()
        {
            if (_rect == null)
                _rect = transform as RectTransform;
            _baseScale = transform.localScale;
            _baseRot = transform.localRotation;
            _basePos = _rect != null ? _rect.anchoredPosition : Vector2.zero;
            _baseCaptured = true;
        }

        public void SetBaseFromLayout(Vector2 anchored, Quaternion rot, Vector3 scale)
        {
            _basePos = anchored;
            _baseRot = rot;
            _baseScale = scale;
            _baseCaptured = true;
        }

        public void PlayHover(bool on)
        {
            if (!_baseCaptured) CaptureBase();
            StopPunch();
            float target = on ? _hoverScale : 1f;
            transform.localScale = _baseScale * target;
        }

        public void PlayDrag(bool on)
        {
            if (!_baseCaptured) CaptureBase();
            // Keep readable while dragging
            transform.localScale = _baseScale * (on ? Mathf.Max(_dragScale, 1.35f) : 1f);
        }

        /// <summary>
        /// Large inspect mode so description is readable (mobile + desktop).
        /// Крупный просмотр с читаемым описанием.
        /// </summary>
        public void PlaySelected(bool on)
        {
            if (!_baseCaptured) CaptureBase();
            StopPunch();
            if (_rect == null)
                _rect = transform as RectTransform;

            if (on)
            {
                transform.localScale = _baseScale * _selectedScale;
                if (_rect != null)
                    _rect.anchoredPosition = _basePos + new Vector2(0f, _selectedLift);
                transform.localRotation = Quaternion.identity;
            }
            else
            {
                transform.localScale = _baseScale;
                if (_rect != null)
                    _rect.anchoredPosition = _basePos;
                transform.localRotation = _baseRot;
            }
        }

        /// <summary>Pulse when receiving an element fusion. / Пульс при fusion.</summary>
        public void PlayFusePulse(Color flashColor)
        {
            if (_punchRoutine != null)
                StopCoroutine(_punchRoutine);
            _punchRoutine = StartCoroutine(FusePulseRoutine(flashColor));
        }

        /// <summary>Shake reject. / Тряска при отказе drop.</summary>
        public void PlayReject()
        {
            if (_punchRoutine != null)
                StopCoroutine(_punchRoutine);
            _punchRoutine = StartCoroutine(RejectRoutine());
        }

        /// <summary>Fly toward world/UI point then destroy callback. / Полёт к цели.</summary>
        public void PlayCastFly(Vector3 worldTarget, float duration, System.Action onComplete)
        {
            if (_moveRoutine != null)
                StopCoroutine(_moveRoutine);
            _moveRoutine = StartCoroutine(CastFlyRoutine(worldTarget, duration, onComplete));
        }

        public void PlayDealIn(float delay)
        {
            StartCoroutine(DealInRoutine(delay));
        }

        private IEnumerator FusePulseRoutine(Color flash)
        {
            IsAnimating = true;
            if (!_baseCaptured) CaptureBase();

            var images = GetComponentsInChildren<UnityEngine.UI.Image>(true);
            var original = new Color[images.Length];
            for (int i = 0; i < images.Length; i++)
                original[i] = images[i].color;

            float t = 0f;
            const float dur = 0.35f;
            while (t < dur)
            {
                t += Time.unscaledDeltaTime;
                float u = t / dur;
                float pulse = 1f + Mathf.Sin(u * Mathf.PI) * 0.14f;
                transform.localScale = _baseScale * pulse;

                float flashA = Mathf.Sin(u * Mathf.PI) * 0.55f;
                for (int i = 0; i < images.Length; i++)
                {
                    if (images[i] == null) continue;
                    images[i].color = Color.Lerp(original[i], flash, flashA);
                }

                yield return null;
            }

            for (int i = 0; i < images.Length; i++)
            {
                if (images[i] != null)
                    images[i].color = original[i];
            }

            transform.localScale = _baseScale;
            IsAnimating = false;
            _punchRoutine = null;
        }

        private IEnumerator RejectRoutine()
        {
            IsAnimating = true;
            if (!_baseCaptured) CaptureBase();
            float t = 0f;
            const float dur = 0.28f;
            while (t < dur)
            {
                t += Time.unscaledDeltaTime;
                float u = t / dur;
                float shake = Mathf.Sin(u * Mathf.PI * 6f) * (1f - u) * 12f;
                if (_rect != null)
                    _rect.anchoredPosition = _basePos + new Vector2(shake, 0f);
                yield return null;
            }

            if (_rect != null)
                _rect.anchoredPosition = _basePos;
            IsAnimating = false;
            _punchRoutine = null;
        }

        private IEnumerator CastFlyRoutine(Vector3 worldTarget, float duration, System.Action onComplete)
        {
            IsAnimating = true;
            Vector3 start = transform.position;
            Vector3 startScale = transform.localScale;
            float t = 0f;
            duration = Mathf.Max(0.05f, duration);

            while (t < duration)
            {
                t += Time.unscaledDeltaTime;
                float u = Mathf.SmoothStep(0f, 1f, t / duration);
                transform.position = Vector3.Lerp(start, worldTarget, u);
                transform.localScale = Vector3.Lerp(startScale, startScale * 0.35f, u);
                transform.localRotation = Quaternion.Euler(0f, 0f, Mathf.Lerp(0f, 25f, u));
                yield return null;
            }

            IsAnimating = false;
            _moveRoutine = null;
            onComplete?.Invoke();
        }

        private IEnumerator DealInRoutine(float delay)
        {
            if (delay > 0f)
                yield return new WaitForSecondsRealtime(delay);

            if (!_baseCaptured) CaptureBase();
            transform.localScale = Vector3.zero;
            float t = 0f;
            const float dur = 0.25f;
            while (t < dur)
            {
                t += Time.unscaledDeltaTime;
                float u = Mathf.SmoothStep(0f, 1f, t / dur);
                transform.localScale = Vector3.Lerp(Vector3.zero, _baseScale, u);
                yield return null;
            }

            transform.localScale = _baseScale;
        }

        private void StopPunch()
        {
            if (_punchRoutine != null)
            {
                StopCoroutine(_punchRoutine);
                _punchRoutine = null;
            }
        }
    }
}
