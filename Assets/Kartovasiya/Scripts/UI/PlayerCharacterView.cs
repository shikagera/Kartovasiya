// Player 2D aristocrat avatar in combat (idle + cast gesture)
// 2D-моделька игрока: готический аристократ
// Chaos Spellweaver (Картовасия)

using System.Collections;
using UnityEngine;
using UnityEngine.UI;

namespace Kartovasiya.UI
{
    /// <summary>
    /// Shows the player character on the combat HUD and plays a cast pose when spells fly.
    /// </summary>
    public class PlayerCharacterView : MonoBehaviour
    {
        public static PlayerCharacterView Instance { get; private set; }

        [SerializeField] private Image _portrait;
        [SerializeField] private RectTransform _root;
        [SerializeField] private float _idleBobAmp = 5f;
        [SerializeField] private float _idleBobSpeed = 0.9f;

        private Sprite _idle;
        private Sprite _cast;
        private Vector2 _basePos;
        private bool _casting;
        private Coroutine _castRoutine;
        private Coroutine _bobRoutine;

        /// <summary>World position for spell projectiles (near the casting hand).</summary>
        public Vector3 CastOrigin
        {
            get
            {
                if (_root == null) return transform.position;
                // Offset toward upper-right of figure (hand / cane area)
                var corners = new Vector3[4];
                _root.GetWorldCorners(corners);
                // corners: 0=bl, 1=tl, 2=tr, 3=br
                return Vector3.Lerp(corners[1], corners[2], 0.72f)
                       + (corners[1] - corners[0]) * 0.35f;
            }
        }

        private void Awake()
        {
            Instance = this;
        }

        private void OnDestroy()
        {
            if (Instance == this)
                Instance = null;
        }

        public void Bind(Image portrait, RectTransform root)
        {
            _portrait = portrait;
            _root = root;
            if (_root != null)
                _basePos = _root.anchoredPosition;

            LoadSprites();
            ApplyIdle();
            if (_bobRoutine != null)
                StopCoroutine(_bobRoutine);
            _bobRoutine = StartCoroutine(IdleBob());
        }

        private void LoadSprites()
        {
            // Prefer Resources path if present; else load via Resources.Load from Art is not automatic.
            // We assign sprites from Build in GameApp using AssetDatabase-less Resources or Texture2D load.
            // At runtime in builds, put under Resources/Characters OR assign via Sprite.Create from Texture2D.
            _idle = Resources.Load<Sprite>("Characters/Player_Idle");
            _cast = Resources.Load<Sprite>("Characters/Player_Cast");

            // Fallback: try Resources textures
            if (_idle == null)
            {
                var t = Resources.Load<Texture2D>("Characters/Player_Idle");
                if (t != null)
                    _idle = Sprite.Create(t, new Rect(0, 0, t.width, t.height), new Vector2(0.5f, 0f), 100f);
            }

            if (_cast == null)
            {
                var t = Resources.Load<Texture2D>("Characters/Player_Cast");
                if (t != null)
                    _cast = Sprite.Create(t, new Rect(0, 0, t.width, t.height), new Vector2(0.5f, 0f), 100f);
            }
        }

        /// <summary>Inject sprites from code (GameApp) when not using Resources.</summary>
        public void SetSprites(Sprite idle, Sprite cast)
        {
            if (idle != null) _idle = idle;
            if (cast != null) _cast = cast;
            ApplyIdle();
        }

        public void ApplyIdle()
        {
            if (_portrait == null) return;
            if (_idle != null)
            {
                _portrait.sprite = _idle;
                _portrait.color = Color.white;
                _portrait.preserveAspect = true;
            }
        }

        /// <summary>Elegant cast gesture, then return to idle.</summary>
        public void PlayCastGesture(float duration = 0.55f)
        {
            if (!isActiveAndEnabled) return;
            if (_castRoutine != null)
                StopCoroutine(_castRoutine);
            _castRoutine = StartCoroutine(CastRoutine(duration));
        }

        private IEnumerator CastRoutine(float duration)
        {
            _casting = true;
            if (_portrait != null && _cast != null)
                _portrait.sprite = _cast;

            float t = 0f;
            // Lean forward + slight raise
            while (t < duration * 0.45f)
            {
                t += Time.unscaledDeltaTime;
                float u = Mathf.Clamp01(t / (duration * 0.45f));
                float ease = Mathf.SmoothStep(0f, 1f, u);
                if (_root != null)
                {
                    _root.anchoredPosition = _basePos + new Vector2(12f * ease, 10f * ease);
                    _root.localRotation = Quaternion.Euler(0f, 0f, -4f * ease);
                    _root.localScale = Vector3.one * (1f + 0.04f * ease);
                }

                yield return null;
            }

            // Hold cast pose briefly
            float hold = 0f;
            while (hold < duration * 0.25f)
            {
                hold += Time.unscaledDeltaTime;
                yield return null;
            }

            // Return to idle
            t = 0f;
            float ret = duration * 0.3f;
            while (t < ret)
            {
                t += Time.unscaledDeltaTime;
                float u = Mathf.SmoothStep(0f, 1f, t / ret);
                if (_root != null)
                {
                    _root.anchoredPosition = Vector2.Lerp(_basePos + new Vector2(12f, 10f), _basePos, u);
                    _root.localRotation = Quaternion.Euler(0f, 0f, Mathf.Lerp(-4f, 0f, u));
                    _root.localScale = Vector3.one * Mathf.Lerp(1.04f, 1f, u);
                }

                yield return null;
            }

            if (_root != null)
            {
                _root.anchoredPosition = _basePos;
                _root.localRotation = Quaternion.identity;
                _root.localScale = Vector3.one;
            }

            ApplyIdle();
            _casting = false;
            _castRoutine = null;
        }

        private IEnumerator IdleBob()
        {
            float t = 0f;
            while (true)
            {
                if (!_casting && _root != null)
                {
                    t += Time.unscaledDeltaTime * _idleBobSpeed;
                    float y = Mathf.Sin(t) * _idleBobAmp;
                    // Melancholy sway
                    float x = Mathf.Sin(t * 0.5f) * 1.5f;
                    _root.anchoredPosition = _basePos + new Vector2(x, y);
                }

                yield return null;
            }
        }

        /// <summary>
        /// Large bottom-center aristocrat — main player visual anchor.
        /// </summary>
        public static PlayerCharacterView Build(Transform hudRoot)
        {
            var go = new GameObject("PlayerCharacter", typeof(RectTransform), typeof(PlayerCharacterView));
            go.transform.SetParent(hudRoot, false);
            var rt = go.GetComponent<RectTransform>();
            // Bottom-center, large — sits above hand fan, on magic circle
            rt.anchorMin = new Vector2(0.18f, 0.14f);
            rt.anchorMax = new Vector2(0.82f, 0.48f);
            rt.offsetMin = Vector2.zero;
            rt.offsetMax = Vector2.zero;
            rt.pivot = new Vector2(0.5f, 0f);

            // Soft pedestal / magic glow under feet
            var shadow = UiFactory.Image(go.transform, "Shadow",
                ProceduralSpriteFactory.SoftGlow(64, Color.black),
                new Color(0.15f, 0.05f, 0.2f, 0.65f), false);
            UiFactory.Anchor(shadow.rectTransform, new Vector2(0.12f, 0f), new Vector2(0.88f, 0.22f));

            var ring = UiFactory.Image(go.transform, "Ring",
                ProceduralSpriteFactory.Disc(128, new Color(0.2f, 0.1f, 0.05f, 0.01f), TarotPalette.Gold, 3),
                new Color(1f, 1f, 1f, 0.55f), false);
            UiFactory.Anchor(ring.rectTransform, new Vector2(0.22f, 0.02f), new Vector2(0.78f, 0.20f));
            ring.raycastTarget = false;

            var img = UiFactory.Image(go.transform, "Portrait",
                ProceduralSpriteFactory.RoundedRect(64, 96, 8, TarotPalette.Night, TarotPalette.Gold, 2),
                Color.white, false);
            UiFactory.Anchor(img.rectTransform, new Vector2(0.08f, 0.08f), new Vector2(0.92f, 1f));
            img.preserveAspect = true;
            img.raycastTarget = false;

            // Name plate at feet
            var plate = UiFactory.Image(go.transform, "NamePlate",
                ProceduralSpriteFactory.RoundedRect(64, 24, 6, TarotPalette.VoidPurple, TarotPalette.Gold, 2),
                Color.white, false);
            UiFactory.Anchor(plate.rectTransform, new Vector2(0.22f, 0.0f), new Vector2(0.78f, 0.10f));
            var name = UiFactory.Text(go.transform, "Name", "…", 14,
                TextAnchor.MiddleCenter, TarotPalette.Parchment, FontStyle.Bold);
            UiFactory.Anchor(name.rectTransform, new Vector2(0.24f, 0.0f), new Vector2(0.76f, 0.10f));
            name.resizeTextForBestFit = true;
            name.resizeTextMinSize = 10;
            name.resizeTextMaxSize = 15;
            name.raycastTarget = false;

            var view = go.GetComponent<PlayerCharacterView>();
            view.Bind(img, rt);
            return view;
        }
    }
}
