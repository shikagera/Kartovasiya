// Tiny procedural SFX (no audio assets) / Процедурные SFX без клипов
// Chaos Spellweaver (Картовасия)

using UnityEngine;

namespace Kartovasiya.UI
{
    /// <summary>
    /// Generates short AudioClips at runtime (pickup, fuse, cast, hit, UI).
    /// Генерирует короткие клипы в runtime.
    /// </summary>
    public class ProceduralSfx : MonoBehaviour
    {
        public static ProceduralSfx Instance { get; private set; }

        [SerializeField] [Range(0f, 1f)] private float _volume = 0.35f;
        [SerializeField] private bool _muted;

        private AudioSource _source;

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }

            Instance = this;
            _source = gameObject.AddComponent<AudioSource>();
            _source.playOnAwake = false;
            _source.spatialBlend = 0f;
        }

        private void OnDestroy()
        {
            if (Instance == this)
                Instance = null;
        }

        public void SetMuted(bool muted) => _muted = muted;

        public void SetVolume(float v) => _volume = Mathf.Clamp01(v);

        public float Volume => _volume;

        public void PlayPickup() => PlayTone(660f, 0.05f, 0.2f);
        public void PlayDropOk() => PlayTone(520f, 0.06f, 0.25f);
        public void PlayDropFail() => PlayTone(180f, 0.1f, 0.3f);
        public void PlayFuse() => PlayChord(0.12f, 440f, 554f, 659f);
        public void PlayCombo() => PlayChord(0.18f, 523f, 659f, 784f, 1046f);
        public void PlayCast() => PlayTone(300f, 0.08f, 0.35f, true);
        public void PlayHit() => PlayTone(140f, 0.09f, 0.4f, true);
        public void PlayUi() => PlayTone(800f, 0.04f, 0.15f);

        private void PlayTone(float freq, float dur, float volMul, bool noise = false)
        {
            if (_muted || _source == null)
                return;

            int sampleRate = 22050;
            int samples = Mathf.CeilToInt(sampleRate * dur);
            var clip = AudioClip.Create("sfx", samples, 1, sampleRate, false);
            var data = new float[samples];
            for (int i = 0; i < samples; i++)
            {
                float t = i / (float)sampleRate;
                float env = 1f - (i / (float)samples); // decay
                env *= env;
                float v;
                if (noise)
                    v = (Random.value * 2f - 1f) * 0.5f * env;
                else
                    v = Mathf.Sin(2f * Mathf.PI * freq * t) * env;
                data[i] = v * _volume * volMul;
            }

            clip.SetData(data, 0);
            _source.PlayOneShot(clip, 1f);
        }

        private void PlayChord(float dur, params float[] freqs)
        {
            if (_muted || _source == null || freqs == null || freqs.Length == 0)
                return;

            int sampleRate = 22050;
            int samples = Mathf.CeilToInt(sampleRate * dur);
            var clip = AudioClip.Create("chord", samples, 1, sampleRate, false);
            var data = new float[samples];
            float inv = 1f / freqs.Length;

            for (int i = 0; i < samples; i++)
            {
                float t = i / (float)sampleRate;
                float env = 1f - (i / (float)samples);
                env = Mathf.Sin(env * Mathf.PI * 0.5f);
                float v = 0f;
                for (int f = 0; f < freqs.Length; f++)
                    v += Mathf.Sin(2f * Mathf.PI * freqs[f] * t);
                data[i] = v * inv * env * _volume * 0.45f;
            }

            clip.SetData(data, 0);
            _source.PlayOneShot(clip, 1f);
        }
    }
}
