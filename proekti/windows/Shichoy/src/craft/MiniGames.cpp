#include "craft/MiniGames.h"
#include "art/ProcArt.h"
#include "audio/ProcAudio.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace shichoy {

// ── Общее ─────────────────────────────────────────────────────────────────
LS MiniGame::title() const {
    switch (id_) {
        case MiniGameId::Timing:     return LS("Верный удар", "The True Stroke");
        case MiniGameId::Wires:      return LS("Жилы", "The Veins");
        case MiniGameId::Seal:       return LS("Сургучная печать", "The Wax Seal");
        case MiniGameId::SteadyHand: return LS("Твёрдая рука", "A Steady Hand");
        case MiniGameId::Candles:    return LS("Задуть свечи", "Snuff the Candles");
        case MiniGameId::Reflex:     return LS("Миг", "The Instant");
        case MiniGameId::Sequence:   return LS("Наизусть", "By Heart");
        case MiniGameId::Cipher:     return LS("Шифр", "The Cipher");
        default:                     return LS("", "");
    }
}

LS MiniGame::hint() const {
    switch (id_) {
        case MiniGameId::Timing:
            return LS("SPACE или клик — остановить бегунок в зелёной полосе.",
                      "SPACE or click to stop the marker inside the green band.");
        case MiniGameId::Wires:
            return LS("Тяни жилу к гнезду того же цвета. Ошибка стоит времени.",
                      "Drag each vein to the socket of its colour. Mistakes cost time.");
        case MiniGameId::Seal:
            return LS("Зажми кнопку и веди печать вправо ровно — не рвись и не тяни.",
                      "Hold the button and draw the seal right at an even pace.");
        case MiniGameId::SteadyHand:
            return LS("Проведи курсор по коридору от начала до конца, не задевая стенок.",
                      "Guide the cursor along the corridor without touching the walls.");
        case MiniGameId::Candles:
            return LS("Чередуй A и D, чтобы раздуть мех и погасить все свечи.",
                      "Alternate A and D to work the bellows and snuff every candle.");
        case MiniGameId::Reflex:
            return LS("Жми SPACE ровно тогда, когда сигил вспыхнет. Раньше — провал.",
                      "Press SPACE the instant the sigil flares. Too early is a failure.");
        case MiniGameId::Sequence:
            return LS("Запомни порядок вспышек и повтори его цифрами 1–4.",
                      "Remember the order of the flashes and repeat it with 1–4.");
        case MiniGameId::Cipher:
            return LS("Найди в решётке руну, показанную слева, и щёлкни по ней.",
                      "Find the rune shown on the left and click it in the grid.");
        default: return LS("", "");
    }
}

void MiniGame::finish(bool ok, bool great) {
    running_ = false;
    finished_ = true;
    success_ = ok;
    perfect_ = ok && great;
}

void MiniGame::startRandom(float chance, Rng& r) {
    start(MiniGameId(r.i(0, int(MiniGameId::COUNT) - 1)), chance, r);
}

void MiniGame::start(MiniGameId id, float chance, Rng& r) {
    id_ = id;
    chance_ = clampf(chance, 0.10f, 0.70f);
    rng_.reseed(r.next());
    timer_ = 0.0f;
    running_ = true; finished_ = false; success_ = false; perfect_ = false;

    // Чем выше «шанс», тем мягче условия — но потолок всё равно давит.
    const float ease = (chance_ - 0.10f) / 0.60f;    // 0 строго … 1 мягко

    switch (id_) {
        case MiniGameId::Timing: {
            limit_ = 12.0f;
            float w = chance_;
            zoneA_ = rng_.f(0.05f, 0.95f - w);
            zoneB_ = zoneA_ + w;
            float pw = std::max(0.03f, w * 0.18f);
            float pc = (zoneA_ + zoneB_) * 0.5f + rng_.f(-w * 0.2f, w * 0.2f);
            perfA_ = clampf(pc - pw * 0.5f, zoneA_, zoneB_);
            perfB_ = clampf(pc + pw * 0.5f, zoneA_, zoneB_);
            marker_ = rng_.f(0.0f, 1.0f);
            dir_ = rng_.chance(0.5f) ? 1.0f : -1.0f;
            speed_ = 0.85f + (1.0f - chance_) * 0.9f;
            break;
        }
        case MiniGameId::Wires: {
            wireCount_ = 3 + (ease < 0.5f ? 2 : 1);
            limit_ = 7.0f + ease * 5.0f;
            // Цвета обязаны быть разными, иначе пара становится неоднозначной.
            int pool[MAX_W];
            for (int i = 0; i < wireCount_; ++i) {
                int c;
                bool dup;
                int guard = 0;
                do {
                    c = 1 + rng_.i(0, ELEMENT_COUNT - 1);
                    dup = false;
                    for (int k = 0; k < i; ++k) if (wireLeft_[k] == c) dup = true;
                } while (dup && ++guard < 64);
                wireLeft_[i] = c;
                pool[i] = c;
                wireLinked_[i] = -1;
            }
            for (int i = wireCount_ - 1; i > 0; --i) std::swap(pool[i], pool[rng_.i(0, i)]);
            for (int i = 0; i < wireCount_; ++i) wireRight_[i] = pool[i];
            wireDrag_ = -1;
            wireErrors_ = 0;
            break;
        }
        case MiniGameId::Seal: {
            limit_ = 9.0f;
            sealX_ = 0.0f; sealPrev_ = 0.0f; sealGood_ = 0.0f; sealBad_ = 0.0f;
            sealHeld_ = false;
            sealLo_ = 0.22f - ease * 0.06f;
            sealHi_ = 0.48f + ease * 0.22f;
            break;
        }
        case MiniGameId::SteadyHand: {
            limit_ = 12.0f;
            corridor_ = 16.0f + ease * 20.0f;
            steadyNode_ = 0; steadyTouch_ = 0; steadyStarted_ = false;
            for (int i = 0; i < PATH_N; ++i) {
                float u = float(i) / (PATH_N - 1);
                path_[i] = Vector2{ u, 0.5f + std::sin(u * 6.0f + rng_.f(0.0f, 3.0f)) * 0.26f
                                            * (0.5f + 0.5f * rng_.f(0.0f, 1.0f)) };
                path_[i].y = clampf(path_[i].y, 0.18f, 0.82f);
            }
            path_[0].y = 0.5f; path_[PATH_N - 1].y = 0.5f;
            break;
        }
        case MiniGameId::Candles: {
            candleTotal_ = 4 + int((1.0f - ease) * 4.0f);
            candleOut_ = 0;
            candleLastKey_ = 0;
            candlePuff_ = 0.0f;
            limit_ = 6.0f + ease * 3.0f;
            break;
        }
        case MiniGameId::Reflex: {
            limit_ = 8.0f;
            reflexWait_ = rng_.f(1.2f, 3.4f);
            reflexLit_ = false;
            reflexAt_ = 0.0f;
            break;
        }
        case MiniGameId::Sequence: {
            seqLen_ = 3 + int((1.0f - ease) * 3.0f);
            for (int i = 0; i < seqLen_; ++i) seq_[i] = rng_.i(0, 3);
            seqShow_ = 0; seqInput_ = 0; seqTimer_ = 0.0f;
            seqPhaseInput_ = false; seqFlash_ = -1;
            limit_ = 6.0f + seqLen_ * 2.2f;
            break;
        }
        case MiniGameId::Cipher: {
            limit_ = 4.0f + ease * 4.0f;
            for (int i = 0; i < GRID_N; ++i) grid_[i] = 1 + rng_.i(0, ELEMENT_COUNT - 1);
            cipherTarget_ = rng_.i(0, GRID_N - 1);
            // Похожие соседи: половину решётки заполняем близкими сигилами
            for (int i = 0; i < GRID_N; ++i)
                if (i != cipherTarget_ && rng_.chance(0.45f))
                    grid_[i] = grid_[cipherTarget_] % ELEMENT_COUNT + 1;
            cipherWrong_ = 0;
            cipherHover_ = -1;
            break;
        }
        default: break;
    }
}

void MiniGame::commit() {
    if (!running_) return;
    if (id_ == MiniGameId::Timing) {
        bool ok = (marker_ >= zoneA_ && marker_ <= zoneB_);
        finish(ok, ok && marker_ >= perfA_ && marker_ <= perfB_);
    } else if (id_ == MiniGameId::Reflex) {
        if (!reflexLit_) finish(false, false);                      // фальстарт
        else {
            float rt = timer_ - reflexAt_;
            finish(true, rt < 0.26f);
        }
    }
}

void MiniGame::update(float dt, Rectangle a) {
    if (!running_) return;
    timer_ += dt;
    if (timer_ >= limit_ && id_ != MiniGameId::Timing) { finish(false, false); return; }

    switch (id_) {
        case MiniGameId::Timing:     updTiming(dt, a); break;
        case MiniGameId::Wires:      updWires(dt, a); break;
        case MiniGameId::Seal:       updSeal(dt, a); break;
        case MiniGameId::SteadyHand: updSteady(dt, a); break;
        case MiniGameId::Candles:    updCandles(dt, a); break;
        case MiniGameId::Reflex:     updReflex(dt, a); break;
        case MiniGameId::Sequence:   updSequence(dt, a); break;
        case MiniGameId::Cipher:     updCipher(dt, a); break;
        default: break;
    }
}

// ── 1. Верный удар ────────────────────────────────────────────────────────
void MiniGame::updTiming(float dt, Rectangle) {
    marker_ += dir_ * speed_ * dt;
    if (marker_ > 1.0f) { marker_ = 1.0f; dir_ = -1.0f; }
    if (marker_ < 0.0f) { marker_ = 0.0f; dir_ = 1.0f; }
    if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) commit();
}

void MiniGame::drawTiming(Rectangle a, float t) const {
    Rectangle track{ a.x + 30, a.y + a.height * 0.5f - 14, a.width - 60, 28 };
    DrawRectangleRounded(track, 0.5f, 6, hex(0x141018));
    Rectangle zone{ track.x + track.width * zoneA_, track.y,
                    track.width * (zoneB_ - zoneA_), track.height };
    DrawRectangleRounded(zone, 0.5f, 6, withA(hex(0x2D6A4F), 200));
    Rectangle pz{ track.x + track.width * perfA_, track.y,
                  track.width * (perfB_ - perfA_), track.height };
    DrawRectangleRounded(pz, 0.5f, 6, withA(art::pal::GoldLit, 215));
    float mx = track.x + track.width * marker_;
    DrawRectangle(int(mx) - 2, int(track.y) - 9, 4, int(track.height) + 18, art::pal::Parchment);
    DrawRectangleRoundedLinesEx(track, 0.5f, 6, 1.4f, withA(art::pal::Gold, 160));
    (void)t;
}

// ── 2. Жилы ───────────────────────────────────────────────────────────────
static void wireEnds(Rectangle a, int i, int n, Vector2& l, Vector2& r) {
    float step = a.height / (n + 1.0f);
    l = Vector2{ a.x + 46.0f, a.y + step * (i + 1) };
    r = Vector2{ a.x + a.width - 46.0f, a.y + step * (i + 1) };
}

void MiniGame::updWires(float, Rectangle a) {
    Vector2 m = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (int i = 0; i < wireCount_; ++i) {
            if (wireLinked_[i] >= 0) continue;
            Vector2 l, r; wireEnds(a, i, wireCount_, l, r);
            if (CheckCollisionPointCircle(m, l, 20.0f)) { wireDrag_ = i; break; }
        }
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && wireDrag_ >= 0) {
        int hit = -1;
        for (int j = 0; j < wireCount_; ++j) {
            Vector2 l, r; wireEnds(a, j, wireCount_, l, r);
            if (CheckCollisionPointCircle(m, r, 22.0f)) { hit = j; break; }
        }
        if (hit >= 0) {
            bool taken = false;
            for (int k = 0; k < wireCount_; ++k) if (wireLinked_[k] == hit) taken = true;
            if (!taken && wireRight_[hit] == wireLeft_[wireDrag_]) {
                wireLinked_[wireDrag_] = hit;
                audio::play(audio::Sfx::UiClick, 0.6f, 1.2f);
            } else {
                wireErrors_++;
                timer_ += 1.2f;                       // ошибка стоит времени
                audio::play(audio::Sfx::UiBack, 0.6f);
            }
        }
        wireDrag_ = -1;
    }
    int done = 0;
    for (int i = 0; i < wireCount_; ++i) if (wireLinked_[i] >= 0) done++;
    if (done == wireCount_) finish(true, wireErrors_ == 0 && timer_ < limit_ * 0.6f);
}

void MiniGame::drawWires(Rectangle a, float t) const {
    Vector2 m = GetMousePosition();
    for (int i = 0; i < wireCount_; ++i) {
        Vector2 l, r; wireEnds(a, i, wireCount_, l, r);
        Color cl = elem(ElementId(wireLeft_[i])).primary;
        Color cr = elem(ElementId(wireRight_[i])).primary;
        // Гнёзда
        DrawCircleV(l, 14.0f, hex(0x141018));
        DrawCircleV(r, 14.0f, hex(0x141018));
        DrawCircleLines(int(l.x), int(l.y), 14.0f, cl);
        DrawCircleLines(int(r.x), int(r.y), 14.0f, cr);
        DrawCircleV(l, 7.0f, cl);
        DrawCircleV(r, 7.0f, cr);
        // Протянутая жила провисает
        if (wireLinked_[i] >= 0) {
            Vector2 l2, r2; wireEnds(a, wireLinked_[i], wireCount_, l2, r2);
            for (int s = 0; s < 12; ++s) {
                float u0 = s / 12.0f, u1 = (s + 1) / 12.0f;
                auto pt = [&](float u) {
                    return Vector2{ l.x + (r2.x - l.x) * u,
                                    l.y + (r2.y - l.y) * u + std::sin(u * PI) * 16.0f };
                };
                DrawLineEx(pt(u0), pt(u1), 4.0f, cl);
            }
        } else if (wireDrag_ == i) {
            DrawLineEx(l, m, 4.0f, withA(cl, 220));
        }
    }
    (void)t;
}

// ── 3. Сургучная печать ───────────────────────────────────────────────────
void MiniGame::updSeal(float dt, Rectangle a) {
    Vector2 m = GetMousePosition();
    Rectangle track{ a.x + 40, a.y + a.height * 0.5f - 22, a.width - 80, 44 };
    float mx = clampf((m.x - track.x) / track.width, 0.0f, 1.0f);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && std::fabs(mx - sealX_) < 0.12f)
        sealHeld_ = true;
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) sealHeld_ = false;

    if (sealHeld_) {
        sealPrev_ = sealX_;
        sealX_ = std::max(sealX_, mx);               // назад печать не ведут
        float v = (sealX_ - sealPrev_) / std::max(0.0001f, dt);
        if (v >= sealLo_ && v <= sealHi_) sealGood_ += dt;
        else if (sealX_ < 0.995f)         sealBad_ += dt;
    }
    if (sealX_ >= 0.995f) {
        bool ok = sealBad_ < sealGood_ * 0.55f;
        finish(ok, ok && sealBad_ < sealGood_ * 0.15f);
    }
}

void MiniGame::drawSeal(Rectangle a, float t) const {
    Rectangle track{ a.x + 40, a.y + a.height * 0.5f - 22, a.width - 80, 44 };
    DrawRectangleRounded(track, 0.25f, 6, hex(0x2A1C1E));
    // Уже пройденный воск
    DrawRectangleRounded(Rectangle{ track.x, track.y, track.width * sealX_, track.height },
                         0.25f, 6, withA(art::pal::Blood, 190));
    DrawRectangleRoundedLinesEx(track, 0.25f, 6, 1.4f, withA(art::pal::Gold, 160));
    // Сама печать
    Vector2 s{ track.x + track.width * sealX_, track.y + track.height * 0.5f };
    DrawCircleV(s, 19.0f, lerpC(art::pal::Gold, art::pal::Blood, 0.35f));
    art::drawRuneRing(s, 13.0f, 6, art::pal::GoldLit, t * 2.0f);
    DrawCircleLines(int(s.x), int(s.y), 19.0f, art::pal::Ink);
    // Показатель ровности хода
    float bar = sealGood_ + sealBad_;
    if (bar > 0.01f) {
        Rectangle q{ track.x, track.y + track.height + 12, track.width, 8 };
        DrawRectangleRounded(q, 0.5f, 5, hex(0x141018));
        DrawRectangleRounded(Rectangle{ q.x, q.y, q.width * (sealGood_ / bar), q.height },
                             0.5f, 5, hex(0x2D6A4F));
    }
}

// ── 4. Твёрдая рука ───────────────────────────────────────────────────────
static Vector2 pathPoint(const Vector2* p, int i, Rectangle a) {
    return Vector2{ a.x + 30 + p[i].x * (a.width - 60), a.y + p[i].y * a.height };
}

void MiniGame::updSteady(float, Rectangle a) {
    Vector2 m = GetMousePosition();
    Vector2 start = pathPoint(path_, 0, a);
    if (!steadyStarted_) {
        if (CheckCollisionPointCircle(m, start, corridor_)) steadyStarted_ = true;
        return;
    }
    // Ближайший отрезок коридора
    float best = 1e9f;
    int bestSeg = 0;
    for (int i = 0; i < PATH_N - 1; ++i) {
        Vector2 p0 = pathPoint(path_, i, a), p1 = pathPoint(path_, i + 1, a);
        float dx = p1.x - p0.x, dy = p1.y - p0.y;
        float len2 = dx * dx + dy * dy;
        float u = len2 > 0.0001f ? clampf(((m.x - p0.x) * dx + (m.y - p0.y) * dy) / len2, 0, 1) : 0;
        float px = p0.x + dx * u, py = p0.y + dy * u;
        float d = std::sqrt((m.x - px) * (m.x - px) + (m.y - py) * (m.y - py));
        if (d < best) { best = d; bestSeg = i; }
    }
    if (bestSeg > steadyNode_) steadyNode_ = bestSeg;
    if (best > corridor_) {
        steadyTouch_++;
        audio::play(audio::Sfx::Miss, 0.35f);
        steadyStarted_ = false;                       // сорвался — начни заново
        steadyNode_ = 0;
        if (steadyTouch_ >= 3) finish(false, false);
        return;
    }
    Vector2 end = pathPoint(path_, PATH_N - 1, a);
    if (CheckCollisionPointCircle(m, end, corridor_) && steadyNode_ >= PATH_N - 3)
        finish(true, steadyTouch_ == 0);
}

void MiniGame::drawSteady(Rectangle a, float t) const {
    for (int i = 0; i < PATH_N - 1; ++i) {
        Vector2 p0 = pathPoint(path_, i, a), p1 = pathPoint(path_, i + 1, a);
        DrawLineEx(p0, p1, corridor_ * 2.0f, withA(hex(0x1C2A22), 220));
        DrawLineEx(p0, p1, corridor_ * 2.0f - 4.0f, hex(0x0E1612));
        DrawLineEx(p0, p1, 1.2f, withA(art::pal::Gold, 70));
    }
    Vector2 s = pathPoint(path_, 0, a), e = pathPoint(path_, PATH_N - 1, a);
    float pulse = 0.5f + 0.5f * std::sin(t * 4.0f);
    DrawCircleLines(int(s.x), int(s.y), corridor_,
                    steadyStarted_ ? withA(art::pal::Gold, 120)
                                   : withA(art::pal::GoldLit, (unsigned char)(140 + 110 * pulse)));
    DrawCircleLines(int(e.x), int(e.y), corridor_, withA(hex(0x59C772), 200));
    art::textC(g_lang == Lang::Ru ? "старт" : "start", s.x, s.y - corridor_ - 16, 11.0f,
               withA(art::pal::Vellum, 200));
    art::textC(g_lang == Lang::Ru ? "конец" : "end", e.x, e.y - corridor_ - 16, 11.0f,
               withA(art::pal::Vellum, 200));
    for (int i = 0; i < steadyTouch_; ++i)
        DrawCircleV(Vector2{ a.x + 18.0f + i * 16.0f, a.y + 16.0f }, 5.0f, art::pal::Risk);
}

// ── 5. Задуть свечи ───────────────────────────────────────────────────────
void MiniGame::updCandles(float dt, Rectangle) {
    candlePuff_ = std::max(0.0f, candlePuff_ - dt * 3.0f);
    int key = 0;
    if (IsKeyPressed(KEY_A)) key = 1;
    if (IsKeyPressed(KEY_D)) key = 2;
    if (key != 0) {
        if (key != candleLastKey_) {                  // засчитывается чередование
            candleLastKey_ = key;
            candlePuff_ = 1.0f;
            candleOut_++;
            audio::play(audio::Sfx::Step, 0.5f, 1.4f);
            if (candleOut_ >= candleTotal_)
                finish(true, timer_ < limit_ * 0.5f);
        } else {
            audio::play(audio::Sfx::UiBack, 0.35f);   // два раза подряд — мимо
        }
    }
}

void MiniGame::drawCandles(Rectangle a, float t) const {
    float step = a.width / (candleTotal_ + 1.0f);
    for (int i = 0; i < candleTotal_; ++i) {
        float x = a.x + step * (i + 1);
        float base = a.y + a.height * 0.72f;
        DrawRectangleRounded(Rectangle{ x - 7, base - 46, 14, 46 }, 0.3f, 5, art::pal::Vellum);
        DrawEllipse(int(x), int(base), 12, 4, hex(0x3A3026));
        if (i >= candleOut_) {
            float fl = 0.75f + 0.25f * std::sin(t * 11.0f + i * 1.7f);
            DrawCircleGradient(int(x), int(base - 56), 26 * fl, withA(hex(0xFFBA08), 110), BLANK);
            DrawTriangle(Vector2{ x, base - 46 - 20 * fl }, Vector2{ x - 6, base - 46 },
                         Vector2{ x + 6, base - 46 }, hex(0xFFD60A));
            DrawTriangle(Vector2{ x, base - 46 - 11 * fl }, Vector2{ x - 3, base - 46 },
                         Vector2{ x + 3, base - 46 }, hex(0xFFF3B0));
        } else {
            DrawLineEx(Vector2{ x, base - 46 }, Vector2{ x + 3, base - 54 }, 1.6f,
                       withA(art::pal::Fog, 150));
        }
    }
    // Мех: подсказка, какую клавишу жать следующей
    const char* nextKey = (candleLastKey_ == 1) ? "D" : "A";
    float pw = 26.0f + candlePuff_ * 14.0f;
    Rectangle bell{ a.x + a.width * 0.5f - pw, a.y + a.height - 40.0f, pw * 2, 26.0f };
    DrawRectangleRounded(bell, 0.5f, 6, withA(art::pal::Gold, 60));
    DrawRectangleRoundedLinesEx(bell, 0.5f, 6, 1.4f, art::pal::Gold);
    art::textC(nextKey, bell.x + bell.width * 0.5f, bell.y + 5.0f, 16.0f, art::pal::GoldLit);
}

// ── 6. Миг ────────────────────────────────────────────────────────────────
void MiniGame::updReflex(float, Rectangle) {
    if (!reflexLit_ && timer_ >= reflexWait_) {
        reflexLit_ = true;
        reflexAt_ = timer_;
        audio::play(audio::Sfx::UiHover, 0.8f, 1.5f);
    }
    if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) commit();
    if (reflexLit_ && timer_ - reflexAt_ > 1.1f) finish(false, false);
}

void MiniGame::drawReflex(Rectangle a, float t) const {
    Vector2 c{ a.x + a.width * 0.5f, a.y + a.height * 0.5f };
    if (reflexLit_) {
        float age = timer_ - reflexAt_;
        float k = clampf(1.0f - age, 0.0f, 1.0f);
        DrawCircleGradient(int(c.x), int(c.y), 150 * k, withA(art::pal::GoldLit, 150), BLANK);
        art::drawSigil(ElementId::Light, c, 54.0f, art::pal::GoldLit, t);
        art::textC(g_lang == Lang::Ru ? "СЕЙЧАС" : "NOW", c.x, c.y + 66.0f, 20.0f, art::pal::GoldLit);
    } else {
        DrawCircleLines(int(c.x), int(c.y), 54.0f, withA(art::pal::Gold, 90));
        art::textC(g_lang == Lang::Ru ? "ждите…" : "wait…", c.x, c.y - 8.0f, 16.0f,
                   withA(art::pal::Vellum, 150));
    }
}

// ── 7. Наизусть ───────────────────────────────────────────────────────────
void MiniGame::updSequence(float dt, Rectangle) {
    if (!seqPhaseInput_) {
        seqTimer_ += dt;
        if (seqTimer_ > 0.55f) {
            seqTimer_ = 0.0f;
            if (seqShow_ < seqLen_) {
                seqFlash_ = seq_[seqShow_++];
                audio::playElement(ElementId(1 + seqFlash_ * 4), 0.5f);
            } else {
                seqFlash_ = -1;
                seqPhaseInput_ = true;
            }
        } else if (seqTimer_ > 0.35f) seqFlash_ = -1;
        return;
    }
    for (int k = 0; k < 4; ++k) {
        if (!IsKeyPressed(KEY_ONE + k)) continue;
        seqFlash_ = k;
        seqTimer_ = 0.0f;
        if (seq_[seqInput_] == k) {
            seqInput_++;
            audio::play(audio::Sfx::UiClick, 0.5f, 1.0f + k * 0.12f);
            if (seqInput_ >= seqLen_) finish(true, timer_ < limit_ * 0.75f);
        } else {
            audio::play(audio::Sfx::UiBack, 0.7f);
            finish(false, false);
        }
    }
}

void MiniGame::drawSequence(Rectangle a, float t) const {
    static const ElementId cols[4] = { ElementId::Fire, ElementId::Water,
                                       ElementId::Nature, ElementId::Arcane };
    float w = std::min(110.0f, (a.width - 60.0f) / 4.0f);
    float x0 = a.x + (a.width - w * 4 - 30) * 0.5f;
    for (int i = 0; i < 4; ++i) {
        Rectangle r{ x0 + i * (w + 10), a.y + a.height * 0.30f, w, a.height * 0.42f };
        bool lit = (seqFlash_ == i);
        Color c = elem(cols[i]).primary;
        DrawRectangleRounded(r, 0.18f, 6, lit ? c : scaleC(c, 0.32f));
        DrawRectangleRoundedLinesEx(r, 0.18f, 6, lit ? 3.0f : 1.4f,
                                    lit ? WHITE : withA(c, 170));
        art::drawSigil(cols[i], Vector2{ r.x + r.width * 0.5f, r.y + r.height * 0.45f },
                       r.width * 0.28f, lit ? WHITE : withA(c, 200), t);
        char n[4]; snprintf(n, sizeof n, "%d", i + 1);
        art::textC(n, r.x + r.width * 0.5f, r.y + r.height - 22.0f, 15.0f,
                   lit ? WHITE : withA(art::pal::Parchment, 200));
    }
    const char* ph = seqPhaseInput_ ? (g_lang == Lang::Ru ? "повторите" : "repeat it")
                                    : (g_lang == Lang::Ru ? "запоминайте" : "watch");
    art::textC(ph, a.x + a.width * 0.5f, a.y + 6.0f, 14.0f, withA(art::pal::GoldLit, 220));
    for (int i = 0; i < seqLen_; ++i) {
        Vector2 p{ a.x + a.width * 0.5f - (seqLen_ - 1) * 9.0f + i * 18.0f, a.y + a.height - 16.0f };
        DrawCircleV(p, 5.0f, i < seqInput_ ? art::pal::GoldLit : withA(art::pal::Gold, 90));
    }
}

// ── 8. Шифр ───────────────────────────────────────────────────────────────
void MiniGame::updCipher(float, Rectangle a) {
    Vector2 m = GetMousePosition();
    float cell = std::min((a.width - 230.0f) / 4.0f, a.height / 4.0f) - 4.0f;
    float gx = a.x + 196.0f, gy = a.y + (a.height - cell * 4) * 0.5f;
    cipherHover_ = -1;
    for (int i = 0; i < GRID_N; ++i) {
        Rectangle r{ gx + (i % 4) * cell, gy + (i / 4) * cell, cell - 6, cell - 6 };
        if (!CheckCollisionPointRec(m, r)) continue;
        cipherHover_ = i;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (i == cipherTarget_) finish(true, cipherWrong_ == 0 && timer_ < limit_ * 0.5f);
            else {
                cipherWrong_++;
                timer_ += 0.9f;
                audio::play(audio::Sfx::UiBack, 0.6f);
                if (cipherWrong_ >= 3) finish(false, false);
            }
        }
    }
}

void MiniGame::drawCipher(Rectangle a, float t) const {
    // Образец слева
    Vector2 sample{ a.x + 96.0f, a.y + a.height * 0.5f };
    DrawCircleV(sample, 46.0f, hex(0x141018));
    DrawCircleLines(int(sample.x), int(sample.y), 46.0f, art::pal::Gold);
    art::drawSigil(ElementId(grid_[cipherTarget_]), sample, 30.0f,
                   elem(ElementId(grid_[cipherTarget_])).primary, t);
    art::textC(g_lang == Lang::Ru ? "найдите" : "find this", sample.x, sample.y + 54.0f, 12.0f,
               withA(art::pal::Vellum, 210));

    float cell = std::min((a.width - 230.0f) / 4.0f, a.height / 4.0f) - 4.0f;
    float gx = a.x + 196.0f, gy = a.y + (a.height - cell * 4) * 0.5f;
    for (int i = 0; i < GRID_N; ++i) {
        Rectangle r{ gx + (i % 4) * cell, gy + (i / 4) * cell, cell - 6, cell - 6 };
        bool hov = (cipherHover_ == i);
        DrawRectangleRounded(r, 0.12f, 5, hov ? hex(0x241C2E) : hex(0x14101A));
        DrawRectangleRoundedLinesEx(r, 0.12f, 5, hov ? 2.0f : 1.0f,
                                    withA(art::pal::Gold, hov ? 220 : 110));
        art::drawSigil(ElementId(grid_[i]),
                       Vector2{ r.x + r.width * 0.5f, r.y + r.height * 0.5f },
                       r.width * 0.32f, elem(ElementId(grid_[i])).primary, t * 0.4f);
    }
    for (int i = 0; i < cipherWrong_; ++i)
        DrawCircleV(Vector2{ a.x + 18.0f + i * 16.0f, a.y + 16.0f }, 5.0f, art::pal::Risk);
}

// ── Отрисовка ─────────────────────────────────────────────────────────────
void MiniGame::draw(Rectangle a, float t) const {
    switch (id_) {
        case MiniGameId::Timing:     drawTiming(a, t); break;
        case MiniGameId::Wires:      drawWires(a, t); break;
        case MiniGameId::Seal:       drawSeal(a, t); break;
        case MiniGameId::SteadyHand: drawSteady(a, t); break;
        case MiniGameId::Candles:    drawCandles(a, t); break;
        case MiniGameId::Reflex:     drawReflex(a, t); break;
        case MiniGameId::Sequence:   drawSequence(a, t); break;
        case MiniGameId::Cipher:     drawCipher(a, t); break;
        default: break;
    }
    // Песочные часы: сколько осталось
    if (running_ && limit_ > 0.0f && id_ != MiniGameId::Timing) {
        float k = clampf(1.0f - timer_ / limit_, 0.0f, 1.0f);
        Rectangle tb{ a.x + 8, a.y + a.height - 7, (a.width - 16) * k, 4 };
        DrawRectangleRounded(Rectangle{ a.x + 8, a.y + a.height - 7, a.width - 16, 4 }, 0.5f, 4,
                             hex(0x141018));
        DrawRectangleRounded(tb, 0.5f, 4, k > 0.3f ? art::pal::Gold : art::pal::Risk);
    }
}

} // namespace shichoy
