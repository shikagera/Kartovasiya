#include "world/Overworld.h"
#include "art/ProcArt.h"
#include <algorithm>
#include <cmath>

namespace kv {

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

void Overworld::generate(int depth, Rng& r) {
    depth_ = std::max(1, depth);
    seed_ = uint32_t(r.next());
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
    cam_.zoom = 1.6f;

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
    // Бродячие враги
    int enemies = 5 + depth_;
    for (int i = 0; i < enemies; ++i) {
        WorldEnt e;
        e.kind = EntKind::Enemy;
        e.seed = uint32_t(r.next());
        if (!freeSpot(e.pos, 220.0f)) continue;
        e.home = e.pos;
        e.data = r.i(0, 3);
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
    if (len > 0.0001f) { dir.x /= len; dir.y /= len; walkPhase_ += dt * 11.0f; }
    else walkPhase_ *= 0.85f;

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
static Color tileBase(Tile t) {
    switch (t) {
        case Tile::Grass:     return hex(0x24352A);
        case Tile::DarkGrass: return hex(0x18241D);
        case Tile::Path:      return hex(0x3A3227);
        case Tile::Mud:       return hex(0x2A2318);
        case Tile::Water:     return hex(0x101C2A);
        case Tile::Rock:      return hex(0x2B2B33);
        case Tile::Tree:      return hex(0x121C15);
        case Tile::Bramble:   return hex(0x1B1A14);
        case Tile::Grave:     return hex(0x22252A);
        case Tile::Flagstone: return hex(0x33333C);
        default:              return hex(0x1A1A1A);
    }
}

void Overworld::drawTile(int x, int y, float time) const {
    Tile t = at(x, y);
    float fx = float(x * TILE), fy = float(y * TILE);
    float v = hash01(x, y, seed_);
    // Освещение берём из низкочастотного шума — тайлы сливаются в пятна,
    // а не в шахматку.
    float shade = 0.84f + 0.34f * fbm(x * 0.22f, y * 0.22f, seed_ + 5u, 3);
    Color c = scaleC(tileBase(t), shade + v * 0.05f);
    DrawRectangle(int(fx), int(fy), TILE, TILE, c);
    // мягкая растушёвка стыка с соседями
    DrawRectangleGradientV(int(fx), int(fy), TILE, TILE / 2,
                           Color{ 0, 0, 0, (unsigned char)(26 * (1.0f - shade + 0.2f)) }, BLANK);
    // мелкая крошка — разбивает сетку тайлов
    for (int i = 0; i < 3; ++i) {
        float hx = hash01(x * 7 + i, y * 13 + i, seed_ + 3u);
        float hy = hash01(x * 11 - i, y * 5 + i, seed_ + 17u);
        if (hx + hy < 0.55f) continue;
        DrawRectangle(int(fx + hx * (TILE - 3)), int(fy + hy * (TILE - 3)), 2, 2,
                      scaleC(c, hx > 0.75f ? 1.35f : 0.7f));
    }

    switch (t) {
        case Tile::Water: {
            float w = std::sin(time * 1.6f + x * 0.7f + y * 0.4f);
            DrawRectangle(int(fx), int(fy + 8 + w * 3), TILE, 2, withA(hex(0x35608A), 90));
            DrawRectangle(int(fx), int(fy + 20 - w * 3), TILE, 1, withA(hex(0x35608A), 60));
            break;
        }
        case Tile::Tree: {
            float sway = std::sin(time * 0.9f + x * 1.3f + y * 0.7f) * 1.5f;
            float r = 9.0f + v * 4.0f;                       // размер кроны от тайла
            float dx = (hash01(x, y, seed_ + 41u) - 0.5f) * 5.0f;
            float dy = (hash01(x, y, seed_ + 71u) - 0.5f) * 4.0f;
            DrawEllipse(int(fx + 16 + dx), int(fy + 25), int(r * 0.8f), 4, Color{ 0, 0, 0, 120 });
            DrawRectangle(int(fx + 14 + dx), int(fy + 12), 4, 13, hex(0x241A14));
            DrawCircleV(Vector2{ fx + 16 + dx + sway, fy + 11 + dy }, r, hex(0x16241A));
            DrawCircleV(Vector2{ fx + 16 + dx - r * 0.5f + sway, fy + 14 + dy }, r * 0.66f, hex(0x0E1A12));
            DrawCircleV(Vector2{ fx + 16 + dx + r * 0.5f + sway, fy + 14 + dy }, r * 0.66f, hex(0x1C2E20));
            if (v > 0.72f)   // редкие сухие ветви
                DrawLineEx(Vector2{ fx + 16 + dx, fy + 16 },
                           Vector2{ fx + 16 + dx + (v > 0.86f ? 8 : -8), fy + 6 }, 1.4f, hex(0x2A2018));
            break;
        }
        case Tile::Rock: {
            int sides = 5 + int(v * 3.0f);
            float rr = 9.0f + v * 6.0f;
            float rx = fx + 16 + (hash01(x, y, seed_ + 61u) - 0.5f) * 6.0f;
            float ry = fy + 16 + (hash01(x, y, seed_ + 91u) - 0.5f) * 6.0f;
            DrawPoly(Vector2{ rx, ry }, sides, rr, v * 90.0f, hex(0x24242C));
            DrawPolyLinesEx(Vector2{ rx, ry }, sides, rr, v * 90.0f, 1.6f, hex(0x3E3E4A));
            break;
        }
        case Tile::Bramble:
            for (int i = 0; i < 4; ++i) {
                float a = v * 6.28f + i * 1.5f;
                DrawLineEx(Vector2{ fx + 16, fy + 22 },
                           Vector2{ fx + 16 + std::cos(a) * 11, fy + 22 - std::abs(std::sin(a)) * 15 },
                           1.4f, hex(0x2E2A1C));
            }
            break;
        case Tile::Grave:
            DrawRectangleRounded(Rectangle{ fx + 11, fy + 8, 10, 16 }, 0.5f, 4, hex(0x4A4A54));
            DrawLineEx(Vector2{ fx + 16, fy + 12 }, Vector2{ fx + 16, fy + 20 }, 1.5f, hex(0x2A2A32));
            DrawLineEx(Vector2{ fx + 13, fy + 15 }, Vector2{ fx + 19, fy + 15 }, 1.5f, hex(0x2A2A32));
            break;
        case Tile::DarkGrass:
            if (v > 0.7f) DrawLineEx(Vector2{ fx + 8, fy + 24 }, Vector2{ fx + 10, fy + 17 }, 1.2f, hex(0x22301F));
            break;
        case Tile::Path:
            if (v > 0.5f) DrawCircleV(Vector2{ fx + 6 + v * 20, fy + 6 + v * 18 }, 1.4f, hex(0x4A4034));
            break;
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
        case EntKind::Enemy: {
            float bob = std::sin(time * 3.0f + e.pos.x * 0.03f) * 2.0f;
            DrawEllipse(int(e.pos.x), int(e.pos.y + 11), 10, 4, Color{ 0, 0, 0, 130 });
            Color body = e.aggro > 0.35f ? hex(0x3A1420) : hex(0x191424);
            DrawTriangle(Vector2{ e.pos.x - 10, e.pos.y + 11 },
                         Vector2{ e.pos.x, e.pos.y - 12 + bob },
                         Vector2{ e.pos.x + 10, e.pos.y + 11 }, body);
            DrawCircleV(Vector2{ e.pos.x - 3, e.pos.y - 4 + bob }, 1.8f, hex(0xFF4D4D));
            DrawCircleV(Vector2{ e.pos.x + 3, e.pos.y - 4 + bob }, 1.8f, hex(0xFF4D4D));
            if (e.aggro > 0.35f)
                art::textC("!", e.pos.x, e.pos.y - 30 + bob, 16.0f, hex(0xFF5A40));
            break;
        }
    }
}

void Overworld::draw(float time) const {
    ClearBackground(art::pal::Night);
    BeginMode2D(cam_);

    // Видимая область в тайлах
    float halfW = GetScreenWidth() * 0.5f / cam_.zoom;
    float halfH = GetScreenHeight() * 0.5f / cam_.zoom;
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

    // Туман и лунный свет — поверх мира, в экранных координатах
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    for (int i = 0; i < 5; ++i) {
        float ox = std::fmod(time * (6.0f + i * 3.0f), float(sw + 400)) - 200.0f;
        float oy = 90.0f + i * 96.0f + std::sin(time * 0.4f + i) * 18.0f;
        DrawCircleGradient(int(ox), int(oy), 190.0f, withA(art::pal::Fog, 12), BLANK);
    }
    DrawCircleGradient(sw / 2, sh / 2, float(sh) * 1.15f, BLANK, Color{ 0, 0, 0, 120 });
    art::drawVignette(sw, sh, 0.9f);
}

void Overworld::drawHud(float) const { /* HUD рисует Game — здесь только мир */ }

} // namespace kv
