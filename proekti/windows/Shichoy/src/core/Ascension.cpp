#include "core/Ascension.h"

namespace shichoy {

// Правила нарочно разного рода: часть давит числами, часть отнимает
// инструменты. Лестница из одних множителей надоедает к пятой ступени.
Ascension ascensionAt(int level) {
    Ascension a;
    if (level < 1) return a;
    const int L = (level > ASCENSION_MAX) ? ASCENSION_MAX : level;

    for (int i = 1; i <= L; ++i) {
        switch (i) {
            case 1:  a.enemyDamage *= 1.10f; break;
            case 2:  a.rewardMul   *= 0.85f; break;
            case 3:  a.eliteDense   = true;  break;
            case 4:  a.startHp     -= 5;     break;
            case 5:  a.healing     *= 0.75f; break;
            case 6:  a.enemyHp     *= 1.15f; break;
            case 7:  a.extraAction += 2;     break;
            case 8:  a.craftCap     = 0.60f; break;
            case 9:  a.enemyDamage *= 1.10f; break;
            case 10: a.markTurns   -= 1;     break;
            case 11: a.bossHp      *= 1.25f; break;
            case 12: a.drawPerTurn -= 1;     break;
            case 13: a.rewardMul   *= 0.85f; break;
            case 14: a.enemyHp     *= 1.15f; break;
            case 15: a.noFlee       = true;  break;
            case 16: a.startHp     -= 10;    break;
            case 17: a.enemyDamage *= 1.15f; break;
            case 18: a.cardLimit   -= 1;     break;
            case 19: a.craftCap     = 0.50f; break;
            case 20: a.bossDouble   = true;  break;
            default: break;
        }
    }
    return a;
}

LS ascensionRule(int level) {
    switch (level) {
        case 1:  return LS("Враги бьют на 10% сильнее.", "Enemies hit 10% harder.");
        case 2:  return LS("С боёв падает на 15% меньше материалов.",
                           "Fights yield 15% fewer materials.");
        case 3:  return LS("Бродячих врагов на карте заметно больше.",
                           "Far more wanderers roam the map.");
        case 4:  return LS("Стартовое здоровье меньше на 5.",
                           "Starting health is 5 lower.");
        case 5:  return LS("Любое лечение слабее на четверть.",
                           "All healing is a quarter weaker.");
        case 6:  return LS("У обычных врагов на 15% больше здоровья.",
                           "Ordinary enemies have 15% more health.");
        case 7:  return LS("Враги получают второе действие на две глубины раньше.",
                           "Enemies gain a second action two depths earlier.");
        case 8:  return LS("Потолок шанса в крафте падает до 60%.",
                           "The craft chance cap drops to 60%.");
        case 9:  return LS("Враги бьют ещё на 10% сильнее.",
                           "Enemies hit another 10% harder.");
        case 10: return LS("Метка стихии держится на ход меньше.",
                           "Elemental marks last one turn less.");
        case 11: return LS("У боссов на 25% больше здоровья.",
                           "Bosses have 25% more health.");
        case 12: return LS("Добор в начале хода меньше на одну карту.",
                           "You draw one card fewer each turn.");
        case 13: return LS("Материалов падает ещё на 15% меньше.",
                           "Materials drop another 15% lower.");
        case 14: return LS("У обычных врагов ещё на 15% больше здоровья.",
                           "Ordinary enemies gain another 15% health.");
        case 15: return LS("Сбежать из боя больше нельзя.",
                           "You can no longer flee a fight.");
        case 16: return LS("Стартовое здоровье меньше ещё на 10.",
                           "Starting health is 10 lower still.");
        case 17: return LS("Враги бьют ещё на 15% сильнее.",
                           "Enemies hit another 15% harder.");
        case 18: return LS("Слотов хода на один меньше.",
                           "You have one fewer turn slot.");
        case 19: return LS("Потолок шанса в крафте падает до 50%.",
                           "The craft chance cap drops to 50%.");
        case 20: return LS("Боссы начинают бой с двумя действиями.",
                           "Bosses open the fight with two actions.");
        default: return LS("", "");
    }
}

} // namespace shichoy
