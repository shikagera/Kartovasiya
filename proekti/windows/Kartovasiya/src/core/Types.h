// Core types / Базовые типы — Kartovasiya (Картовасия)
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <random>
#include "raylib.h"

namespace kv {

// ── Language / Язык ───────────────────────────────────────────────────────
enum class Lang { Ru = 0, En = 1 };
extern Lang g_lang;

// Bilingual literal. Хранит обе строки, отдаёт активную.
struct L {
    const char* ru = "";
    const char* en = "";
    constexpr L() = default;
    constexpr L(const char* r, const char* e) : ru(r), en(e) {}
    const char* c_str() const { return g_lang == Lang::Ru ? ru : en; }
    operator const char*() const { return c_str(); }
};

// Runtime bilingual string (for procedurally built names).
struct LS {
    std::string ru, en;
    LS() = default;
    LS(std::string r, std::string e) : ru(std::move(r)), en(std::move(e)) {}
    LS(const L& l) : ru(l.ru), en(l.en) {}
    const char* c_str() const { return (g_lang == Lang::Ru ? ru : en).c_str(); }
    const std::string& str() const { return g_lang == Lang::Ru ? ru : en; }
    bool empty() const { return ru.empty() && en.empty(); }
};

// ── Elements / Стихии (16) ────────────────────────────────────────────────
enum class ElementId : int {
    None = 0,
    Fire = 1, Water, Earth, Air, Lightning, Ice, Nature, Shadow,      // basic
    Metal = 9, Light, Poison, Aether, Blood, Storm, Crystal, Arcane   // extended
};
constexpr int ELEMENT_COUNT = 16;
inline int eidx(ElementId e) { return static_cast<int>(e); }

enum class ElementTier : int { Basic = 0, Extended = 1 };

enum class CardRarity : int { Common = 0, Uncommon, Rare, Epic, Legendary, Mythic };
constexpr int RARITY_COUNT = 6;

// Spell — прямое действие; Element — метка/реакция; Effect — немагический статус.
enum class CardType : int { Element = 0, Spell = 1, Effect = 2 };

enum class CardFlags : uint32_t {
    None      = 0,
    Wild      = 1u << 0,   // подстраивается под метку врага
    Corrupted = 1u << 1,   // сильнее, но с риском
    Mutated   = 1u << 2,   // мутировала при крафте/событии
    Crafted   = 1u << 3,
    Precise   = 1u << 4,   // бьёт по выбранной точке без штрафа
};
inline CardFlags operator|(CardFlags a, CardFlags b) {
    return static_cast<CardFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline CardFlags& operator|=(CardFlags& a, CardFlags b) { a = a | b; return a; }
inline bool has(CardFlags v, CardFlags f) {
    return (static_cast<uint32_t>(v) & static_cast<uint32_t>(f)) != 0;
}

// ── RNG / Генератор случайности ───────────────────────────────────────────
class Rng {
public:
    explicit Rng(uint64_t seed = 0x9E3779B97F4A7C15ull) : s_(seed ? seed : 1) {}
    void reseed(uint64_t seed) { s_ = seed ? seed : 1; }
    uint64_t next() {                       // splitmix64
        uint64_t z = (s_ += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }
    int    i(int lo, int hi) { return hi <= lo ? lo : lo + int(next() % uint64_t(hi - lo + 1)); }
    float  f() { return float(next() >> 11) * (1.0f / 9007199254740992.0f); }
    float  f(float lo, float hi) { return lo + f() * (hi - lo); }
    bool   chance(float p) { return f() < p; }
    template <class T> const T& pick(const std::vector<T>& v) { return v[size_t(i(0, int(v.size()) - 1))]; }
    template <class T, size_t N> const T& pick(const std::array<T, N>& v) { return v[size_t(i(0, int(N) - 1))]; }
    uint64_t state() const { return s_; }
private:
    uint64_t s_;
};
extern Rng g_rng;   // global run RNG / общий ГПСЧ забега

// ── Small helpers / Мелкие утилиты ────────────────────────────────────────
inline Color hex(uint32_t rgb, unsigned char a = 255) {
    return Color{ (unsigned char)((rgb >> 16) & 0xFF), (unsigned char)((rgb >> 8) & 0xFF),
                  (unsigned char)(rgb & 0xFF), a };
}
inline Color withA(Color c, unsigned char a) { c.a = a; return c; }
inline Color lerpC(Color a, Color b, float t) {
    return Color{ (unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t),
                  (unsigned char)(a.b + (b.b - a.b) * t), (unsigned char)(a.a + (b.a - a.a) * t) };
}
inline Color scaleC(Color c, float k) {
    auto cl = [](float v) { return (unsigned char)(v < 0 ? 0 : (v > 255 ? 255 : v)); };
    return Color{ cl(c.r * k), cl(c.g * k), cl(c.b * k), c.a };
}
inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline int   clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

} // namespace kv
