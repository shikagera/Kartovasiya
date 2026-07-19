// Procedural Undertale-style chiptune for combat runs
// Чиптюн в духе Undertale на время run (без аудио-ассетов)
// Chaos Spellweaver (Картовасия)

using UnityEngine;

namespace Kartovasiya.UI
{
    /// <summary>
    /// Looping 8-bit battle theme: square lead, triangle bass, soft drums.
    /// Inspired by Undertale energy (original melody — not a copy).
    /// </summary>
    public class ProceduralRunMusic : MonoBehaviour
    {
        public static ProceduralRunMusic Instance { get; private set; }

        [SerializeField] [Range(0f, 1f)] private float _volume = 0.26f;
        [SerializeField] private bool _muted;

        private AudioSource _source;
        private AudioClip _loop;
        private bool _built;

        // ~130 BPM feels like a light battle theme; 8 bars = ~14.8s
        private const int SampleRate = 22050;
        private const float Bpm = 130f;
        private const int Bars = 8;
        private const int StepsPerBar = 16;

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
            _source.priority = 70;
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
                StopRun();
            else if (_source != null && !_source.isPlaying && _loop != null)
                PlayRun();
        }

        public void SetVolume(float v)
        {
            _volume = Mathf.Clamp01(v);
            if (_source != null)
                _source.volume = _volume;
        }

        public void PlayRun()
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

        public void StopRun()
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
            int stepSamples = Mathf.RoundToInt(secondsPerBeat * 0.25f * SampleRate);
            int totalSteps = Bars * StepsPerBar;
            int samples = stepSamples * totalSteps;

            var data = new float[samples];

            // A minor pentatonic-ish battle palette (Hz)
            // Root A3=220, C4=261.63, D4=293.66, E4=329.63, G4=392, A4=440, C5=523.25
            const float A2 = 110f;
            const float E2 = 82.41f;
            const float G2 = 98f;
            const float A3 = 220f;
            const float C4 = 261.63f;
            const float D4 = 293.66f;
            const float E4 = 329.63f;
            const float G4 = 392f;
            const float A4 = 440f;
            const float B4 = 493.88f;
            const float C5 = 523.25f;
            const float E5 = 659.25f;

            // Bass roots per bar (walking undertale-like)
            float[] bassRoots =
            {
                A2, A2, C4 * 0.5f, C4 * 0.5f,
                G2, G2, E2, E2
            };

            // Lead melody in 16ths (0 = rest). Two 4-bar phrases.
            float[] lead = new float[totalSteps];
            // Phrase A — hopeful bounce
            float[] phraseA =
            {
                A4, 0, C5, 0, E5, 0, C5, 0,
                A4, 0, G4, 0, E4, 0, G4, 0,
                A4, 0, C5, 0, E5, E5, D4, 0,
                C5, 0, A4, 0, G4, 0, E4, 0,
                // bar 3–4
                C5, 0, D4, 0, E5, 0, G4, 0,
                A4, A4, 0, G4, E4, 0, C4, 0,
                D4, 0, E4, 0, G4, 0, A4, 0,
                B4, 0, C5, 0, A4, 0, 0, 0
            };
            // Phrase B — higher energy (battle lift)
            float[] phraseB =
            {
                E5, 0, E5, 0, D4, 0, C5, 0,
                A4, 0, G4, A4, C5, 0, E5, 0,
                G4, 0, A4, 0, C5, 0, E5, E5,
                D4, 0, C5, 0, A4, 0, G4, 0,
                A4, 0, C5, E5, 0, C5, A4, 0,
                G4, 0, E4, 0, G4, A4, 0, C5,
                E5, 0, D4, 0, C5, 0, A4, 0,
                G4, E4, 0, A4, 0, 0, A4, 0
            };

            for (int i = 0; i < 64 && i < totalSteps; i++)
                lead[i] = phraseA[i];
            for (int i = 0; i < 64 && 64 + i < totalSteps; i++)
                lead[64 + i] = phraseB[i];

            // Arpeggio countermelody (quieter, every other bar)
            float[] arp = new float[totalSteps];
            float[] arpPat = { A3, C4, E4, A4, E4, C4, A3, 0 };
            for (int step = 0; step < totalSteps; step++)
            {
                int bar = step / StepsPerBar;
                if (bar % 2 == 1)
                    arp[step] = arpPat[step % arpPat.Length];
            }

            for (int step = 0; step < totalSteps; step++)
            {
                int start = step * stepSamples;
                int bar = step / StepsPerBar;
                int stepInBar = step % StepsPerBar;
                float bassRoot = bassRoots[bar % bassRoots.Length];

                // Kick on 1 & 3
                if (stepInBar == 0 || stepInBar == 8)
                    MixKick(data, start, stepSamples, 0.55f);
                // Snare/clap on 2 & 4
                if (stepInBar == 4 || stepInBar == 12)
                    MixSnare(data, start, stepSamples, 0.32f);
                // Closed hat 8ths
                if ((stepInBar & 1) == 0)
                    MixHat(data, start, stepSamples, stepInBar % 4 == 0 ? 0.14f : 0.09f);

                // Bass: half notes (square-ish triangle hybrid — chip bass)
                if (stepInBar == 0 || stepInBar == 8)
                {
                    float note = stepInBar == 0 ? bassRoot : bassRoot * 1.5f; // fifth
                    MixBass(data, start, stepSamples * 7, note, 0.34f);
                }

                // Lead square
                if (lead[step] > 1f)
                    MixSquare(data, start, Mathf.RoundToInt(stepSamples * 1.5f), lead[step], 0.20f, duty: 0.5f);

                // Soft arp pulse
                if (arp[step] > 1f)
                    MixSquare(data, start, Mathf.RoundToInt(stepSamples * 0.9f), arp[step], 0.08f, duty: 0.25f);
            }

            // Soft pad under whole loop (gentle sine fifths)
            MixPad(data, samples, A2, E2 * 2f, 0.035f);

            SoftClip(data, 0.9f);

            var clip = AudioClip.Create("RunChiptuneLoop", samples, 1, SampleRate, false);
            clip.SetData(data, 0);
            return clip;
        }

        // ── Chip voices ───────────────────────────────────────────────

        private static void MixKick(float[] buf, int start, int maxLen, float vol)
        {
            int n = Mathf.Min(maxLen, Mathf.RoundToInt(SampleRate * 0.07f));
            n = Mathf.Min(n, buf.Length - start);
            for (int i = 0; i < n; i++)
            {
                float t = i / (float)SampleRate;
                float env = Mathf.Exp(-t * 42f);
                float freq = Mathf.Lerp(140f, 50f, Mathf.Clamp01(t * 50f));
                buf[start + i] += Mathf.Sin(2f * Mathf.PI * freq * t) * env * vol;
            }
        }

        private static void MixSnare(float[] buf, int start, int maxLen, float vol)
        {
            int n = Mathf.Min(maxLen, Mathf.RoundToInt(SampleRate * 0.055f));
            n = Mathf.Min(n, buf.Length - start);
            for (int i = 0; i < n; i++)
            {
                float t = i / (float)SampleRate;
                float env = Mathf.Exp(-t * 35f);
                float noise = HashNoise(start + i * 3);
                float tone = Mathf.Sin(2f * Mathf.PI * 180f * t) * 0.35f;
                buf[start + i] += (noise * 0.7f + tone) * env * vol;
            }
        }

        private static void MixHat(float[] buf, int start, int maxLen, float vol)
        {
            int n = Mathf.Min(maxLen, Mathf.RoundToInt(SampleRate * 0.02f));
            n = Mathf.Min(n, buf.Length - start);
            for (int i = 0; i < n; i++)
            {
                float t = i / (float)SampleRate;
                float env = Mathf.Exp(-t * 110f);
                float noise = HashNoise(start + i * 7);
                buf[start + i] += noise * env * vol;
            }
        }

        private static void MixBass(float[] buf, int start, int len, float freq, float vol)
        {
            int n = Mathf.Min(len, buf.Length - start);
            for (int i = 0; i < n; i++)
            {
                float t = i / (float)SampleRate;
                float pos = i / (float)Mathf.Max(1, n);
                float env = pos < 0.05f ? pos / 0.05f : pos > 0.85f ? (1f - pos) / 0.15f : 1f;
                // Triangle-ish bass (NES triangle vibe)
                float phase = (freq * t) % 1f;
                float tri = phase < 0.5f ? phase * 4f - 1f : 3f - phase * 4f;
                float sine = Mathf.Sin(2f * Mathf.PI * freq * t);
                buf[start + i] += (tri * 0.65f + sine * 0.35f) * env * vol;
            }
        }

        private static void MixSquare(float[] buf, int start, int len, float freq, float vol, float duty)
        {
            int n = Mathf.Min(len, buf.Length - start);
            duty = Mathf.Clamp(duty, 0.1f, 0.9f);
            for (int i = 0; i < n; i++)
            {
                float t = i / (float)SampleRate;
                float pos = i / (float)Mathf.Max(1, n);
                // Plucky envelope like UT melodies
                float env = Mathf.Exp(-pos * 2.8f) * (1f - pos * 0.25f);
                env = Mathf.Clamp01(env);
                float phase = (freq * t) % 1f;
                float sq = phase < duty ? 1f : -1f;
                // Soften harsh edges with a little sine
                float soft = sq * 0.85f + Mathf.Sin(2f * Mathf.PI * freq * t) * 0.15f;
                buf[start + i] += soft * env * vol;
            }
        }

        private static void MixPad(float[] buf, int samples, float f1, float f2, float vol)
        {
            for (int i = 0; i < samples; i++)
            {
                float t = i / (float)SampleRate;
                float loopPos = i / (float)samples;
                float fade = 1f;
                const float edge = 0.02f;
                if (loopPos < edge) fade = loopPos / edge;
                else if (loopPos > 1f - edge) fade = (1f - loopPos) / edge;

                float a = Mathf.Sin(2f * Mathf.PI * f1 * t);
                float b = Mathf.Sin(2f * Mathf.PI * f2 * t);
                float trem = 0.88f + 0.12f * Mathf.Sin(2f * Mathf.PI * 0.5f * t);
                buf[i] += (a + b) * 0.5f * vol * fade * trem;
            }
        }

        private static void SoftClip(float[] data, float ceiling)
        {
            for (int i = 0; i < data.Length; i++)
            {
                float x = (float)System.Math.Tanh(data[i] * 1.1f);
                data[i] = Mathf.Clamp(x * 0.95f, -ceiling, ceiling);
            }
        }

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
