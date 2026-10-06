// Mini-games / Мини-игры: короткие, разные, на разных органах управления.
// Каждая длится несколько секунд и кончается одним из трёх исходов:
// провал, успех, безупречно. Ими решается крафт, события и часть сделок.
#pragma once
#include "core/Types.h"

namespace shichoy {

enum class MiniGameId : int {
    Timing = 0,   // бегунок по полосе — остановить в зоне
    Seal,         // провести печать по воску с ровной скоростью
    SteadyHand,   // провести курсор по коридору, не задев стенок
    Candles,      // задуть свечи, чередуя A и D
    Reflex,       // ударить ровно в тот миг, когда вспыхнет сигил
    Sequence,     // повторить порядок вспышек цифрами
    Cipher,       // найти нужную руну в решётке похожих
    COUNT
};

class MiniGame {
public:
    // chance 0..0.70 задаёт снисходительность: ширину зоны, время, число жил.
    void start(MiniGameId id, float chance, Rng& r);
    void startRandom(float chance, Rng& r);
    void update(float dt, Rectangle area);
    void draw(Rectangle area, float t) const;
    void commit();                    // для игр, где исход решает одно нажатие
    void reset() { running_ = false; finished_ = false; }

    bool running() const { return running_; }
    bool finished() const { return finished_; }
    bool success() const { return success_; }
    bool perfect() const { return perfect_; }
    MiniGameId id() const { return id_; }
    LS title() const;
    LS hint() const;
    float timeLeft() const { return limit_ - timer_; }
    float timeLimit() const { return limit_; }

private:
    void finish(bool ok, bool great);
    void updTiming(float dt, Rectangle a);
    void updSeal(float dt, Rectangle a);
    void updSteady(float dt, Rectangle a);
    void updCandles(float dt, Rectangle a);
    void updReflex(float dt, Rectangle a);
    void updSequence(float dt, Rectangle a);
    void updCipher(float dt, Rectangle a);

    void drawTiming(Rectangle a, float t) const;
    void drawSeal(Rectangle a, float t) const;
    void drawSteady(Rectangle a, float t) const;
    void drawCandles(Rectangle a, float t) const;
    void drawReflex(Rectangle a, float t) const;
    void drawSequence(Rectangle a, float t) const;
    void drawCipher(Rectangle a, float t) const;

    MiniGameId id_ = MiniGameId::Timing;
    float chance_ = 0.5f;
    float timer_ = 0.0f, limit_ = 6.0f;
    bool  running_ = false, finished_ = false, success_ = false, perfect_ = false;
    Rng   rng_{ 1 };

    // ── Тайминг ───────────────────────────────────────────────────────────
    float marker_ = 0, dir_ = 1, speed_ = 1;
    float zoneA_ = 0.3f, zoneB_ = 0.7f, perfA_ = 0.47f, perfB_ = 0.53f;

    // ── Жилы ──────────────────────────────────────────────────────────────
    static constexpr int MAX_W = 5;

    // ── Печать ────────────────────────────────────────────────────────────
    float sealX_ = 0.0f, sealPrev_ = 0.0f, sealGood_ = 0.0f, sealBad_ = 0.0f;
    bool  sealHeld_ = false;
    float sealLo_ = 0.25f, sealHi_ = 0.55f;   // допустимая скорость, доли в секунду

    // ── Твёрдая рука ──────────────────────────────────────────────────────
    static constexpr int PATH_N = 9;
    Vector2 path_[PATH_N]{};
    float corridor_ = 0.08f;        // доля высоты поля, а не пиксели
    float steadyCool_ = 0.0f;       // пауза между засчитанными касаниями
    int   steadyNode_ = 0, steadyTouch_ = 0;
    bool  steadyStarted_ = false;
    bool  steadyOff_ = false;       // курсор вне поля — игра ждёт
    float corridorPx(Rectangle a) const;

    // ── Свечи ─────────────────────────────────────────────────────────────
    int   candleTotal_ = 5, candleOut_ = 0;
    int   candleLastKey_ = 0;       // 0 нет, 1 A, 2 D
    float candlePuff_ = 0.0f;

    // ── Миг ───────────────────────────────────────────────────────────────
    float reflexWait_ = 1.0f;
    bool  reflexLit_ = false;
    float reflexAt_ = 0.0f;

    // ── Наизусть ──────────────────────────────────────────────────────────
    static constexpr int SEQ_N = 6;
    int   seq_[SEQ_N]{};
    int   seqLen_ = 4, seqShow_ = 0, seqInput_ = 0;
    float seqTimer_ = 0.0f;
    bool  seqPhaseInput_ = false;
    int   seqFlash_ = -1;

    // ── Шифр ──────────────────────────────────────────────────────────────
    static constexpr int GRID_N = 16;
    int   grid_[GRID_N]{};
    int   cipherTarget_ = 0;
    int   cipherWrong_ = 0;
    int   cipherHover_ = -1;
};

} // namespace shichoy
