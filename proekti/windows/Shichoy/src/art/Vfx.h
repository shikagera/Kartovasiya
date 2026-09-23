// Screen VFX / Эффекты по краям экрана при розыгрыше карты.
// Сыграл Огонь — по краям встаёт пламя; слил Огонь с Тенью в Тёмный огонь —
// то же пламя, но в цвете реакции, и поверх ползут тени. Стихии
// накладываются, поэтому составная карта выглядит как сумма своих частей.
#pragma once
#include "core/Cards.h"

namespace shichoy {
namespace art {

struct CardFx {
    ElementId a = ElementId::None;   // ведущая стихия
    ElementId b = ElementId::None;   // вторая (у реакций и составных карт)
    Color tint = WHITE;              // цвет реакции; для чистой стихии — её собственный
    float life = 0.0f, maxLife = 1.0f;
    float power = 1.0f;              // 1 — обычная карта, больше — реакция/Ultima
    bool  reaction = false;
};

class ScreenFx {
public:
    void trigger(ElementId a, ElementId b, Color tint, float power, bool reaction);
    void update(float dt);
    void draw(int sw, int sh, float t) const;
    void clear() { fx_.clear(); }
    bool active() const { return !fx_.empty(); }
    // Насколько сильно сейчас трясёт и светится — для общего тонирования сцены.
    float intensity() const;

private:
    std::vector<CardFx> fx_;
};

} // namespace art
} // namespace shichoy
