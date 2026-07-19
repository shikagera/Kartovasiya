// Procedural minimal techno for main menu (Hotline Miami / Katana Zero vibe)
// Процедурный минимал-техно для меню — без аудио-ассетов
// Chaos Spellweaver (Картовасия)

using UnityEngine;

namespace Kartovasiya.UI
{
    /// <summary>
    /// Runtime-generated looping dark synth / minimal techno for the main menu.
    /// Dark minor bass, sparse kick+clap, dry hats, short stab lead.
    /// </summary>
    public class ProceduralMenuMusic : MonoBehaviour
    {
        public static ProceduralMenuMusic Instance { get; private set; }

        [SerializeField] [Range(0f, 1f)] private float _volume = 0.28f;
        [SerializeField] private bool _muted;

        private AudioSource _source;
        private AudioClip _loop;
        private bool _built;

        // 120 BPM → clean 8s / 4-bar loop
        private const int SampleRate = 22050;
        private const float Bpm = 120f;
        private const int Bars = 4;
        private const int StepsPerBar = 16; // 16th notes

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
            _source.loop = true;
            _source.spatialBlend = 0f;
            _source.priority = 64;
            _source.volume = _volume;
        }

        private void OnDestroy()
        {
            if (Instance == this)
                Instance = null;
            if (_loop != null)
            {
                Destroy(_loop);
                _loop = null;
            }
        }

        public void SetMuted(bool muted)
        {
            _muted = muted;
            if (_muted)
                StopMenu();
            else if (_source != null && !_source.isPlaying && _loop != null)
                PlayMenu();
        }

        public void SetVolume(float v)
        {
            _volume = Mathf.Clamp01(v);
            if (_source != null)
                _source.volume = _volume;
        }

        public void PlayMenu()
        {
            if (_muted || _source == null)
                return;

            EnsureClip();
            if (_loop == null)
                return;

            if (_source.isPlaying && _source.clip == _loop)
                return;

            _source.clip = _loop;
            _source.volume = _volume;
            _source.Play();
        }

        public void StopMenu()
        {
            if (_source == null)
                return;
            if (_source.isPlaying)
                _source.Stop();
        }

        private void EnsureClip()
        {
            if (_built && _loop != null)
                return;

            _loop = BuildLoop();
            _built = _loop != null;
        }

        private static AudioClip BuildLoop()
        {
            float secondsPerBeat = 60f / Bpm;
            float loopSeconds = Bars * 4f * secondsPerBeat; // 4/4
            int samples = Mathf.RoundToInt(loopSeconds * SampleRate);
            // Snap to whole steps so pattern lands cleanly
            int stepSamples = Mathf.RoundToInt(secondsPerBeat * 0.25f * SampleRate);
            int totalSteps = Bars * StepsPerBar;
            samples = stepSamples * totalSteps;

            var data = new float[samples];

            // A minor dark palette (Hz)
            // A2 bass, E3, C4, A3 lead — sparse HM/KZ motif
            const float A2 = 110f;
            const float E2 = 82.41f;
            const float C3 = 130.81f;
            const float G2 = 98f;
            const float A3 = 220f;
            const float C4 = 261.63f;
            const float E4 = 329.63f;
            const float G4 = 392f;

            // Bass root per bar (4 bars)
            float[] bassRoots = { A2, A2, E2, G2 };

            // Lead stab pattern: step index → freq (0 = rest). Sparse.
            // One short motif that repeats with a variation in bar 3–4.
            float[] leadNotes = new float[totalSteps];
            // Bar 1: offbeat stabs
            leadNotes[2] = A3;
            leadNotes[6] = C4;
            leadNotes[10] = A3;
            leadNotes[14] = E4;
            // Bar 2: quieter echo of motif
            leadNotes[18] = A3;
            leadNotes[26] = C4;
            // Bar 3: higher tension
            leadNotes[34] = E4;
            leadNotes[38] = G4;
            leadNotes[42] = E4;
            leadNotes[46] = C4;
            // Bar 4: resolve-ish
            leadNotes[50] = A3;
            leadNotes[54] = C4;
            leadNotes[58] = A3;

            for (int step = 0; step < totalSteps; step++)
            {
                int start = step * stepSamples;
                int bar = step / StepsPerBar;
                int stepInBar = step % StepsPerBar;
                float bassRoot = bassRoots[bar % bassRoots.Length];

                // Kick: 1 and 3 (steps 0, 8) — dry, punchy
                if (stepInBar == 0 || stepInBar == 8)
                    MixKick(data, start, stepSamples, 0.72f);

                // Extra ghost kick bar 4 for drive
                if (bar == 3 && stepInBar == 12)
                    MixKick(data, start, stepSamples, 0.35f);

                // Clap/snare: 2 and 4 (steps 4, 12)
                if (stepInBar == 4 || stepInBar == 12)
                    MixClap(data, start, stepSamples, 0.42f);

                // Hats: every other 16th, quieter on off
                if ((stepInBar & 1) == 0)
                {
                    float hatVol = stepInBar % 4 == 0 ? 0.18f : 0.11f;
                    MixHat(data, start, stepSamples, hatVol);
                }

                // Open hat on last 8th of bar occasionally
                if (stepInBar == 14)
                    MixOpenHat(data, start, stepSamples * 2, 0.12f);

                // Bass: long notes every half-bar (steps 0 and 8), slight slide feel via detune
                if (stepInBar == 0 || stepInBar == 8)
                {
                    float note = stepInBar == 0 ? bassRoot : bassRoot * (bar % 2 == 0 ? 1.5f : 1.0f); // 5th or root
                    // Hold for ~half bar minus tiny gap
                    int dur = stepSamples * 7;
                    MixBass(data, start, dur, note, 0.38f);
                }

                // Sub pulse on every beat for body
                if (stepInBar % 4 == 0)
                    MixSub(data, start, stepSamples * 2, bassRoot * 0.5f, 0.16f);

                // Lead stabs
                if (leadNotes[step] > 1f)
                    MixLead(data, start, Mathf.RoundToInt(stepSamples * 1.4f), leadNotes[step], 0.22f);
            }

            // Soft pad drone under whole loop (A minor fifth)
            MixPad(data, samples, A2, E2 * 2f, 0.045f);

            SoftClip(data, 0.92f);

            var clip = AudioClip.Create("MenuTechnoLoop", samples, 1, SampleRate, false);
            clip.SetData(data, 0);
            return clip;
        }

        // ── Synth voices ──────────────────────────────────────────────

        private static void MixKick(float[] buf, int start, int maxLen, float vol)
        {
            int n = Mathf.Min(maxLen, Mathf.RoundToInt(SampleRate * 0.09f));
            n = Mathf.Min(n, buf.Length - start);
            for (int i = 0; i < n; i++)
            {
                float t = i / (float)SampleRate;
                float env = Mathf.Exp(-t * 38f);
                // Pitch sweep 120 → 45 Hz
                float freq = Mathf.Lerp(120f, 45f, Mathf.Clamp01(t * 55f));
                float phase = 2f * Mathf.PI * freq * t;
                float body = Mathf.Sin(phase);
                // Soft click
                float click = (i < 40 ? (1f - i / 40f) : 0f) * (Mathf.Sin(2f * Mathf.PI * 1800f * t) * 0.35f);
                buf[start + i] += (body * 0.9f + click) * env * vol;
            }
        }

        private static void MixClap(float[] buf, int start, int maxLen, float vol)
        {
            int n = Mathf.Min(maxLen, Mathf.RoundToInt(SampleRate * 0.07f));
            n = Mathf.Min(n, buf.Length - start);
            // Multi-burst noise clap
            for (int i = 0; i < n; i++)
            {
                float t = i / (float)SampleRate;
                float env = Mathf.Exp(-t * 28f);
                // 3 staggered bursts
                float burst = 0f;
                burst += Burst(t, 0.000f);
                burst += Burst(t, 0.012f) * 0.7f;
                burst += Burst(t, 0.024f) * 0.45f;
                // Band-ish: high-pass-ish by subtracting slow noise (cheap)
                float noise = HashNoise(start + i);
                float high = noise - HashNoise(start + i + 17) * 0.5f;
                buf[start + i] += high * burst * env * vol;
            }
        }

        private static float Burst(float t, float delay)
        {
            float u = t - delay;
            if (u < 0f || u > 0.02f) return 0f;
            return Mathf.Exp(-u * 90f);
        }

        private static void MixHat(float[] buf, int start, int maxLen, float vol)
        {
            int n = Mathf.Min(maxLen, Mathf.RoundToInt(SampleRate * 0.025f));
            n = Mathf.Min(n, buf.Length - start);
            for (int i = 0; i < n; i++)
            {
                float t = i / (float)SampleRate;
                float env = Mathf.Exp(-t * 90f);
                float noise = HashNoise(start + i * 3);
                // Metallic edge
                float metal = Mathf.Sin(2f * Mathf.PI * 6500f * t) * 0.25f
                              + Mathf.Sin(2f * Mathf.PI * 9200f * t) * 0.15f;
                buf[start + i] += (noise * 0.7f + metal) * env * vol;
            }
        }

        private static void MixOpenHat(float[] buf, int start, int maxLen, float vol)
        {
            int n = Mathf.Min(maxLen, Mathf.RoundToInt(SampleRate * 0.12f));
            n = Mathf.Min(n, buf.Length - start);
            for (int i = 0; i < n; i++)
            {
                float t = i / (float)SampleRate;
                float env = Mathf.Exp(-t * 14f);
                float noise = HashNoise(start + i * 5);
                float metal = Mathf.Sin(2f * Mathf.PI * 5800f * t) * 0.2f;
                buf[start + i] += (noise * 0.55f + metal) * env * vol;
            }
        }

        private static void MixBass(float[] buf, int start, int len, float freq, float vol)
        {
            int n = Mathf.Min(len, buf.Length - start);
            if (n <= 0) return;
            for (int i = 0; i < n; i++)
            {
                float t = i / (float)SampleRate;
                float pos = i / (float)n;
                // ADSR-ish: fast attack, long sustain, short release
                float env = pos < 0.04f
                    ? pos / 0.04f
                    : pos > 0.85f
                        ? (1f - pos) / 0.15f
                        : 1f;
                env = Mathf.Clamp01(env);

                // Soft square (odd harmonics limited) — dark techno bass
                float phase = 2f * Mathf.PI * freq * t;
                float sq = Mathf.Sin(phase)
                           + Mathf.Sin(phase * 3f) * 0.22f
                           + Mathf.Sin(phase * 5f) * 0.08f;
                // Slight PWM-ish thickness
                float saw = 2f * ((freq * t) % 1f) - 1f;
                float v = sq * 0.75f + saw * 0.18f;
                buf[start + i] += v * env * vol;
            }
        }

        private static void MixSub(float[] buf, int start, int len, float freq, float vol)
        {
            int n = Mathf.Min(len, buf.Length - start);
            for (int i = 0; i < n; i++)
            {
                float t = i / (float)SampleRate;
                float pos = i / (float)Mathf.Max(1, n);
                float env = Mathf.Sin(pos * Mathf.PI); // half-sine pulse
                buf[start + i] += Mathf.Sin(2f * Mathf.PI * freq * t) * env * vol;
            }
        }

        private static void MixLead(float[] buf, int start, int len, float freq, float vol)
        {
            int n = Mathf.Min(len, buf.Length - start);
            for (int i = 0; i < n; i++)
            {
                float t = i / (float)SampleRate;
                float pos = i / (float)Mathf.Max(1, n);
                // Plucky envelope
                float env = Mathf.Exp(-pos * 4.5f) * (1f - pos * 0.35f);
                env = Mathf.Clamp01(env);

                float phase = 2f * Mathf.PI * freq * t;
                // Thin saw + detuned twin (synthwave stab)
                float s1 = 2f * ((freq * t) % 1f) - 1f;
                float s2 = 2f * ((freq * 1.007f * t) % 1f) - 1f;
                float sine = Mathf.Sin(phase);
                float v = s1 * 0.35f + s2 * 0.25f + sine * 0.4f;
                // Light bit of high shimmer
                v += Mathf.Sin(phase * 2f) * 0.12f * env;
                buf[start + i] += v * env * vol;
            }
        }

        private static void MixPad(float[] buf, int samples, float f1, float f2, float vol)
        {
            for (int i = 0; i < samples; i++)
            {
                float t = i / (float)SampleRate;
                float loopPos = i / (float)samples;
                // Tiny edge fade hides phase seams on non-integer period tones
                float fade = 1f;
                const float edgeFrac = 0.015f;
                if (loopPos < edgeFrac)
                    fade = loopPos / edgeFrac;
                else if (loopPos > 1f - edgeFrac)
                    fade = (1f - loopPos) / edgeFrac;

                float a = Mathf.Sin(2f * Mathf.PI * f1 * t);
                float b = Mathf.Sin(2f * Mathf.PI * f2 * t);
                float c = Mathf.Sin(2f * Mathf.PI * f1 * 1.5f * t) * 0.35f;
                float trem = 0.85f + 0.15f * Mathf.Sin(2f * Mathf.PI * 0.25f * t);
                buf[i] += (a + b + c) * 0.33f * vol * fade * trem;
            }
        }

        private static void SoftClip(float[] data, float ceiling)
        {
            for (int i = 0; i < data.Length; i++)
            {
                float x = data[i];
                // tanh soft clip
                x = (float)System.Math.Tanh(x * 1.15f);
                data[i] = Mathf.Clamp(x * 0.95f, -ceiling, ceiling);
            }
        }

        /// <summary>Deterministic noise (no Random — stable loop).</summary>
        private static float HashNoise(int n)
        {
            unchecked
            {
                uint x = (uint)n;
                x ^= x >> 13;
                x *= 0x5bd1e995u;
                x ^= x >> 15;
                return (x & 0xFFFF) / 32768f - 1f;
            }
        }
    }
}
