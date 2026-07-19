// Combo / cast VFX overlays + elemental projectiles
// VFX комбо, кастов и снарядов стихий
// Chaos Spellweaver (Картовасия)

using System.Collections;
using UnityEngine;
using UnityEngine.UI;
using Kartovasiya.Core;

namespace Kartovasiya.UI
{
    /// <summary>
    /// Screen-space VFX: flashes, floating text, elemental spell projectiles.
    /// </summary>
    public class CombatVfxService : MonoBehaviour
    {
        public static CombatVfxService Instance { get; private set; }

        [SerializeField] private Canvas _canvas;
        [SerializeField] private RectTransform _overlayRoot;

        private Image _fullFlash;

        private void Awake()
        {
            Instance = this;
        }

        private void OnDestroy()
        {
            if (Instance == this)
                Instance = null;
        }

        public void Configure(Canvas canvas)
        {
            _canvas = canvas;
            if (_overlayRoot == null)
            {
                var go = new GameObject("VfxOverlay", typeof(RectTransform));
                go.transform.SetParent(canvas.transform, false);
                _overlayRoot = go.GetComponent<RectTransform>();
                UiFactory.Stretch(_overlayRoot);
                go.transform.SetAsLastSibling();
            }

            if (_fullFlash == null)
            {
                var flashGo = new GameObject("FullFlash", typeof(RectTransform), typeof(Image));
                flashGo.transform.SetParent(_overlayRoot, false);
                _fullFlash = flashGo.GetComponent<Image>();
                _fullFlash.sprite = ProceduralSpriteFactory.SoftGlow(64, Color.white);
                _fullFlash.color = new Color(1f, 1f, 1f, 0f);
                _fullFlash.raycastTarget = false;
                UiFactory.Stretch(_fullFlash.rectTransform);
            }
        }

        public void PlayComboBurst(Vector3 worldPos, Color tint, string title, bool risk)
        {
            if (_overlayRoot == null)
                return;

            StartCoroutine(ComboRoutine(worldPos, tint, title, risk));
            ProceduralSfx.Instance?.PlayCombo();
        }

        public void PlayFuseSpark(Vector3 worldPos, Color tint)
        {
            SpawnGlow(worldPos, tint, 80f, 0.35f);
            ProceduralSfx.Instance?.PlayFuse();
        }

        public void PlayCastImpact(Vector3 worldPos, Color tint, int damage)
        {
            StartCoroutine(ImpactRoutine(worldPos, tint, damage));
            StartCoroutine(ScreenShakeRoutine(0.18f, 12f));
            ProceduralSfx.Instance?.PlayCast();
            ProceduralSfx.Instance?.PlayHit();
        }

        /// <summary>
        /// Elemental projectile from player toward enemy (fireball, bolt, ice shard…).
        /// Снаряд стихии: от игрока к боссу. onArrived — когда попал.
        /// </summary>
        public void PlaySpellProjectile(
            Vector3 fromWorld,
            Vector3 toWorld,
            ElementId element,
            Color tint,
            System.Action onArrived,
            float duration = 0.48f)
        {
            if (_overlayRoot == null)
            {
                onArrived?.Invoke();
                return;
            }

            // Bring overlay on top so projectile is visible
            _overlayRoot.SetAsLastSibling();
            StartCoroutine(ProjectileRoutine(fromWorld, toWorld, element, tint, onArrived, duration));
            // Launch whoosh; impact SFX plays on hit via PlayCastImpact
            ProceduralSfx.Instance?.PlayFuse();
        }

        /// <summary>Golden aura when a buff lands on the aristocrat.</summary>
        public void PlaySelfBuff(Vector3 playerWorld, Color tint)
        {
            if (_overlayRoot == null) return;
            _overlayRoot.SetAsLastSibling();
            StartCoroutine(SelfBuffRoutine(playerWorld, tint));
        }

        private IEnumerator SelfBuffRoutine(Vector3 pos, Color tint)
        {
            PlayScreenFlash(tint, 0.28f, 0.35f);
            SpawnGlow(pos, tint, 160f, 0.55f);
            SpawnGlow(pos, Color.white, 70f, 0.35f);
            for (int i = 0; i < 10; i++)
            {
                float ang = i / 10f * Mathf.PI * 2f;
                var off = new Vector3(Mathf.Cos(ang), Mathf.Sin(ang), 0f) * 55f;
                SpawnGlow(pos + off, tint, 40f, 0.4f);
            }

            FloatText(pos + Vector3.up * 40f, "✦", tint, 40);
            yield return null;
        }

        /// <summary>Light UI camera-less shake via overlay root. / Лёгкая тряска UI.</summary>
        public void PlayScreenShake(float duration = 0.2f, float magnitude = 14f)
        {
            if (_overlayRoot == null) return;
            StartCoroutine(ScreenShakeRoutine(duration, magnitude));
        }

        private IEnumerator ScreenShakeRoutine(float duration, float magnitude)
        {
            if (_overlayRoot == null || _canvas == null)
                yield break;

            var root = _canvas.transform as RectTransform;
            if (root == null) yield break;

            Vector2 origin = root.anchoredPosition;
            float t = 0f;
            while (t < duration)
            {
                t += Time.unscaledDeltaTime;
                float damp = 1f - t / duration;
                root.anchoredPosition = origin + Random.insideUnitCircle * (magnitude * damp);
                yield return null;
            }

            root.anchoredPosition = origin;
        }

        public void PlayScreenFlash(Color color, float peakAlpha = 0.35f, float duration = 0.25f)
        {
            if (_fullFlash == null)
                return;
            StartCoroutine(FlashRoutine(color, peakAlpha, duration));
        }

        public void FloatText(Vector3 worldPos, string text, Color color, int fontSize = 28)
        {
            StartCoroutine(FloatTextRoutine(worldPos, text, color, fontSize));
        }

        private IEnumerator ComboRoutine(Vector3 worldPos, Color tint, string title, bool risk)
        {
            PlayScreenFlash(risk ? TarotPalette.Risk : tint, risk ? 0.4f : 0.28f, 0.3f);
            SpawnGlow(worldPos, tint, 140f, 0.45f);
            SpawnGlow(worldPos, TarotPalette.GoldBright, 90f, 0.35f);

            // Ring of sparks
            for (int i = 0; i < 8; i++)
            {
                float ang = i / 8f * Mathf.PI * 2f;
                var offset = new Vector3(Mathf.Cos(ang), Mathf.Sin(ang), 0f) * 40f;
                SpawnGlow(worldPos + offset, tint, 40f, 0.3f);
            }

            FloatText(worldPos + Vector3.up * 30f, title, risk ? TarotPalette.Risk : TarotPalette.GoldBright, 32);
            if (risk)
                FloatText(worldPos + Vector3.down * 10f, "RISK", TarotPalette.Risk, 20);

            yield return null;
        }

        private IEnumerator ImpactRoutine(Vector3 worldPos, Color tint, int damage)
        {
            PlayScreenFlash(tint, 0.22f, 0.2f);
            SpawnGlow(worldPos, tint, 120f, 0.4f);
            // Extra burst for hit
            for (int i = 0; i < 6; i++)
            {
                float ang = i / 6f * Mathf.PI * 2f;
                var off = new Vector3(Mathf.Cos(ang), Mathf.Sin(ang), 0f) * 50f;
                SpawnGlow(worldPos + off, tint, 36f, 0.28f);
            }

            FloatText(worldPos, $"-{damage}", Color.white, 36);
            yield return null;
        }

        private IEnumerator ProjectileRoutine(
            Vector3 from,
            Vector3 to,
            ElementId element,
            Color tint,
            System.Action onArrived,
            float duration)
        {
            // Muzzle flash at caster
            SpawnGlow(from, tint, 90f, 0.28f);
            SpawnGlow(from, Color.white, 40f, 0.18f);

            // Projectile root
            var rootGo = new GameObject("SpellProjectile", typeof(RectTransform));
            rootGo.transform.SetParent(_overlayRoot, false);
            var root = rootGo.GetComponent<RectTransform>();
            root.sizeDelta = Vector2.zero;
            root.position = from;

            // Outer glow
            var glow = UiFactory.Image(root, "Glow",
                ProceduralSpriteFactory.SoftGlow(64, Color.white), tint, false);
            glow.rectTransform.sizeDelta = ProjectileSize(element) * 1.55f;

            // Core ball / shard
            var core = UiFactory.Image(root, "Core",
                ProjectileSprite(element), Color.white, false);
            core.rectTransform.sizeDelta = ProjectileSize(element);
            core.color = Color.Lerp(tint, Color.white, 0.25f);

            // Inner hot core
            var hot = UiFactory.Image(root, "Hot",
                ProceduralSpriteFactory.SoftGlow(32, Color.white), Color.white, false);
            hot.rectTransform.sizeDelta = ProjectileSize(element) * 0.35f;
            hot.color = new Color(1f, 1f, 1f, 0.85f);

            duration = Mathf.Max(0.2f, duration);
            // Lightning is snappier
            if (element == ElementId.Lightning || element == ElementId.Storm)
                duration *= 0.72f;
            // Ice / earth slightly slower
            if (element == ElementId.Ice || element == ElementId.Earth)
                duration *= 1.12f;

            float t = 0f;
            float trailCd = 0f;
            Vector3 dir = (to - from);
            float angle = Mathf.Atan2(dir.y, dir.x) * Mathf.Rad2Deg;

            while (t < duration)
            {
                t += Time.unscaledDeltaTime;
                float u = Mathf.Clamp01(t / duration);
                // Ease-in then ease-out arc
                float ease = u * u * (3f - 2f * u);
                // Slight parabolic arc
                float arc = Mathf.Sin(u * Mathf.PI) * ArcHeight(element);
                Vector3 pos = Vector3.Lerp(from, to, ease);
                // Perpendicular offset for arc
                Vector3 perp = new Vector3(-dir.y, dir.x, 0f).normalized;
                if (perp.sqrMagnitude < 0.01f)
                    perp = Vector3.right;
                pos += perp * arc;

                // Lightning zig-zag
                if (element == ElementId.Lightning || element == ElementId.Storm)
                {
                    float zig = Mathf.Sin(u * Mathf.PI * 8f) * 18f * (1f - u);
                    pos += perp * zig;
                }

                root.position = pos;

                // Face flight direction
                Vector3 look = to - from;
                if (element == ElementId.Lightning || element == ElementId.Ice || element == ElementId.Metal)
                    root.localRotation = Quaternion.Euler(0f, 0f, angle - 90f);
                else
                    root.localRotation = Quaternion.identity;

                // Pulse scale
                float pulse = 1f + Mathf.Sin(u * Mathf.PI * 6f) * 0.08f;
                float grow = Mathf.Lerp(0.7f, 1.15f, ease);
                root.localScale = Vector3.one * pulse * grow;

                // Fire flicker
                if (element == ElementId.Fire || element == ElementId.Blood)
                {
                    var fc = Color.Lerp(tint, TarotPalette.GoldBright, Mathf.Abs(Mathf.Sin(u * 20f)) * 0.4f);
                    glow.color = fc;
                    core.color = Color.Lerp(fc, Color.white, 0.2f);
                }

                // Trail sparks
                trailCd -= Time.unscaledDeltaTime;
                if (trailCd <= 0f)
                {
                    trailCd = element == ElementId.Lightning ? 0.03f : 0.05f;
                    SpawnGlow(pos, tint, ProjectileSize(element).x * 0.45f, 0.22f);
                }

                yield return null;
            }

            root.position = to;
            if (rootGo != null)
                Destroy(rootGo);

            // Arrival burst
            SpawnGlow(to, tint, 100f, 0.3f);
            SpawnGlow(to, Color.white, 50f, 0.2f);

            onArrived?.Invoke();
        }

        private static Vector2 ProjectileSize(ElementId e) => e switch
        {
            ElementId.Fire => new Vector2(72f, 72f),
            ElementId.Lightning => new Vector2(48f, 90f),
            ElementId.Ice => new Vector2(56f, 70f),
            ElementId.Water => new Vector2(68f, 68f),
            ElementId.Earth => new Vector2(70f, 64f),
            ElementId.Shadow => new Vector2(66f, 66f),
            ElementId.Light => new Vector2(74f, 74f),
            ElementId.Nature => new Vector2(64f, 64f),
            ElementId.Storm => new Vector2(52f, 88f),
            ElementId.Poison => new Vector2(60f, 68f),
            ElementId.Aether => new Vector2(70f, 70f),
            ElementId.Crystal => new Vector2(58f, 72f),
            ElementId.Blood => new Vector2(64f, 64f),
            ElementId.Metal => new Vector2(60f, 60f),
            ElementId.Arcane => new Vector2(68f, 68f),
            _ => new Vector2(64f, 64f)
        };

        private static float ArcHeight(ElementId e) => e switch
        {
            ElementId.Lightning => 8f,
            ElementId.Storm => 10f,
            ElementId.Earth => 25f,
            ElementId.Ice => 35f,
            ElementId.Fire => 45f,
            ElementId.Air => 55f,
            _ => 40f
        };

        private static Sprite ProjectileSprite(ElementId e)
        {
            // Soft core for orbs; disc for solid; reuse soft glow for most
            return e switch
            {
                ElementId.Lightning or ElementId.Storm =>
                    ProceduralSpriteFactory.RoundedRect(32, 64, 6,
                        Color.Lerp(TarotPalette.ForElement(e), Color.white, 0.4f),
                        TarotPalette.GoldBright, 2),
                ElementId.Ice or ElementId.Crystal =>
                    ProceduralSpriteFactory.RoundedRect(40, 56, 4,
                        Color.Lerp(TarotPalette.ForElement(e), Color.white, 0.35f),
                        Color.white, 2),
                ElementId.Earth or ElementId.Metal =>
                    ProceduralSpriteFactory.RoundedRect(48, 44, 10,
                        TarotPalette.ForElement(e), TarotPalette.Gold, 2),
                _ => ProceduralSpriteFactory.SoftGlow(64, Color.white)
            };
        }

        private IEnumerator FlashRoutine(Color color, float peak, float duration)
        {
            float t = 0f;
            while (t < duration)
            {
                t += Time.unscaledDeltaTime;
                float u = t / duration;
                float a = u < 0.3f
                    ? Mathf.Lerp(0f, peak, u / 0.3f)
                    : Mathf.Lerp(peak, 0f, (u - 0.3f) / 0.7f);
                color.a = a;
                _fullFlash.color = color;
                yield return null;
            }

            color.a = 0f;
            _fullFlash.color = color;
        }

        private IEnumerator FloatTextRoutine(Vector3 worldPos, string text, Color color, int fontSize)
        {
            var label = UiFactory.Text(_overlayRoot, "Float", text, fontSize, TextAnchor.MiddleCenter, color, FontStyle.Bold);
            var rt = label.rectTransform;
            rt.sizeDelta = new Vector2(400f, 80f);
            rt.position = worldPos;

            // outline-ish
            label.horizontalOverflow = HorizontalWrapMode.Overflow;
            label.verticalOverflow = VerticalWrapMode.Overflow;

            float t = 0f;
            const float dur = 0.85f;
            Vector3 start = worldPos;
            while (t < dur)
            {
                t += Time.unscaledDeltaTime;
                float u = t / dur;
                rt.position = start + Vector3.up * (u * 80f);
                var c = color;
                c.a = 1f - u;
                label.color = c;
                rt.localScale = Vector3.one * (1f + u * 0.25f);
                yield return null;
            }

            if (label != null)
                Destroy(label.gameObject);
        }

        private void SpawnGlow(Vector3 worldPos, Color tint, float size, float life)
        {
            StartCoroutine(GlowRoutine(worldPos, tint, size, life));
        }

        private IEnumerator GlowRoutine(Vector3 worldPos, Color tint, float size, float life)
        {
            var img = UiFactory.Image(_overlayRoot, "Glow",
                ProceduralSpriteFactory.SoftGlow(64, Color.white), tint, false);
            var rt = img.rectTransform;
            rt.sizeDelta = new Vector2(size, size);
            rt.position = worldPos;

            float t = 0f;
            while (t < life)
            {
                t += Time.unscaledDeltaTime;
                float u = t / life;
                float s = Mathf.Lerp(0.6f, 1.4f, u);
                rt.localScale = Vector3.one * s;
                var c = tint;
                c.a = (1f - u) * tint.a;
                img.color = c;
                yield return null;
            }

            if (img != null)
                Destroy(img.gameObject);
        }
    }
}
