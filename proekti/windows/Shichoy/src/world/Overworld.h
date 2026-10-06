// Overworld / Оверворлд: топ-даун исследование, ресурсы, NPC, руины, события.
#pragma once
#include "core/Biome.h"
#include "combat/Enemy.h"
#include "core/Cards.h"

namespace shichoy {

constexpr int TILE = 32;
constexpr int MAP_W = 96;
constexpr int MAP_H = 96;

enum class Tile : unsigned char {
    Grass = 0, DarkGrass, Path, Mud, Water, Rock, Tree, Bramble, Grave, Flagstone
};
bool tileSolid(Tile t);

enum class EntKind : int {
    Resource = 0,   // куст/жила/пень — сбор материалов
    Npc,            // бродячий торговец / плакальщица
    RuinGate,       // вход в руины: босс
    EventSite,      // событие: мини-игра, мутация, находка
    Carriage,       // выход из забега
    Bonfire,        // костёр: отдых и перековка
    Cache,          // тайник: заперт, нужен верный приём
    Shrine,         // алтарь: жертва ради постоянной прибавки
    Enemy           // бродячий враг
};

struct WorldEnt {
    EntKind kind = EntKind::Resource;
    Vector2 pos{ 0, 0 };
    int   data = 0;          // материал / тип NPC / вариант события
    bool  used = false;
    bool  boss = false;
    float phase = 0.0f;      // анимация
    float aggro = 0.0f;      // для врагов
    Vector2 home{ 0, 0 };
    uint32_t seed = 0;
    // Для бродячих врагов: кем он окажется в бою. На карте рисуется тот же.
    int   arche = 0;
    ElementId flavour = ElementId::None;
    LS name;
};

struct WorldTouch {         // что произошло за кадр
    bool encounter = false; // столкновение с врагом
    int  entIndex = -1;     // индекс сущности для взаимодействия
    bool interactReady = false;
};

class Overworld {
public:
    void generate(int depth, Rng& r);
    // Регион — чистая функция от (глубина, сид): нужно для сохранения забега.
    void generate(int depth, uint32_t seed);
    uint32_t seed() const { return seed_; }
    std::vector<WorldEnt>& entsMut() { return ents_; }
    const std::vector<WorldEnt>& ents() const { return ents_; }
    void update(float dt, bool inputEnabled);
    // Мир рисуется в низкое разрешение и растягивается без сглаживания —
    // отсюда «пиксельный» вид вместо гладкой векторной геометрии.
    void draw(float time, int viewW, int viewH, float pixelScale) const;
    void drawHud(float time) const;

    Vector2 playerPos() const { return player_; }
    void setPlayerPos(Vector2 p) { player_ = p; }
    // Камера без догоняющей интерполяции: нужно после телепорта игрока.
    void snapCamera() { cam_.target = player_; }
    Camera2D camera() const { return cam_; }
    int depth() const { return depth_; }

    // Сущность рядом с игроком (для нажатия E) и результат столкновений.
    int nearestInteractable(float radius = 40.0f) const;
    int pendingEncounter() const { return encounter_; }
    void clearEncounter() { encounter_ = -1; }
    void removeEnt(int index);
    WorldEnt& ent(int i) { return ents_[size_t(i)]; }
    const WorldEnt& ent(int i) const { return ents_[size_t(i)]; }
    int entCount() const { return int(ents_.size()); }
    float dangerAt(Vector2 p) const;
    Biome biomeAt(Vector2 p) const;      // биом под точкой — влияет на бой

private:
    Tile at(int x, int y) const;
    bool blocked(float x, float y) const;
    void carvePath(Rng& r, int x0, int y0, int x1, int y1);
    void placeEntities(Rng& r);
    void drawTile(int x, int y, float time) const;
    Color groundColor(int x, int y, float time) const;

    std::vector<Tile> map_;
    std::vector<WorldEnt> ents_;
    Vector2 player_{ 0, 0 };
    Vector2 vel_{ 0, 0 };
    int   facing_ = 2;
    float walkPhase_ = 0.0f;
    Camera2D cam_{};
    int   depth_ = 1;
    uint32_t seed_ = 1;
    int   encounter_ = -1;
    mutable float fogT_ = 0.0f;
};

} // namespace shichoy
