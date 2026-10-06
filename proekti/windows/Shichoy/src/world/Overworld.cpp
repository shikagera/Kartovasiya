#include "world/Overworld.h"
#include "art/ProcArt.h"
#include "audio/ProcAudio.h"
#include <algorithm>
#include <cmath>

namespace shichoy {

using art::hash01;
using art::fbm;

bool tileSolid(Tile t) {
    return t == Tile::Water || t == Tile::Rock || t == Tile::Tree || t == Tile::Bramble;
}

Tile Overworld::at(int x, int y) const {
    if (x < 0 || y < 0 || x >= MAP_W || y >= MAP_H) return Tile::Rock;
    return map_[size_t(y) * MAP_W + size_t(x)];
}

bool Overworld::blocked(float x, float y) const {
    return tileSolid(at(int(std::floor(x / TILE)), int(std::floor(y / TILE))));
}

void Overworld::carvePath(Rng& r, int x0, int y0, int x1, int y1) {
    int x = x0, y = y0;
    int guard = 0;
    while ((x != x1 || y != y1) && guard++ < 4000) {
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                int px = clampi(x + dx, 1, MAP_W - 2), py = clampi(y + dy, 1, MAP_H - 2);
                if (std::abs(dx) + std::abs(dy) <= 1)
                    map_[size_t(py) * MAP_W + size_t(px)] = Tile::Path;
            }
        if (x != x1 && (y == y1 || r.chance(0.55f))) x += (x1 > x) ? 1 : -1;
        else if (y != y1)                            y += (y1 > y) ? 1 : -1;
    }
}

void Overworld::generate(int depth, Rng& outer) {
    generate(depth, uint32_t(outer.next()));
}

void Overworld::generate(int depth, uint32_t seed) {
    Rng r(seed ? seed : 1u);
    depth_ = std::max(1, depth);
    seed_ = seed;
    map_.assign(size_t(MAP_W) * MAP_H, Tile::Grass);
    ents_.clear();
    encounter_ = -1;

    // Рельеф — два слоя шума: влага и «порча» (чем глубже, тем темнее биом).
    float corrupt = clampf(0.10f + depth_ * 0.045f, 0.10f, 0.62f);
    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            float n = fbm(x * 0.07f, y * 0.07f, seed_, 4);
            float m = fbm(x * 0.13f + 40.0f, y * 0.13f - 21.0f, seed_ ^ 0x5A5Au, 3);
            Tile t = Tile::Grass;
            if (n < 0.30f)                       t = Tile::Water;
            else if (n < 0.36f)                  t = Tile::Mud;
            else if (n > 0.74f)                  t = Tile::Rock;
            else if (m > 0.68f)                  t = Tile::Tree;
            else if (m < 0.30f - corrupt * 0.1f) t = Tile::Bramble;
            else if (m > 0.5f + corrupt * 0.2f)  t = Tile::DarkGrass;
            // Порча ложится плавными пятнами, а не шахматкой.
            float rot = fbm(x * 0.05f - 13.0f, y * 0.05f + 8.0f, seed_ + 7u, 3);
            if (t == Tile::Grass && rot < 0.34f + corrupt * 0.30f) t = Tile::DarkGrass;
            if (t == Tile::DarkGrass && hash01(x, y, seed_ + 11u) < 0.02f) t = Tile::Grave;
            map_[size_t(y) * MAP_W + size_t(x)] = t;
        }
    }

    // Границы — непроходимый камень
    for (int x = 0; x < MAP_W; ++x) { map_[size_t(x)] = Tile::Rock; map_[size_t(MAP_H - 1) * MAP_W + size_t(x)] = Tile::Rock; }
    for (int y = 0; y < MAP_H; ++y) { map_[size_t(y) * MAP_W] = Tile::Rock; map_[size_t(y) * MAP_W + MAP_W - 1] = Tile::Rock; }

    // Тропы соединяют ключевые точки
    int cx = MAP_W / 2, cy = MAP_H / 2;
    carvePath(r, cx, cy, r.i(6, MAP_W - 7), r.i(6, 14));
    carvePath(r, cx, cy, r.i(6, MAP_W - 7), r.i(MAP_H - 15, MAP_H - 7));
    carvePath(r, cx, cy, r.i(6, 14), r.i(6, MAP_H - 7));

    player_ = Vector2{ float(cx * TILE + TILE / 2), float(cy * TILE + TILE / 2) };
    cam_.target = player_;
    cam_.offset = Vector2{ GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f };
    cam_.rotation = 0.0f;
    cam_.zoom = 2.1f;

    placeEntities(r);
}

void Overworld::placeEntities(Rng& r) {
    auto freeSpot = [&](Vector2& out, float minDistFromPlayer) {
        for (int tries = 0; tries < 400; ++tries) {
            int x = r.i(3, MAP_W - 4), y = r.i(3, MAP_H - 4);
            if (tileSolid(at(x, y))) continue;
            Vector2 p{ float(x * TILE + TILE / 2), float(y * TILE + TILE / 2) };
            float dx = p.x - player_.x, dy = p.y - player_.y;
            if (dx * dx + dy * dy < minDistFromPlayer * minDistFromPlayer) continue;
            out = p;
            return true;
        }
        return false;
    };

    // Ресурсы
    int resources = 16 + depth_ * 2;
    for (int i = 0; i < resources; ++i) {
        WorldEnt e;
        e.kind = EntKind::Resource;
        e.data = r.i(0, MATERIAL_COUNT - 1);
        e.seed = uint32_t(r.next());
        if (!freeSpot(e.pos, 90.0f)) continue;
        e.name = LS(material(e.data).name.ru, material(e.data).name.en);
        ents_.push_back(e);
    }
    // NPC
    for (int i = 0; i < r.i(2, 3); ++i) {
        WorldEnt e;
        e.kind = EntKind::Npc;
        e.data = r.i(0, 2);   // 0 знахарь, 1 гравёр карт, 2 скупщик
        e.seed = uint32_t(r.next());
        if (!freeSpot(e.pos, 160.0f)) continue;
        static const LS names[3] = {
            LS("Знахарка у костра", "Herbwife by the Fire"),
            LS("Гравёр карт", "Card Engraver"),
            LS("Скупщик реликвий", "Relic Buyer")
        };
        e.name = names[e.data];
        ents_.push_back(e);
    }
    // События
    for (int i = 0; i < r.i(3, 5); ++i) {
        WorldEnt e;
        e.kind = EntKind::EventSite;
        e.data = r.i(0, 3);
        e.seed = uint32_t(r.next());
        if (!freeSpot(e.pos, 150.0f)) continue;
        static const LS names[4] = {
            LS("Часовня без крыши", "Roofless Chapel"),
            LS("Колодец шёпотов", "Whispering Well"),
            LS("Опрокинутая карета", "Overturned Carriage"),
            LS("Круг из камней", "Ring of Stones")
        };
        e.name = names[e.data];
        ents_.push_back(e);
    }
    // Руины с боссом
    for (int i = 0; i < (depth_ % 3 == 0 ? 2 : 1); ++i) {
        WorldEnt e;
        e.kind = EntKind::RuinGate;
        e.boss = true;
        e.seed = uint32_t(r.next());
        if (!freeSpot(e.pos, 420.0f)) continue;
        e.name = LS("Руины поместья", "Manor Ruins");
        ents_.push_back(e);
    }
    // Карета — выход из забега
    {
        WorldEnt e;
        e.kind = EntKind::Carriage;
        e.seed = uint32_t(r.next());
        if (freeSpot(e.pos, 260.0f)) {
            e.name = LS("Ваша карета", "Your Carriage");
            ents_.push_back(e);
        }
    }
    // Костры — редкая передышка
    for (int i = 0; i < r.i(1, 2); ++i) {
        WorldEnt e;
        e.kind = EntKind::Bonfire;
        e.seed = uint32_t(r.next());
        if (!freeSpot(e.pos, 200.0f)) continue;
        e.name = LS("Чей-то костёр", "Someone's Campfire");
        ents_.push_back(e);
    }
    // Тайники — заперты, открываются верным приёмом
    for (int i = 0; i < r.i(2, 3); ++i) {
        WorldEnt e;
        e.kind = EntKind::Cache;
        e.data = r.i(0, 2);
        e.seed = uint32_t(r.next());
        if (!freeSpot(e.pos, 180.0f)) continue;
        e.name = LS("Запертый тайник", "A Locked Cache");
        ents_.push_back(e);
    }
    // Алтари — жертва ради постоянной прибавки
    for (int i = 0; i < (depth_ >= 2 ? r.i(1, 2) : 1); ++i) {
        WorldEnt e;
        e.kind = EntKind::Shrine;
        e.data = r.i(0, 2);
        e.seed = uint32_t(r.next());
        if (!freeSpot(e.pos, 240.0f)) continue;
        e.name = LS("Алтарь без имени", "A Nameless Altar");
        ents_.push_back(e);
    }

    // Бродячие враги
    int enemies = 5 + depth_;
    for (int i = 0; i < enemies; ++i) {
        WorldEnt e;
        e.kind = EntKind::Enemy;
        e.seed = uint32_t(r.next());
        if (!freeSpot(e.pos, 220.0f)) continue;
        e.home = e.pos;
        e.data = r.i(0, 3);
        // Кем он будет в бою — решается здесь, чтобы фигурка на карте совпала.
        e.arche = r.i(0, int(Archetype::COUNT) - 1);
        e.flavour = randomElement(r, depth_);
        e.name = LS(std::string(archetypeName(Archetype(e.arche)).ru) + ", "
                        + elem(e.flavour).name.ru,
                    std::string(archetypeName(Archetype(e.arche)).en) + ", "
                        + elem(e.flavour).name.en);
        ents_.push_back(e);
    }
}

float Overworld::dangerAt(Vector2 p) const {
    float d = 1e9f;
    for (const auto& e : ents_) {
        if (e.kind != EntKind::Enemy || e.used) continue;
        float dx = e.pos.x - p.x, dy = e.pos.y - p.y;
        d = std::min(d, std::sqrt(dx * dx + dy * dy));
    }
    return d;
}

int Overworld::nearestInteractable(float radius) const {
    int best = -1;
    float bestD = radius * radius;
    for (int i = 0; i < int(ents_.size()); ++i) {
        const WorldEnt& e = ents_[size_t(i)];
        if (e.kind == EntKind::Enemy || e.used) continue;
        float dx = e.pos.x - player_.x, dy = e.pos.y - player_.y;
        float d2 = dx * dx + dy * dy;
        if (d2 < bestD) { bestD = d2; best = i; }
    }
    return best;
}

void Overworld::removeEnt(int index) {
    if (index < 0 || index >= int(ents_.size())) return;
    ents_.erase(ents_.begin() + index);
    if (encounter_ == index) encounter_ = -1;
    else if (encounter_ > index) encounter_--;
}

void Overworld::update(float dt, bool inputEnabled) {
    fogT_ += dt;

    Vector2 dir{ 0, 0 };
    if (inputEnabled) {
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    { dir.y -= 1; facing_ = 0; }
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  { dir.y += 1; facing_ = 2; }
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  { dir.x -= 1; facing_ = 3; }
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) { dir.x += 1; facing_ = 1; }
    }
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len > 0.0001f) {
        dir.x /= len; dir.y /= len;
        float before = walkPhase_;
        walkPhase_ += dt * 11.0f;
        // Шаг звучит на каждом полупериоде походки.
        if (int(before / PI) != int(walkPhase_ / PI))
            audio::play(audio::Sfx::Step, 0.22f, 0.85f + 0.3f * art::hash01(int(walkPhase_), 0, 7u));
    } else walkPhase_ *= 0.85f;

    const float speed = 150.0f;
    float nx = player_.x + dir.x * speed * dt;
    float ny = player_.y + dir.y * speed * dt;
    const float pad = 7.0f;
    if (!blocked(nx - pad, player_.y) && !blocked(nx + pad, player_.y)) player_.x = nx;
    if (!blocked(player_.x, ny - pad) && !blocked(player_.x, ny + pad)) player_.y = ny;

    // Враги: патруль + преследование
    for (int i = 0; i < int(ents_.size()); ++i) {
        WorldEnt& e = ents_[size_t(i)];
        e.phase += dt;
        if (e.kind != EntKind::Enemy) continue;
        float dx = player_.x - e.pos.x, dy = player_.y - e.pos.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        float sight = 150.0f + depth_ * 6.0f;
        if (dist < sight) e.aggro = std::min(1.0f, e.aggro + dt * 2.0f);
        else              e.aggro = std::max(0.0f, e.aggro - dt * 0.8f);

        float sp = 40.0f + depth_ * 3.0f + e.aggro * 46.0f;
        Vector2 want;
        if (e.aggro > 0.35f && dist > 0.001f) {
            want = Vector2{ dx / dist, dy / dist };
        } else {
            float a = e.phase * 0.6f + float(e.seed % 100);
            want = Vector2{ std::cos(a), std::sin(a) };
            float hx = e.home.x - e.pos.x, hy = e.home.y - e.pos.y;
            float hd = std::sqrt(hx * hx + hy * hy);
            if (hd > 130.0f) want = Vector2{ hx / hd, hy / hd };
        }
        float ex = e.pos.x + want.x * sp * dt;
        float ey = e.pos.y + want.y * sp * dt;
        if (!blocked(ex, e.pos.y)) e.pos.x = ex;
        if (!blocked(e.pos.x, ey)) e.pos.y = ey;

        if (inputEnabled && dist < 20.0f && encounter_ < 0) encounter_ = i;
    }

    cam_.offset = Vector2{ GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f };
    cam_.target.x += (player_.x - cam_.target.x) * std::min(1.0f, dt * 6.0f);
    cam_.target.y += (player_.y - cam_.target.y) * std::min(1.0f, dt * 6.0f);
}

// ── Drawing ───────────────────────────────────────────────────────────────
// ── Земля / Ground ────────────────────────────────────────────────────────
// Раньше каждый тайл был плоским квадратом одного цвета — отсюда шахматка.
// Теперь цвет тайла смешивается с соседями, поверх ложится низкочастотный
// свет, а детали (трава, камешки, корни, кости) раскиданы независимо от
// сетки, так что границы клеток не читаются.

static Color tileBase(Tile t) {
    switch (t) {
        case Tile::Grass:     return hex(0x4E7A3A);
        case Tile::DarkGrass: return hex(0x33522C);
        case Tile::Path:      return hex(0x8A6A42);
        case Tile::Mud:       return hex(0x5E452A);
        case Tile::Water:     return hex(0x2A5A86);
        case Tile::Rock:      return hex(0x6A6A74);
        case Tile::Tree:      return hex(0x3A5E2E);
        case Tile::Bramble:   return hex(0x4A4028);
        case Tile::Grave:     return hex(0x535660);
        case Tile::Flagstone: return hex(0x3A3A44);
        default:              return hex(0x1A1A1A);
    }
}

// Цвет земли в точке: среднее по соседним тайлам, чтобы стык был мягким.
Color Overworld::groundColor(int x, int y, float time) const {
    Color c = tileBase(at(x, y));
    // Смешиваем с четырьмя соседями — граница размывается на полклетки.
    int nx[4] = { x - 1, x + 1, x, x };
    int ny[4] = { y, y, y - 1, y + 1 };
    for (int i = 0; i < 4; ++i) c = lerpC(c, tileBase(at(nx[i], ny[i])), 0.04f);

    // Крупный свет: облачная тень медленно ползёт по земле.
    float cloud = fbm(x * 0.045f + time * 0.06f, y * 0.045f, seed_ + 31u, 3);
    float lightK = 0.94f + 0.22f * fbm(x * 0.11f, y * 0.11f, seed_ + 5u, 3);
    lightK *= (0.95f + 0.12f * cloud);
    c = scaleC(c, lightK);
    // Ступенчатый цвет: плавная заливка из шума после уменьшения читается
    // как грязь, а ограниченная палитра — как осознанно нарисованный тайл.
    auto step = [](unsigned char v) -> unsigned char {
        const int S = 10;                                   // ширина ступени
        return (unsigned char)clampi(((v + S / 2) / S) * S, 0, 255);
    };
    return Color{ step(c.r), step(c.g), step(c.b), c.a };
}

void Overworld::drawTile(int x, int y, float time) const {
    Tile t = at(x, y);
    float fx = float(x * TILE), fy = float(y * TILE);
    float v = hash01(x, y, seed_);
    Color c = groundColor(x, y, time);

    DrawRectangle(int(fx), int(fy), TILE, TILE, c);

    // Стык с соседями — не градиент, а дизеринг блоками: градиент после
    // уменьшения до пиксельного буфера читался как размытое пятно, а
    // шахматка из блоков остаётся честным пиксель-артом на любом масштабе.
    const int DB = TILE / 8;                     // размер блока дизеринга
    for (int side = 0; side < 4; ++side) {
        int nxp = x + (side == 0 ? -1 : side == 1 ? 1 : 0);
        int nyp = y + (side == 2 ? -1 : side == 3 ? 1 : 0);
        Tile nt = at(nxp, nyp);
        if (nt == t) continue;
        Color nc = groundColor(nxp, nyp, time);
        // Два ряда блоков от края: первый почти сплошной, второй редкий.
        for (int d = 0; d < 2; ++d) {
            float keep = (d == 0) ? 0.72f : 0.30f;
            for (int k = 0; k < 8; ++k) {
                if (hash01(x * 29 + k, y * 53 + d + side * 7, seed_ + 907u) > keep) continue;
                int bx = 0, by = 0;
                switch (side) {
                    case 0:  bx = 0 + d;            by = k;      break;
                    case 1:  bx = 7 - d;            by = k;      break;
                    case 2:  bx = k;                by = 0 + d;  break;
                    default: bx = k;                by = 7 - d;  break;
                }
                DrawRectangle(int(fx) + bx * DB, int(fy) + by * DB, DB, DB, nc);
            }
        }
    }

    // Мелкая крошка поверх — рассыпана по своим координатам, не по сетке
    art::grit(Vector2{ fx + TILE * 0.5f, fy + TILE * 0.5f }, TILE * 0.5f, TILE * 0.5f,
              seed_ + uint32_t(x * 131 + y * 977), scaleC(c, 1.6f), 4);

    switch (t) {
        case Tile::Water: {
            // Глубина считается по соседям, а не градиентом на весь тайл:
            // сплошная заливка затирала дизеринг берега, и озеро получалось
            // жёстким синим прямоугольником.
            int open = 0;
            if (at(x - 1, y) == Tile::Water) open++;
            if (at(x + 1, y) == Tile::Water) open++;
            if (at(x, y - 1) == Tile::Water) open++;
            if (at(x, y + 1) == Tile::Water) open++;
            const int WB = TILE / 8;
            if (open == 4) {
                // Дно открытой воды — вставка внутрь, края остаются за дизерингом.
                DrawRectangle(int(fx) + WB, int(fy) + WB, TILE - WB * 2, TILE - WB * 2,
                              hex(0x1E4568));
                if (v > 0.5f)
                    DrawRectangle(int(fx) + WB * 2, int(fy) + WB * 2, TILE - WB * 4,
                                  TILE - WB * 4, hex(0x183A5A));
            }
            // Рябь: короткие блочные штрихи, они живут на сетке пикселей
            for (int i = 0; i < 2; ++i) {
                float ph = time * 1.1f + x * 0.7f + y * 0.4f + i * 2.1f;
                int rx = int(hash01(x * 13 + i, y * 7, seed_ + 77u) * 5.0f) + 1;
                int ry = int(hash01(x * 5, y * 17 + i, seed_ + 83u) * 6.0f) + 1;
                rx = clampi(rx + int(std::sin(ph) * 1.5f), 1, 5);
                DrawRectangle(int(fx) + rx * WB, int(fy) + ry * WB, WB * 2, WB,
                              hex(0x4E86B4));
            }
            break;
        }
        case Tile::Tree: {
            // Разброс по размеру и месту сильный — иначе деревья встают рядами.
            float hs = hash01(x, y, seed_ + 41u);
            float sway = std::sin(time * 0.9f + x * 1.3f + y * 0.7f) * 2.0f;
            float r = 8.0f + hs * 9.0f;
            float dx = (hash01(x, y, seed_ + 137u) - 0.5f) * 15.0f;
            float dy = (hash01(x, y, seed_ + 71u) - 0.5f) * 11.0f;
            float lean = (hash01(x, y, seed_ + 211u) - 0.5f) * 7.0f;
            Vector2 root{ fx + 16 + dx, fy + 26 + dy * 0.4f };

            DrawEllipse(int(root.x), int(root.y), int(r * 1.05f), int(4 + r * 0.16f),
                        Color{ 0, 0, 0, 110 });
            // Ствол: конус от толстого корня к тонкой верхушке, с наклоном
            float trunkH = 11.0f + hs * 8.0f;
            for (int i = 0; i < 5; ++i) {
                float u = i / 4.0f;
                DrawLineEx(Vector2{ root.x + lean * u, root.y - u * trunkH },
                           Vector2{ root.x + lean * (u + 0.25f) + sway * u * 0.4f,
                                    root.y - (u + 0.25f) * trunkH },
                           (5.2f - u * 2.6f) * (0.7f + hs * 0.5f),
                           lerpC(hex(0x241A13), hex(0x453224), u));
            }
            // Ветви под кроной, чтобы не резали её поверху
            if (hs > 0.45f) {
                float bx = root.x + lean * 0.7f, by = root.y - trunkH * 0.7f;
                float k = (hs > 0.72f) ? 1.0f : -1.0f;
                DrawLineEx(Vector2{ bx, by }, Vector2{ bx + k * (5 + hs * 5), by - 7 }, 1.8f,
                           hex(0x352619));
            }
            // Крона: тёмный контур силуэтом, поверх — яркая листва в три ступени.
            // Контур обязателен: без него крона сливается с землёй и выглядит
            // как пятно, а не как нарисованное дерево.
            Vector2 cr{ root.x + lean + sway, root.y - trunkH - r * 0.35f + dy * 0.3f };
            const Color outline = hex(0x14240F);
            DrawEllipse(int(cr.x - r * 0.52f), int(cr.y + r * 0.26f), r * 0.80f, r * 0.68f, outline);
            DrawEllipse(int(cr.x + r * 0.50f), int(cr.y + r * 0.22f), r * 0.78f, r * 0.66f, outline);
            DrawEllipse(int(cr.x + r * 0.10f), int(cr.y - r * 0.30f), r * 0.68f, r * 0.58f, outline);
            DrawEllipse(int(cr.x), int(cr.y), r * 1.06f, r * 0.90f, outline);
            art::blob(Vector2{ cr.x - r * 0.52f, cr.y + r * 0.26f }, r * 0.74f, r * 0.62f,
                      hex(0x24401C), hex(0x3F6B2E), 3);
            art::blob(Vector2{ cr.x + r * 0.50f, cr.y + r * 0.22f }, r * 0.72f, r * 0.60f,
                      hex(0x26441E), hex(0x467434), 3);
            art::blob(Vector2{ cr.x + r * 0.10f, cr.y - r * 0.30f }, r * 0.62f, r * 0.52f,
                      hex(0x2C4C22), hex(0x588C3E), 3);
            art::blob(cr, r, r * 0.84f, hex(0x274420), hex(0x4C7C36), 4);
            // Просветы в листве
            art::grit(cr, r * 0.7f, r * 0.55f, seed_ + uint32_t(x * 31 + y), hex(0x6E9C4A), 5);
            break;
        }
        case Tile::Rock: {
            int sides = 5 + int(v * 3.0f);
            float rr = 9.0f + v * 6.0f;
            float rx = fx + 16 + (hash01(x, y, seed_ + 61u) - 0.5f) * 7.0f;
            float ry = fy + 17 + (hash01(x, y, seed_ + 91u) - 0.5f) * 6.0f;
            DrawEllipse(int(rx), int(ry + rr * 0.55f), int(rr), 4, Color{ 0, 0, 0, 110 });
            // Камень: тёмный контур, затем слои от тени к свету. Светлее, чем
            // было: на тёплой земле прежний почти чёрный камень читался дырой.
            DrawPoly(Vector2{ rx, ry }, sides, rr * 1.10f, v * 90.0f, hex(0x22222B));
            for (int L = 4; L >= 0; --L) {
                float k = L / 4.0f;
                DrawPoly(Vector2{ rx, ry + rr * 0.22f * k }, sides, rr * (0.55f + 0.42f * k),
                         v * 90.0f, lerpC(hex(0x9A9AA6), hex(0x4A4A56), k));
            }
            art::grit(Vector2{ rx, ry }, rr * 0.7f, rr * 0.6f, seed_ + uint32_t(x + y), hex(0x2E2E38), 6);
            break;
        }
        case Tile::Bramble: {
            for (int i = 0; i < 6; ++i) {
                float a = v * 6.28f + i * 1.05f;
                Vector2 b0{ fx + 16, fy + 24 };
                Vector2 b1{ fx + 16 + std::cos(a) * 12, fy + 24 - std::abs(std::sin(a)) * 17 };
                DrawLineEx(b0, b1, 1.6f, hex(0x33301F));
                // Шипы
                DrawLineEx(b1, Vector2{ b1.x + 3, b1.y - 3 }, 1.0f, hex(0x4A452C));
            }
            DrawEllipse(int(fx + 16), int(fy + 25), 9, 3, Color{ 0, 0, 0, 90 });
            break;
        }
        case Tile::Grave: {
            float gx = fx + 16, gy = fy + 20;
            DrawEllipse(int(gx), int(gy + 6), 11, 4, Color{ 0, 0, 0, 120 });
            // Покосившаяся плита
            float tilt = (v - 0.5f) * 5.0f;
            for (int L = 3; L >= 0; --L) {
                float k = L / 3.0f;
                DrawRectangleRounded(Rectangle{ gx - 6 + tilt * k, gy - 15 + k * 3, 12, 20 },
                                     0.45f, 4, lerpC(hex(0x5C5C68), hex(0x24242C), k));
            }
            DrawLineEx(Vector2{ gx + tilt, gy - 11 }, Vector2{ gx + tilt, gy - 2 }, 1.4f, hex(0x191920));
            DrawLineEx(Vector2{ gx - 3 + tilt, gy - 8 }, Vector2{ gx + 3 + tilt, gy - 8 }, 1.4f,
                       hex(0x191920));
            // Пучок травы у подножия
            for (int i = 0; i < 3; ++i)
                DrawLineEx(Vector2{ gx - 6 + i * 6, gy + 5 }, Vector2{ gx - 7 + i * 6, gy - 1 },
                           1.2f, hex(0x22331F));
            break;
        }
        case Tile::Grass:
        case Tile::DarkGrass: {
            // Пучки травы: положение независимо от центра клетки
            int tufts = (t == Tile::Grass) ? 3 : 2;
            for (int i = 0; i < tufts; ++i) {
                float hx = hash01(x * 7 + i, y * 3, seed_ + 17u);
                float hy = hash01(x * 5, y * 11 + i, seed_ + 29u);
                if (hx + hy < 0.5f) continue;
                float gx = fx + hx * TILE, gy = fy + hy * TILE;
                float sw = std::sin(time * 1.1f + gx * 0.05f) * 1.4f;
                Color gc = scaleC(c, 1.45f);
                for (int b = -1; b <= 1; ++b)
                    DrawLineEx(Vector2{ gx + b * 2.0f, gy },
                               Vector2{ gx + b * 2.6f + sw, gy - 4.5f - std::abs(b) * -1.5f },
                               1.1f, gc);
            }
            // Редкие грибы и косточки на порче
            if (t == Tile::DarkGrass && hash01(x, y, seed_ + 313u) > 0.985f) {
                float mx = fx + 8 + v * 14, my = fy + 12 + v * 10;
                DrawLineEx(Vector2{ mx, my }, Vector2{ mx, my - 3 }, 1.2f, withA(hex(0x8A7D68), 170));
                DrawEllipse(int(mx), int(my - 4), 2, 1, withA(hex(0x5A2028), 190));
            }
            break;
        }
        case Tile::Path: {
            // Утоптанная земля: камешки и колея
            if (v > 0.45f) {
                float px = fx + hash01(x, y, seed_ + 3u) * TILE;
                float py = fy + hash01(x, y, seed_ + 9u) * TILE;
                art::blob(Vector2{ px, py }, 2.4f + v * 1.6f, 1.8f + v, hex(0x2C241A),
                          hex(0x6A5A44), 3);
            }
            DrawLineEx(Vector2{ fx, fy + 10 + v * 12 }, Vector2{ fx + TILE, fy + 8 + v * 14 },
                       1.0f, withA(hex(0x2A2018), 70));
            break;
        }
        case Tile::Mud: {
            // Лужи блоками и в тон земле. Прежние почти чёрно-синие эллипсы
            // на тёплой грязи читались как россыпь посторонних «таблеток».
            if (v > 0.78f) {
                const int MB = TILE / 8;
                int px = int(hash01(x, y, seed_ + 3u) * 4.0f) + 1;
                int py = int(hash01(x, y, seed_ + 9u) * 5.0f) + 1;
                DrawRectangle(int(fx) + px * MB, int(fy) + py * MB, MB * 3, MB * 2,
                              hex(0x44341F));
                DrawRectangle(int(fx) + px * MB, int(fy) + py * MB, MB * 2, MB,
                              hex(0x5A4A32));
            }
            break;
        }
        default: break;
    }
}
static void drawEntity(const WorldEnt& e, float time) {
    switch (e.kind) {
        case EntKind::Resource: {
            Color c = material(e.data).tint;
            DrawEllipse(int(e.pos.x), int(e.pos.y + 7), 8, 3, Color{ 0, 0, 0, 110 });
            float pulse = 0.5f + 0.5f * std::sin(time * 2.0f + e.pos.x * 0.05f);
            DrawCircleGradient(int(e.pos.x), int(e.pos.y), 14.0f, withA(c, (unsigned char)(40 + 30 * pulse)), BLANK);
            DrawPolyLinesEx(Vector2{ e.pos.x, e.pos.y }, 5, 7.0f, time * 20.0f, 1.6f, c);
            DrawCircleV(Vector2{ e.pos.x, e.pos.y }, 3.0f, withA(c, 220));
            break;
        }
        case EntKind::Npc: {
            DrawEllipse(int(e.pos.x), int(e.pos.y + 10), 9, 4, Color{ 0, 0, 0, 120 });
            Color robe = e.data == 0 ? hex(0x3A2A38) : (e.data == 1 ? hex(0x233245) : hex(0x40342A));
            DrawTriangle(Vector2{ e.pos.x - 9, e.pos.y + 10 },
                         Vector2{ e.pos.x, e.pos.y - 8 },
                         Vector2{ e.pos.x + 9, e.pos.y + 10 }, robe);
            DrawCircleV(Vector2{ e.pos.x, e.pos.y - 10 }, 5.0f, hex(0xC9AE96));
            float g = 0.5f + 0.5f * std::sin(time * 1.7f);
            DrawCircleGradient(int(e.pos.x), int(e.pos.y - 2), 22.0f,
                               withA(art::pal::GoldLit, (unsigned char)(30 + 25 * g)), BLANK);
            break;
        }
        case EntKind::RuinGate: {
            DrawEllipse(int(e.pos.x), int(e.pos.y + 14), 22, 6, Color{ 0, 0, 0, 140 });
            DrawRectangle(int(e.pos.x - 20), int(e.pos.y - 26), 7, 40, hex(0x2E2E38));
            DrawRectangle(int(e.pos.x + 13), int(e.pos.y - 26), 7, 40, hex(0x2E2E38));
            DrawRectangle(int(e.pos.x - 22), int(e.pos.y - 32), 44, 8, hex(0x3A3A46));
            float g = 0.5f + 0.5f * std::sin(time * 1.2f);
            DrawRectangleGradientV(int(e.pos.x - 12), int(e.pos.y - 24), 24, 38,
                                   withA(art::pal::Blood, (unsigned char)(70 + 60 * g)), BLANK);
            art::drawRuneRing(Vector2{ e.pos.x, e.pos.y - 6 }, 26.0f, 8,
                              withA(art::pal::Blood, 130), time);
            break;
        }
        case EntKind::EventSite: {
            DrawEllipse(int(e.pos.x), int(e.pos.y + 8), 14, 5, Color{ 0, 0, 0, 120 });
            art::drawRuneRing(Vector2{ e.pos.x, e.pos.y }, 16.0f, 6, withA(hex(0x9D4EDD), 160), time);
            DrawPolyLinesEx(Vector2{ e.pos.x, e.pos.y }, 3, 9.0f, -time * 30.0f, 1.8f, hex(0xE0AAFF));
            break;
        }
        case EntKind::Carriage: {
            DrawEllipse(int(e.pos.x), int(e.pos.y + 14), 26, 6, Color{ 0, 0, 0, 140 });
            DrawRectangleRounded(Rectangle{ e.pos.x - 22, e.pos.y - 18, 44, 26 }, 0.35f, 6, hex(0x1A1620));
            DrawRectangleRoundedLinesEx(Rectangle{ e.pos.x - 22, e.pos.y - 18, 44, 26 }, 0.35f, 6, 1.5f,
                                        art::pal::Gold);
            DrawCircleLines(int(e.pos.x - 13), int(e.pos.y + 10), 7.0f, art::pal::Gold);
            DrawCircleLines(int(e.pos.x + 13), int(e.pos.y + 10), 7.0f, art::pal::Gold);
            DrawRectangle(int(e.pos.x - 8), int(e.pos.y - 12), 16, 12, withA(hex(0xFFD60A), 60));
            break;
        }
        case EntKind::Bonfire: {
            DrawEllipse(int(e.pos.x), int(e.pos.y + 8), 18, 6, Color{ 0, 0, 0, 120 });
            for (int i = 0; i < 5; ++i) {   // поленья шалашом
                float a = i * 1.26f;
                DrawLineEx(Vector2{ e.pos.x + std::cos(a) * 12, e.pos.y + 6 },
                           Vector2{ e.pos.x, e.pos.y - 10 }, 3.0f, hex(0x3A2A1C));
            }
            float fl = 0.7f + 0.3f * std::sin(time * 8.0f + e.pos.x);
            DrawCircleGradient(int(e.pos.x), int(e.pos.y - 10), 40 * fl,
                               withA(hex(0xFFBA08), 110), BLANK);
            DrawTriangle(Vector2{ e.pos.x, e.pos.y - 12 - 20 * fl },
                         Vector2{ e.pos.x - 8, e.pos.y + 2 },
                         Vector2{ e.pos.x + 8, e.pos.y + 2 }, hex(0xE85D04));
            DrawTriangle(Vector2{ e.pos.x, e.pos.y - 12 - 11 * fl },
                         Vector2{ e.pos.x - 4, e.pos.y + 2 },
                         Vector2{ e.pos.x + 4, e.pos.y + 2 }, hex(0xFFD60A));
            break;
        }
        case EntKind::Cache: {
            DrawEllipse(int(e.pos.x), int(e.pos.y + 11), 18, 5, Color{ 0, 0, 0, 120 });
            DrawRectangleRounded(Rectangle{ e.pos.x - 16, e.pos.y - 10, 32, 22 }, 0.2f, 5,
                                 hex(0x3A2A1C));
            DrawRectangleRoundedLinesEx(Rectangle{ e.pos.x - 16, e.pos.y - 10, 32, 22 }, 0.2f, 5,
                                        1.6f, hex(0x6C757D));
            DrawRectangle(int(e.pos.x - 16), int(e.pos.y - 2), 32, 4, hex(0x6C757D));
            // Замок с бьющимся светом внутри
            float g = 0.5f + 0.5f * std::sin(time * 2.2f + e.pos.y * 0.02f);
            DrawCircleV(Vector2{ e.pos.x, e.pos.y }, 5.0f, hex(0x1A1216));
            DrawCircleV(Vector2{ e.pos.x, e.pos.y }, 2.6f,
                        withA(art::pal::GoldLit, (unsigned char)(120 + 100 * g)));
            break;
        }
        case EntKind::Shrine: {
            DrawEllipse(int(e.pos.x), int(e.pos.y + 14), 22, 6, Color{ 0, 0, 0, 130 });
            // Плита и стела
            DrawRectangleRounded(Rectangle{ e.pos.x - 20, e.pos.y + 4, 40, 10 }, 0.3f, 5,
                                 hex(0x3A3A46));
            DrawTriangle(Vector2{ e.pos.x - 13, e.pos.y + 6 },
                         Vector2{ e.pos.x, e.pos.y - 30 },
                         Vector2{ e.pos.x + 13, e.pos.y + 6 }, hex(0x2E2E38));
            art::drawSigil(ElementId(1 + int(e.seed % 16)),
                           Vector2{ e.pos.x, e.pos.y - 8 }, 9.0f,
                           withA(art::pal::GoldLit, 220), time);
            float g = 0.5f + 0.5f * std::sin(time * 1.4f);
            DrawCircleGradient(int(e.pos.x), int(e.pos.y - 8), 34.0f,
                               withA(art::pal::Gold, (unsigned char)(40 + 35 * g)), BLANK);
            break;
        }
        case EntKind::Enemy:
            // На карте стоит ровно тот облик, который встретит в бою.
            art::drawEnemyToken(e.pos, 1.15f, Archetype(e.arche), e.flavour,
                                time, e.aggro, e.seed);
            break;
    }
}

void Overworld::draw(float time, int viewW, int viewH, float pixelScale) const {
    ClearBackground(art::pal::Night);

    // Камера под низкое разрешение: поле зрения то же, пикселей втрое меньше.
    Camera2D cam = cam_;
    cam.offset = Vector2{ viewW * 0.5f, viewH * 0.5f };
    cam.zoom = cam_.zoom * pixelScale;
    BeginMode2D(cam);

    // Видимая область в тайлах
    float halfW = viewW * 0.5f / cam.zoom;
    float halfH = viewH * 0.5f / cam.zoom;
    int x0 = clampi(int((cam_.target.x - halfW) / TILE) - 1, 0, MAP_W - 1);
    int x1 = clampi(int((cam_.target.x + halfW) / TILE) + 1, 0, MAP_W - 1);
    int y0 = clampi(int((cam_.target.y - halfH) / TILE) - 1, 0, MAP_H - 1);
    int y1 = clampi(int((cam_.target.y + halfH) / TILE) + 1, 0, MAP_H - 1);

    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x)
            drawTile(x, y, time);

    // Сущности — сортировка по Y для псевдо-глубины
    std::vector<const WorldEnt*> sorted;
    sorted.reserve(ents_.size());
    for (const auto& e : ents_) sorted.push_back(&e);
    std::sort(sorted.begin(), sorted.end(),
              [](const WorldEnt* a, const WorldEnt* b) { return a->pos.y < b->pos.y; });

    bool drawnPlayer = false;
    for (const WorldEnt* e : sorted) {
        if (!drawnPlayer && e->pos.y > player_.y) {
            art::drawAristocratTopDown(player_, 1.15f, facing_, walkPhase_);
            drawnPlayer = true;
        }
        drawEntity(*e, time);
    }
    if (!drawnPlayer) art::drawAristocratTopDown(player_, 1.15f, facing_, walkPhase_);

    EndMode2D();

    // Туман и лунный свет — поверх мира, в координатах буфера
    const int sw = viewW, sh = viewH;
    for (int i = 0; i < 5; ++i) {
        float ox = std::fmod(time * (6.0f + i * 3.0f) * pixelScale, float(sw + 140)) - 70.0f;
        float oy = (90.0f + i * 96.0f + std::sin(time * 0.4f + i) * 18.0f) * pixelScale;
        DrawCircleGradient(int(ox), int(oy), 190.0f * pixelScale, withA(art::pal::Fog, 12), BLANK);
    }
    DrawCircleGradient(sw / 2, sh / 2, float(sh) * 1.15f, BLANK, Color{ 0, 0, 0, 55 });
}

void Overworld::drawHud(float) const { /* HUD рисует Game — здесь только мир */ }

} // namespace shichoy

namespace shichoy {

// Тайл под ногами решает, где именно принят бой.
Biome Overworld::biomeAt(Vector2 p) const {
    int tx = int(std::floor(p.x / TILE)), ty = int(std::floor(p.y / TILE));
    // Могилы и камни редки — смотрим на окрестность, а не только под ноги.
    int votes[int(Biome::COUNT)] = { 0 };
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            Biome b;
            switch (at(tx + dx, ty + dy)) {
                case Tile::Water:
                case Tile::Mud:       b = Biome::Mire; break;
                case Tile::Rock:      b = Biome::Stone; break;
                case Tile::Tree:      b = Biome::Thicket; break;
                case Tile::Bramble:   b = Biome::Bramble; break;
                case Tile::Grave:     b = Biome::Graves; break;
                case Tile::DarkGrass: b = Biome::Rot; break;
                default:              b = Biome::Meadow; break;
            }
            // Приметное место весит больше пустой травы.
            votes[int(b)] += (b == Biome::Meadow) ? 1 : 3;
            if (dx == 0 && dy == 0) votes[int(b)] += 2;
        }
    }
    int best = 0;
    for (int i = 1; i < int(Biome::COUNT); ++i)
        if (votes[i] > votes[best]) best = i;
    return Biome(best);
}

} // namespace shichoy
