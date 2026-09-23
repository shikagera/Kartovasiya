#include "audio/ProcAudio.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cstdio>

namespace shichoy {
namespace audio {

static constexpr int   SR = 44100;
static constexpr float TAU = 6.2831853f;

// ── Small synth toolkit / Мини-синтезатор ─────────────────────────────────
namespace {

struct Buf {
    std::vector<float> s;
    explicit Buf(float seconds) : s(size_t(seconds * SR), 0.0f) {}
    size_t n() const { return s.size(); }
};

// Быстрый белый шум с собственным состоянием (не трогаем игровой ГПСЧ).
struct Noise {
    uint32_t st = 0x1234567u;
    float operator()() {
        st ^= st << 13; st ^= st >> 17; st ^= st << 5;
        return float(int32_t(st)) * (1.0f / 2147483648.0f);
    }
};

// Резонансный фильтр (state-variable): даёт и полосовой, и низкочастотный.
struct Svf {
    float lp = 0, bp = 0;
    void step(float x, float fc, float q) {
        float f = 2.0f * std::sin(3.14159265f * clampf(fc, 20.0f, SR * 0.45f) / SR);
        float damp = 1.0f / clampf(q, 0.5f, 12.0f);
        float hp = x - lp - damp * bp;
        bp += f * hp;
        lp += f * bp;
    }
};

inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }

// Мягкое насыщение — вместо клиппинга на пиках.
inline float sat(float x) { return std::tanh(x * 1.4f) * 0.85f; }

// Паспорт «голоса стихии»: одна структура вместо шестнадцати функций.
struct Voice {
    float dur;          // длительность, с
    float f0, f1;       // частота: начало → конец
    int   partials;     // сколько обертонов у тона
    float inharm;       // 0 — гармонический ряд, 1 — металлический (несоизмеримый)
    float tone;         // громкость тональной части
    float noise;        // громкость шумовой части
    float fc0, fc1;     // свип фильтра шума
    float q;            // резонанс
    float attack;       // время атаки, с
    float decay;        // постоянная затухания, с
    float crackle;      // амплитудная модуляция шумом (треск)
    float sub;          // громкость низкого гула
    float subF;         // частота гула
    float subDecay;     // затухание гула
    float wobble;       // вибрато, Гц
    float wobDepth;
};

Buf renderVoice(const Voice& v, Noise& rnd) {
    Buf b(v.dur);
    float ph = 0.0f, subPh = 0.0f, wobPh = 0.0f;
    Svf svf;
    float crackleEnv = 0.0f;

    for (size_t i = 0; i < b.n(); ++i) {
        float t = float(i) / SR;
        float u = t / v.dur;                       // 0..1 по длительности

        float atk = v.attack > 0.0001f ? (1.0f - std::exp(-t / v.attack)) : 1.0f;
        float dec = std::exp(-t / std::max(0.005f, v.decay));
        float env = atk * dec;

        // Тон: гармоники либо несоизмеримые частоты (металл, кристалл)
        wobPh += TAU * v.wobble / SR;
        float vib = 1.0f + v.wobDepth * std::sin(wobPh);
        float f = lerpf(v.f0, v.f1, u) * vib;
        ph += TAU * f / SR;
        float tone = 0.0f;
        for (int k = 1; k <= v.partials; ++k) {
            float ratio = lerpf(float(k), 1.0f + (k - 1) * 1.87f + 0.31f * k * k, v.inharm);
            tone += std::sin(ph * ratio) / float(k);
        }
        tone *= v.tone * env;

        // Шум сквозь резонансный свип
        float nz = rnd();
        svf.step(nz, lerpf(v.fc0, v.fc1, u), v.q);
        float noisePart = (v.q > 1.2f ? svf.bp : svf.lp) * v.noise * env;

        // Треск: рваная огибающая поверх шума
        if (v.crackle > 0.0f) {
            crackleEnv = lerpf(crackleEnv, std::fabs(rnd()), 0.35f);
            noisePart *= lerpf(1.0f, crackleEnv * 2.2f, v.crackle);
        }

        // Низкий гул с собственным затуханием
        subPh += TAU * v.subF / SR;
        float sub = std::sin(subPh) * v.sub * std::exp(-t / std::max(0.01f, v.subDecay));

        b.s[i] = sat(tone + noisePart + sub);
    }

    // Короткий фейд в конце — чтобы не щёлкало на обрыве
    size_t fade = std::min<size_t>(b.n(), size_t(0.02f * SR));
    for (size_t i = 0; i < fade; ++i)
        b.s[b.n() - 1 - i] *= float(i) / float(fade);
    return b;
}

// Wave из float-буфера. Данные выделяем malloc — их освободит UnloadWave.
Wave toWave(const Buf& b) {
    Wave w{};
    w.frameCount = unsigned(b.n());
    w.sampleRate = SR;
    w.sampleSize = 16;
    w.channels = 1;
    short* data = (short*)std::malloc(b.n() * sizeof(short));
    for (size_t i = 0; i < b.n(); ++i) {
        float v = clampf(b.s[i], -1.0f, 1.0f);
        data[i] = short(v * 32000.0f);
    }
    w.data = data;
    return w;
}

// ── Звуковой слот с алиасами: короткие эффекты могут накладываться ────────
struct Slot {
    Sound base{};
    Sound alias[3]{};
    int   next = 0;
    bool  loaded = false;

    void load(const Buf& b) {
        Wave w = toWave(b);
        base = LoadSoundFromWave(w);
        UnloadWave(w);
        for (int i = 0; i < 3; ++i) alias[i] = LoadSoundAlias(base);
        loaded = true;
    }
    void unload() {
        if (!loaded) return;
        for (int i = 0; i < 3; ++i) UnloadSoundAlias(alias[i]);
        UnloadSound(base);
        loaded = false;
    }
    Sound& pick() {
        Sound& s = alias[next];
        next = (next + 1) % 3;
        return s;
    }
};

} // namespace

// ── Состояние модуля ──────────────────────────────────────────────────────
static bool  s_ready = false;
static bool  s_muted = false;
static float s_master = 0.75f;

static Slot s_sfx[int(Sfx::COUNT)];
static Slot s_elem[ELEMENT_COUNT + 1];
static Slot s_enemy[8];        // по одному голосу на вид приёма

// ── Рецепты стихий / Element recipes ──────────────────────────────────────
// Каждая стихия — свой набор ручек. Молния: мгновенная атака, резкий треск
// и низкий раскат в хвосте; лёд — несоизмеримые обертоны и звон; и так далее.
static const Voice kVoices[ELEMENT_COUNT + 1] = {
    //                dur   f0     f1    part inh  tone  nz   fc0    fc1    q    atk    dec  crk  sub  subF subDec wob wobD
    /* None      */ { 0.30f,  400,   300,  2, 0.0f, 0.30f,0.20f,1200,  600, 1.0f, 0.005f,0.15f,0.0f,0.0f, 60, 0.10f, 0,  0.0f },
    /* Fire      */ { 0.60f,  240,   120,  3, 0.20f,0.22f,0.85f,2000,  520, 1.6f, 0.004f,0.20f,0.85f,0.25f,58,0.22f, 0,  0.0f },
    /* Water     */ { 0.65f,  520,   170,  2, 0.05f,0.28f,0.62f, 950,  280, 2.6f, 0.012f,0.26f,0.10f,0.15f,70,0.18f, 7,  0.14f },
    /* Earth     */ { 0.75f,   95,    48,  2, 0.10f,0.55f,0.40f, 420,  110, 1.0f, 0.002f,0.24f,0.30f,0.80f,42,0.34f, 0,  0.0f },
    /* Air       */ { 0.55f,  620,   980,  2, 0.0f, 0.10f,0.95f, 700, 3200, 3.2f, 0.070f,0.19f,0.15f,0.0f, 60,0.10f, 0,  0.0f },
    /* Lightning */ { 0.95f, 2600,   300,  4, 0.50f,0.30f,0.95f,6500,  850, 4.2f, 0.0008f,0.10f,0.92f,0.60f,52,0.55f,0, 0.0f },
    /* Ice       */ { 0.85f, 1850,  1780,  5, 0.80f,0.40f,0.35f,5200, 2400, 5.0f, 0.002f,0.36f,0.25f,0.0f, 60,0.10f, 0,  0.0f },
    /* Nature    */ { 0.50f,  310,   205,  3, 0.15f,0.26f,0.58f,1250,  380, 1.3f, 0.020f,0.17f,0.45f,0.12f,55,0.14f, 0,  0.0f },
    /* Shadow    */ { 0.95f,  165,    72,  3, 0.30f,0.32f,0.48f, 520,  145, 2.0f, 0.120f,0.42f,0.20f,0.50f,38,0.46f, 3,  0.10f },
    /* Metal     */ { 0.90f,  720,   690,  5, 1.00f,0.42f,0.22f,3400, 1800, 3.0f, 0.001f,0.46f,0.10f,0.0f, 60,0.10f, 0,  0.0f },
    /* Light     */ { 1.00f,  880,  1340,  4, 0.10f,0.40f,0.16f,2600, 4200, 2.0f, 0.010f,0.52f,0.05f,0.0f, 60,0.10f, 0,  0.0f },
    /* Poison    */ { 0.70f,  270,   155,  2, 0.20f,0.26f,0.52f, 760,  250, 4.0f, 0.008f,0.23f,0.35f,0.15f,48,0.18f, 12, 0.45f },
    /* Aether    */ { 1.10f,  430,   180,  4, 0.65f,0.34f,0.32f,2500,  600, 3.0f, 0.030f,0.52f,0.15f,0.30f,36,0.42f, 3,  0.22f },
    /* Blood     */ { 0.70f,  135,    70,  2, 0.15f,0.34f,0.55f, 620,  185, 1.5f, 0.004f,0.21f,0.50f,0.62f,36,0.30f, 5,  0.10f },
    /* Storm     */ { 1.15f, 1700,   270,  3, 0.40f,0.26f,0.92f,4200,  620, 3.0f, 0.002f,0.31f,0.62f,0.62f,40,0.62f, 0,  0.0f },
    /* Crystal   */ { 1.20f, 2150,  2120,  6, 0.90f,0.40f,0.10f,6000, 4000, 6.0f, 0.001f,0.62f,0.05f,0.0f, 60,0.10f, 0,  0.0f },
    /* Arcane    */ { 1.00f,  310,   640,  5, 0.55f,0.34f,0.26f,1500, 3100, 3.0f, 0.020f,0.46f,0.15f,0.20f,44,0.34f, 6,  0.35f },
};

// ── Голоса приёмов противника ─────────────────────────────────────────────
// Порядок совпадает с MoveKind: удар, тяжёлый, серия, проклятие, защита,
// штопка, ярость, раскол. Все ниже и грубее игроцких — по ту сторону стола
// сидит не человек.
static const Voice kEnemyVoices[8] = {
    //          dur   f0    f1   part inh  tone  nz   fc0   fc1   q    atk    dec  crk  sub  subF subDec wob wobD
    /* Attack */ { 0.45f, 150,   80, 3, 0.30f,0.40f,0.55f,1400,  380, 1.6f, 0.001f,0.14f,0.45f,0.45f,44,0.20f, 0, 0.0f },
    /* Heavy  */ { 1.00f,  95,   42, 3, 0.25f,0.50f,0.60f, 800,  180, 1.4f, 0.002f,0.30f,0.55f,0.85f,32,0.70f, 0, 0.0f },
    /* Multi  */ { 0.60f, 260,  150, 2, 0.45f,0.26f,0.85f,2600,  700, 3.4f, 0.001f,0.16f,0.90f,0.25f,50,0.22f, 0, 0.0f },
    /* Curse  */ { 1.20f, 190,   62, 4, 0.55f,0.30f,0.62f, 900,  200, 2.4f, 0.110f,0.55f,0.30f,0.55f,34,0.62f, 4, 0.28f },
    /* Guard  */ { 0.70f, 520,  480, 5, 1.00f,0.44f,0.25f,3000, 1600, 3.0f, 0.001f,0.30f,0.15f,0.20f,60,0.18f, 0, 0.0f },
    /* Mend   */ { 0.80f, 300,  190, 2, 0.20f,0.28f,0.55f, 700,  240, 3.6f, 0.020f,0.34f,0.40f,0.25f,46,0.26f, 9, 0.35f },
    /* Enrage */ { 1.10f,  90,  230, 4, 0.40f,0.44f,0.55f, 500, 1800, 2.2f, 0.060f,0.50f,0.45f,0.60f,38,0.55f, 5, 0.15f },
    /* Sunder */ { 0.85f, 900,  180, 5, 0.85f,0.38f,0.80f,4200,  600, 3.6f, 0.001f,0.24f,0.75f,0.50f,40,0.36f, 0, 0.0f },
};

// ── Эффекты интерфейса и боя ──────────────────────────────────────────────
static Buf makeSfx(Sfx id, Noise& rnd) {
    auto V = [&](Voice v) { return renderVoice(v, rnd); };

    switch (id) {
        case Sfx::UiHover:
            return V({ 0.10f, 1500, 1500, 1, 0.0f, 0.12f, 0.05f, 3000, 3000, 2.0f,
                       0.001f, 0.030f, 0.0f, 0.0f, 60, 0.05f, 0, 0.0f });
        case Sfx::UiClick:
            return V({ 0.16f, 900, 620, 2, 0.30f, 0.26f, 0.14f, 2600, 1200, 2.0f,
                       0.001f, 0.055f, 0.0f, 0.10f, 90, 0.06f, 0, 0.0f });
        case Sfx::UiBack:
            return V({ 0.22f, 620, 380, 2, 0.20f, 0.24f, 0.12f, 1600, 700, 2.0f,
                       0.002f, 0.090f, 0.0f, 0.12f, 70, 0.09f, 0, 0.0f });

        case Sfx::CardDraw: {
            // Бумага по сукну: узкополосный шум с быстрым свипом вверх.
            Buf b = V({ 0.24f, 1, 1, 1, 0.0f, 0.0f, 0.85f, 900, 5200, 2.2f,
                        0.010f, 0.075f, 0.20f, 0.0f, 60, 0.05f, 0, 0.0f });
            return b;
        }
        case Sfx::CardSpell:
            return V({ 0.55f, 420, 760, 4, 0.45f, 0.30f, 0.30f, 1400, 3000, 2.6f,
                       0.006f, 0.22f, 0.10f, 0.18f, 70, 0.16f, 5, 0.18f });
        case Sfx::CardEffect:
            return V({ 0.40f, 330, 300, 3, 0.55f, 0.34f, 0.18f, 1200, 800, 2.0f,
                       0.003f, 0.19f, 0.05f, 0.20f, 80, 0.12f, 0, 0.0f });

        case Sfx::Reaction:
            // Реакция: удар + восходящий шлейф, самый громкий звук боя.
            return V({ 1.30f, 180, 90, 4, 0.35f, 0.42f, 0.80f, 3800, 500, 2.4f,
                       0.001f, 0.34f, 0.55f, 0.85f, 46, 0.70f, 0, 0.0f });
        case Sfx::Hit:
            return V({ 0.32f, 260, 130, 3, 0.30f, 0.34f, 0.55f, 1800, 500, 1.6f,
                       0.001f, 0.10f, 0.40f, 0.35f, 60, 0.13f, 0, 0.0f });
        case Sfx::Miss:
            return V({ 0.26f, 1, 1, 1, 0.0f, 0.0f, 0.70f, 3000, 900, 2.6f,
                       0.004f, 0.085f, 0.15f, 0.0f, 60, 0.05f, 0, 0.0f });
        case Sfx::PlayerHurt:
            return V({ 0.55f, 190, 82, 3, 0.25f, 0.40f, 0.62f, 900, 220, 1.4f,
                       0.001f, 0.17f, 0.55f, 0.70f, 40, 0.30f, 0, 0.0f });
        case Sfx::Shield:
            return V({ 0.60f, 520, 700, 4, 0.60f, 0.34f, 0.18f, 2200, 3200, 3.0f,
                       0.008f, 0.28f, 0.05f, 0.0f, 60, 0.10f, 0, 0.0f });
        case Sfx::Heal:
            return V({ 0.90f, 620, 940, 3, 0.08f, 0.34f, 0.12f, 2000, 3400, 2.0f,
                       0.030f, 0.42f, 0.0f, 0.0f, 60, 0.10f, 4, 0.05f });

        case Sfx::SpotBreak:
            // Хруст: несоизмеримые обертоны + рваный шум.
            return V({ 0.85f, 1400, 620, 5, 0.95f, 0.40f, 0.72f, 5000, 900, 4.0f,
                       0.001f, 0.24f, 0.80f, 0.35f, 55, 0.32f, 0, 0.0f });
        case Sfx::PhaseShift:
            return V({ 1.40f, 300, 120, 4, 0.55f, 0.38f, 0.45f, 2600, 400, 2.6f,
                       0.040f, 0.62f, 0.30f, 0.70f, 34, 0.80f, 2, 0.20f });
        case Sfx::Victory:
            return V({ 1.60f, 440, 880, 4, 0.05f, 0.40f, 0.10f, 2400, 4200, 2.0f,
                       0.020f, 0.75f, 0.0f, 0.15f, 110, 0.40f, 0, 0.0f });
        case Sfx::Defeat:
            return V({ 2.00f, 220, 55, 3, 0.35f, 0.44f, 0.35f, 900, 150, 1.8f,
                       0.060f, 0.95f, 0.15f, 0.75f, 30, 1.10f, 2, 0.12f });

        case Sfx::TableScrape:
            // Дерево из-под фрака: длинный шершавый свип.
            return V({ 1.10f, 1, 1, 1, 0.0f, 0.0f, 0.85f, 400, 2600, 1.8f,
                       0.090f, 0.55f, 0.55f, 0.20f, 48, 0.40f, 0, 0.0f });
        case Sfx::TableThud:
            return V({ 0.70f, 110, 52, 2, 0.20f, 0.50f, 0.45f, 500, 120, 1.0f,
                       0.001f, 0.20f, 0.35f, 0.85f, 40, 0.36f, 0, 0.0f });

        case Sfx::Gather:
            return V({ 0.45f, 780, 1180, 3, 0.25f, 0.34f, 0.16f, 2200, 3600, 2.4f,
                       0.004f, 0.17f, 0.10f, 0.0f, 60, 0.10f, 0, 0.0f });
        case Sfx::Encounter:
            return V({ 1.20f, 520, 130, 3, 0.45f, 0.38f, 0.55f, 3000, 400, 2.8f,
                       0.003f, 0.42f, 0.40f, 0.70f, 38, 0.70f, 0, 0.0f });

        case Sfx::CraftStart:
            return V({ 0.35f, 380, 520, 3, 0.50f, 0.30f, 0.25f, 1400, 2200, 2.2f,
                       0.004f, 0.14f, 0.15f, 0.10f, 70, 0.10f, 0, 0.0f });
        case Sfx::CraftSuccess:
            return V({ 1.00f, 600, 900, 4, 0.15f, 0.40f, 0.14f, 2200, 3800, 2.2f,
                       0.006f, 0.44f, 0.05f, 0.12f, 90, 0.26f, 0, 0.0f });
        case Sfx::CraftPerfect:
            return V({ 1.50f, 900, 1800, 5, 0.25f, 0.42f, 0.12f, 3000, 6000, 2.6f,
                       0.004f, 0.66f, 0.05f, 0.10f, 120, 0.30f, 0, 0.0f });
        case Sfx::CraftFail:
            return V({ 0.80f, 300, 110, 3, 0.70f, 0.40f, 0.45f, 1500, 300, 2.0f,
                       0.002f, 0.30f, 0.45f, 0.40f, 44, 0.36f, 0, 0.0f });

        case Sfx::Step:
            return V({ 0.14f, 90, 60, 1, 0.0f, 0.16f, 0.35f, 700, 260, 1.2f,
                       0.001f, 0.045f, 0.30f, 0.20f, 55, 0.05f, 0, 0.0f });
        default:
            return V(kVoices[0]);
    }
}

// ── Генеративная музыка / Generative music ────────────────────────────────
// Всё считается прямо в колбэке аудиопотока: ничего не грузится, ничего
// не зацикливается — трек не повторяется.
namespace {

struct Bell { float ph = 0, f = 0, amp = 0, age = 0, decay = 1.0f; bool on = false; };

struct MusicState {
    AudioStream stream{};
    bool  started = false;
    int   mode = int(Track::Off);      // пишется из главного потока
    float gain = 0.0f, target = 0.0f;  // сглаживание переходов
    double t = 0.0;

    float dronePh[3]{ 0, 0, 0 };
    float padPh[4]{ 0, 0, 0, 0 };
    float prevPadPh[4]{ 0, 0, 0, 0 };
    Bell  bells[6];
    Noise rnd;
    float windLp = 0.0f;
    float pulsePh = 0.0f, pulseEnv = 0.0f;

    int   chord = 0, prevChord = 0;
    float chordT = 0.0f, chordFade = 1.0f;
    float bellTimer = 3.0f;
};

MusicState s_music;

// Тоны минорной эстетики: полутона от корня (ре).
const int kChords[6][4] = {
    {  0,  3,  7, 12 },   // i
    {  0,  3,  8, 15 },   // VI
    { -2,  2,  5, 14 },   // VII
    {  0,  3,  7, 10 },   // i7
    { -4,  3,  7, 12 },   // VI/3
    {  0,  5,  8, 12 },   // iv
};
const int kPenta[5] = { 0, 3, 5, 7, 10 };

inline float noteHz(float root, int semis) {
    return root * std::pow(2.0f, semis / 12.0f);
}

// Орган: сумма первых гармоник с падающей амплитудой.
inline float organ(float ph) {
    return std::sin(ph) + 0.45f * std::sin(ph * 2.0f)
         + 0.25f * std::sin(ph * 3.0f) + 0.14f * std::sin(ph * 4.0f);
}

void musicCallback(void* buffer, unsigned int frames) {
    MusicState& m = s_music;
    short* out = (short*)buffer;

    const int mode = m.mode;
    const bool combat = (mode == int(Track::Combat));
    const bool over = (mode == int(Track::Overworld));
    const bool menu = (mode == int(Track::Menu));

    // Куда тянем громкость: в бою тише и напряжённее, в поле почти шёпот.
    float want = 0.0f;
    if (menu) want = 1.0f;
    else if (over) want = 0.55f;
    else if (combat) want = 0.72f;
    m.target = want;

    const float root = combat ? 61.74f : 73.42f;      // си-бемоль1 / ре2
    const float chordSecs = combat ? 7.0f : 10.5f;

    for (unsigned int i = 0; i < frames; ++i) {
        const float dt = 1.0f / SR;
        m.t += dt;
        m.gain += (m.target - m.gain) * 0.00004f;      // ~1 c на переход

        // Смена аккорда с кроссфейдом
        m.chordT += dt;
        if (m.chordT >= chordSecs) {
            m.chordT = 0.0f;
            m.prevChord = m.chord;
            for (int k = 0; k < 4; ++k) m.prevPadPh[k] = m.padPh[k];
            m.chord = int(std::fabs(m.rnd()) * 5.999f) % 6;
            m.chordFade = 0.0f;
        }
        if (m.chordFade < 1.0f) m.chordFade = std::min(1.0f, m.chordFade + dt * 0.45f);

        // 1. Дрон: две расстроенные основы + октава вниз
        float drone = 0.0f;
        const float df[3] = { root, root * 1.0043f, root * 0.5f };
        for (int k = 0; k < 3; ++k) {
            m.dronePh[k] += TAU * df[k] / SR;
            if (m.dronePh[k] > TAU) m.dronePh[k] -= TAU;
            drone += std::sin(m.dronePh[k]) * (k == 2 ? 0.55f : 0.40f);
        }
        drone *= 0.16f;

        // 2. Органный пад по текущему аккорду (со сшивкой предыдущего)
        float pad = 0.0f, padPrev = 0.0f;
        for (int k = 0; k < 4; ++k) {
            float f = noteHz(root * 2.0f, kChords[m.chord][k]);
            m.padPh[k] += TAU * f / SR;
            if (m.padPh[k] > TAU) m.padPh[k] -= TAU;
            pad += organ(m.padPh[k]);

            if (m.chordFade < 1.0f) {
                float pf = noteHz(root * 2.0f, kChords[m.prevChord][k]);
                m.prevPadPh[k] += TAU * pf / SR;
                if (m.prevPadPh[k] > TAU) m.prevPadPh[k] -= TAU;
                padPrev += organ(m.prevPadPh[k]);
            }
        }
        float padMix = (pad * m.chordFade + padPrev * (1.0f - m.chordFade)) * 0.030f;
        // Медленное дыхание громкости
        padMix *= 0.75f + 0.25f * std::sin(float(m.t) * 0.21f);

        // 3. Редкие колокола по пентатонике (в бою почти молчат)
        m.bellTimer -= dt;
        if (m.bellTimer <= 0.0f) {
            float period = menu ? 3.4f : (over ? 6.0f : 9.0f);
            m.bellTimer = period * (0.6f + std::fabs(m.rnd()) * 1.1f);
            for (auto& b : m.bells) {
                if (b.on) continue;
                int oct = 2 + int(std::fabs(m.rnd()) * 2.99f);
                int deg = int(std::fabs(m.rnd()) * 4.999f) % 5;
                b.f = noteHz(root * float(1 << oct), kPenta[deg]);
                b.ph = 0.0f; b.age = 0.0f; b.on = true;
                b.decay = 1.1f + std::fabs(m.rnd()) * 1.6f;
                b.amp = 0.10f + std::fabs(m.rnd()) * 0.07f;
                break;
            }
        }
        float bell = 0.0f;
        for (auto& b : m.bells) {
            if (!b.on) continue;
            b.age += dt;
            b.ph += TAU * b.f / SR;
            float e = std::exp(-b.age / b.decay);
            bell += (std::sin(b.ph) + 0.32f * std::sin(b.ph * 2.01f)) * b.amp * e;
            if (b.age > b.decay * 5.0f) b.on = false;
        }

        // 4. Ветер: низкочастотный шум
        float nz = m.rnd();
        m.windLp += (nz - m.windLp) * 0.010f;
        float wind = m.windLp * (combat ? 0.16f : 0.11f)
                   * (0.6f + 0.4f * std::sin(float(m.t) * 0.13f));

        // 5. В бою — медленный низкий пульс вместо колоколов
        float pulse = 0.0f;
        if (combat) {
            m.pulsePh += dt;
            if (m.pulsePh >= 1.45f) { m.pulsePh = 0.0f; m.pulseEnv = 1.0f; }
            m.pulseEnv *= 0.99985f;
            float pf = 46.0f;
            pulse = std::sin(float(m.t) * TAU * pf) * m.pulseEnv * 0.30f
                  * std::exp(-m.pulsePh * 7.0f);
        }

        float v = (drone + padMix + bell + wind + pulse) * m.gain;
        v = std::tanh(v * 1.2f) * 0.9f;
        int s = int(v * 20000.0f);
        out[i] = short(s > 32000 ? 32000 : (s < -32000 ? -32000 : s));
    }
}

} // namespace

// ── Public API ────────────────────────────────────────────────────────────
void init() {
    if (s_ready) return;
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;

    Noise rnd;
    for (int i = 0; i < int(Sfx::COUNT); ++i) {
        rnd.st = 0x9E3779B9u + uint32_t(i) * 2654435761u;
        s_sfx[i].load(makeSfx(Sfx(i), rnd));
    }
    for (int e = 0; e <= ELEMENT_COUNT; ++e) {
        rnd.st = 0x1B873593u + uint32_t(e) * 374761393u;
        s_elem[e].load(renderVoice(kVoices[e], rnd));
    }
    for (int k = 0; k < 8; ++k) {
        rnd.st = 0x27D4EB2Fu + uint32_t(k) * 2654435761u;
        s_enemy[k].load(renderVoice(kEnemyVoices[k], rnd));
    }

    SetAudioStreamBufferSizeDefault(2048);
    s_music.stream = LoadAudioStream(SR, 16, 1);
    SetAudioStreamCallback(s_music.stream, musicCallback);
    PlayAudioStream(s_music.stream);
    s_music.started = true;

    ::SetMasterVolume(s_master);
    s_ready = true;
}

void shutdown() {
    if (!s_ready) return;
    if (s_music.started) {
        StopAudioStream(s_music.stream);
        UnloadAudioStream(s_music.stream);
        s_music.started = false;
    }
    for (auto& s : s_sfx) s.unload();
    for (auto& s : s_elem) s.unload();
    for (auto& s : s_enemy) s.unload();
    CloseAudioDevice();
    s_ready = false;
}

bool ready() { return s_ready; }

void play(Sfx id, float volume, float pitch) {
    if (!s_ready || s_muted) return;
    int i = int(id);
    if (i < 0 || i >= int(Sfx::COUNT) || !s_sfx[i].loaded) return;
    Sound& s = s_sfx[i].pick();
    SetSoundVolume(s, clampf(volume, 0.0f, 1.0f));
    SetSoundPitch(s, clampf(pitch, 0.35f, 3.0f));
    PlaySound(s);
}

void playElement(ElementId e, float volume) {
    if (!s_ready || s_muted) return;
    int i = eidx(e);
    if (i < 0 || i > ELEMENT_COUNT || !s_elem[i].loaded) return;
    Sound& s = s_elem[i].pick();
    SetSoundVolume(s, clampf(volume, 0.0f, 1.0f));
    SetSoundPitch(s, 0.94f + (i % 5) * 0.02f);
    PlaySound(s);
}

void setTrack(Track t) {
    if (!s_ready) return;
    s_music.mode = int(t);
}

void update(float) { /* сглаживание живёт в колбэке */ }

void setMuted(bool m) {
    s_muted = m;
    if (s_ready) ::SetMasterVolume(m ? 0.0f : s_master);
}
bool muted() { return s_muted; }
float masterVolume() { return s_master; }
void setMasterVolume(float v) {
    s_master = clampf(v, 0.0f, 1.0f);
    if (s_ready && !s_muted) ::SetMasterVolume(s_master);
}

} // namespace audio
} // namespace shichoy

// ── Debug dump / Выгрузка синтеза в .wav ──────────────────────────────────
namespace shichoy {
namespace audio {

static void stats(const char* name, const std::vector<short>& d) {
    double sum = 0.0; int peak = 0;
    for (short v : d) { double f = v / 32768.0; sum += f * f; if (std::abs(int(v)) > peak) peak = std::abs(int(v)); }
    double rms = d.empty() ? 0.0 : std::sqrt(sum / double(d.size()));
    std::printf("%-16s %6.2f s  peak %5.3f  rms %5.3f%s\n",
                name, double(d.size()) / SR, peak / 32768.0, rms,
                peak >= 32700 ? "  <-- КЛИППИНГ" : (rms < 0.004 ? "  <-- ПОЧТИ ТИШИНА" : ""));
}

static void writeWav(const char* dir, const char* name, const std::vector<short>& d) {
    Wave w{};
    w.frameCount = unsigned(d.size());
    w.sampleRate = SR; w.sampleSize = 16; w.channels = 1;
    w.data = (short*)std::malloc(d.size() * sizeof(short));
    std::memcpy(w.data, d.data(), d.size() * sizeof(short));
    char path[512];
    std::snprintf(path, sizeof path, "%s/%s.wav", dir, name);
    ExportWave(w, path);
    UnloadWave(w);
}

void debugDump(const char* outDir) {
    Noise rnd;

    // Голоса стихий
    const char* elemNames[ELEMENT_COUNT + 1] = {
        "elem_none", "elem_fire", "elem_water", "elem_earth", "elem_air",
        "elem_lightning", "elem_ice", "elem_nature", "elem_shadow", "elem_metal",
        "elem_light", "elem_poison", "elem_aether", "elem_blood", "elem_storm",
        "elem_crystal", "elem_arcane"
    };
    std::printf("--- element voices ---\n");
    for (int e = 1; e <= ELEMENT_COUNT; ++e) {
        rnd.st = 0x1B873593u + uint32_t(e) * 374761393u;
        Buf b = renderVoice(kVoices[e], rnd);
        std::vector<short> d(b.n());
        for (size_t i = 0; i < b.n(); ++i) d[i] = short(clampf(b.s[i], -1.0f, 1.0f) * 32000.0f);
        stats(elemNames[e], d);
        writeWav(outDir, elemNames[e], d);
    }

    // Эффекты
    const char* sfxNames[] = {
        "ui_hover", "ui_click", "ui_back", "card_draw", "card_spell", "card_effect",
        "reaction", "hit", "miss", "player_hurt", "shield", "heal", "spot_break",
        "phase_shift", "victory", "defeat", "table_scrape", "table_thud", "gather",
        "encounter", "craft_start", "craft_success", "craft_fail", "craft_perfect", "step"
    };
    std::printf("--- sfx ---\n");
    for (int i = 0; i < int(Sfx::COUNT); ++i) {
        rnd.st = 0x9E3779B9u + uint32_t(i) * 2654435761u;
        Buf b = makeSfx(Sfx(i), rnd);
        std::vector<short> d(b.n());
        for (size_t k = 0; k < b.n(); ++k) d[k] = short(clampf(b.s[k], -1.0f, 1.0f) * 32000.0f);
        stats(sfxNames[i], d);
        writeWav(outDir, sfxNames[i], d);
    }

    // Музыка: гоняем колбэк вручную, без аудиоустройства
    std::printf("--- music ---\n");
    struct { Track t; const char* name; float secs; } tracks[] = {
        { Track::Menu,      "music_menu",      45.0f },
        { Track::Overworld, "music_overworld", 25.0f },
        { Track::Combat,    "music_combat",    25.0f },
    };
    for (auto& tr : tracks) {
        s_music = MusicState{};                 // чистое состояние на каждый трек
        s_music.mode = int(tr.t);
        s_music.gain = 0.0f;
        std::vector<short> d(size_t(tr.secs * SR));
        const unsigned chunk = 2048;
        for (size_t off = 0; off < d.size(); off += chunk) {
            unsigned n = unsigned(std::min<size_t>(chunk, d.size() - off));
            musicCallback(d.data() + off, n);
        }
        stats(tr.name, d);
        writeWav(outDir, tr.name, d);
    }
    s_music = MusicState{};
}

} // namespace audio
} // namespace shichoy

namespace shichoy {
namespace audio {

void playEnemyMove(int moveKind, ElementId flavour, float volume) {
    if (!s_ready || s_muted) return;
    int k = clampi(moveKind, 0, 7);
    if (s_enemy[k].loaded) {
        Sound& s = s_enemy[k].pick();
        SetSoundVolume(s, clampf(volume, 0.0f, 1.0f));
        SetSoundPitch(s, 0.92f + 0.10f * ((k % 3) - 1));
        PlaySound(s);
    }
    // Стихия врага подмешивается ниже и тише — это его окраска, не удар.
    int e = eidx(flavour);
    if (e > 0 && e <= ELEMENT_COUNT && s_elem[e].loaded) {
        Sound& s = s_elem[e].pick();
        SetSoundVolume(s, clampf(volume * 0.45f, 0.0f, 1.0f));
        SetSoundPitch(s, 0.62f);
        PlaySound(s);
    }
}

} // namespace audio
} // namespace shichoy
