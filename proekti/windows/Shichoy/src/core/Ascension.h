// Ascension / Нисхождение: лестница сложности на 20 ступеней.
// Самая дешёвая механика удержания в жанре — бесконечная реиграбельность
// почти без нового контента. Каждая ступень добавляет ровно одно правило,
// и все правила ниже продолжают действовать.
#pragma once
#include "core/Types.h"

namespace shichoy {

constexpr int ASCENSION_MAX = 20;

// Накопленный эффект всех ступеней до указанной включительно.
struct Ascension {
    float enemyDamage = 1.0f;   // множитель урона врага
    float enemyHp     = 1.0f;   // множитель здоровья обычных врагов
    float bossHp      = 1.0f;   // отдельный множитель для боссов
    float healing     = 1.0f;   // множитель любого лечения игрока
    float rewardMul   = 1.0f;   // множитель материалов с боёв
    float craftCap    = 0.70f;  // потолок шанса в крафте
    int   startHp     = 0;      // прибавка (отрицательная) к стартовому HP
    int   cardLimit   = 0;      // прибавка к лимиту карт за ход
    int   drawPerTurn = 0;      // прибавка к добору
    int   markTurns   = 0;      // прибавка к стойкости метки
    int   extraAction = 0;      // на сколько глубин раньше враг получает +действие
    bool  eliteDense  = false;  // больше бродячих врагов на карте
    bool  noFlee      = false;  // сбежать из боя нельзя
    bool  bossDouble  = false;  // босс начинает с двумя действиями
};

Ascension ascensionAt(int level);

// Текст правила, которое добавляет именно эта ступень (1..ASCENSION_MAX).
LS ascensionRule(int level);

} // namespace shichoy
