// Combat / Бой: пошаговый, вид от первого лица, без маны — только лимит карт.
#pragma once
#include "combat/Enemy.h"
#include "core/Biome.h"
#include "core/Relics.h"

namespace shichoy {

struct Player {
    int hp = 60, maxHp = 60;
    int shield = 0;
    int baseCardLimit = 5;   // сколько карт можно сыграть за ход
    int drawPerTurn = 5;     // добор в начале хода; размер руки не ограничен
    std::vector<StatusInst> statuses;
    std::vector<RelicId> relics;

    bool has(RelicId id) const {
        for (RelicId r : relics) if (r == id) return true;
        return false;
    }
    int  statusStacks(StatusId s) const;
    void addStatus(StatusId s, int stacks, int turns);
    void removeNegative();
    bool alive() const { return hp > 0; }
};

enum class CombatState : int { PlayerTurn = 0, Resolving, EnemyTurn, Won, Lost, Fled };

struct FloatText {
    std::string s;
    Vector2 pos{ 0, 0 };
    Color color = WHITE;
    float life = 0.0f, maxLife = 1.0f;
    float size = 20.0f;
};

struct LogLine {
    LS text;
    Color color = WHITE;
    float age = 0.0f;
};

struct CombatRewards {
    int materials[MATERIAL_COUNT] = { 0 };
    std::vector<Card> cardChoices;
    int total() const { int s = 0; for (int i = 0; i < MATERIAL_COUNT; ++i) s += materials[i]; return s; }
};

class Combat {
public:
    void begin(Deck* deck, Player* player, Enemy enemy, int depth, Rng* rng);
    void update(float dt);

    // ── ввод игрока ───────────────────────────────────────────────────────
    bool canPlay(int handIndex) const;
    void playCard(int handIndex);
    // Перестановка карт в руке (перетаскивание карты в промежуток).
    void moveCardInHand(int from, int to);
    // Слияние: карту `from` бросили на карту `onto`. true — слились.
    bool mergeInHand(int from, int onto);
    bool ultimaUsed() const { return ultimaUsed_; }
    void endTurn();
    void flee();
    void cycleTarget(int dir);
    void setTarget(int spotIndex) { target_ = spotIndex; }
    void setBiome(Biome b) { biome_ = b; }
    Biome biome() const { return biome_; }

    // ── состояние для отрисовки ───────────────────────────────────────────
    CombatState state() const { return state_; }
    const Enemy& enemy() const { return enemy_; }
    Enemy& enemyMut() { return enemy_; }
    const std::vector<Card>& hand() const { return hand_; }
    int cardsLeft() const { return cardsLeft_; }
    int cardLimit() const { return cardLimit_; }
    int target() const { return target_; }
    int turn() const { return turn_; }
    ElementId mark() const { return mark_; }
    int markTurns() const { return markTurns_; }
    const std::vector<LogLine>& log() const { return log_; }
    const std::vector<FloatText>& floats() const { return floats_; }
    const CombatRewards& rewards() const { return rewards_; }
    const LS& lastReaction() const { return lastReaction_; }
    float reactionFlash() const { return reactionFlash_; }
    float screenShake() const { return shake_; }
    const LS& telegraph() const { return telegraph_; }
    float movePlay() const { return movePlay_; }
    const EnemyMove& playedMove() const { return playedMove_; }
    int depth() const { return depth_; }

    void pushFloat(const std::string& s, Vector2 p, Color c, float size = 22.0f);
    void pushLog(const LS& t, Color c);

private:
    void drawHand(int n);
    void discardHand();
    void startPlayerTurn();
    void runEnemyTurn();
    void tickStatusesEndOfRound();
    int  dealDamageToEnemy(int dmg, bool precise, Color col);
    void damagePlayer(int dmg, const LS& src);
    void applyReaction(const ReactionDef& rc, const Card& card);
    void resolveElement(const Card& c);
    void resolveCompound(const Card& c);
    float biomeMul(const Card& c) const;
    void resolveSpell(const Card& c);
    void resolveEffect(const Card& c);
    void onVictory();
    void breakSpot(int index);

    Deck*  deck_ = nullptr;
    Player* player_ = nullptr;
    Rng*   rng_ = nullptr;
    Enemy  enemy_;
    int    depth_ = 1;

    std::vector<Card> hand_;
    std::vector<LogLine> log_;
    std::vector<FloatText> floats_;
    CombatRewards rewards_;

    CombatState state_ = CombatState::PlayerTurn;
    int   cardsLeft_ = 5;
    int   cardLimit_ = 5;
    int   turn_ = 1;
    int   target_ = -1;          // -1 = корпус (авто-наведение)
    ElementId mark_ = ElementId::None;
    int   markTurns_ = 0;
    Biome biome_ = Biome::Meadow;   // где принят бой
    bool  ultimaUsed_ = false;
    bool  undyingUsed_ = false;  // Восковой двойник — раз за бой
    bool  firstCardTurn_ = true; // Право первой руки
    float ampBonus_ = 0.0f;      // от карт Amplify
    float timer_ = 0.0f;
    float shake_ = 0.0f;
    float reactionFlash_ = 0.0f;
    float movePlay_ = 0.0f;      // анимация выложенной карты противника
    EnemyMove playedMove_;
    LS    lastReaction_;
    LS    telegraph_;
};

Vector2 spotScreenPos(const Enemy& e, int index, Vector2 center, float size);

} // namespace shichoy
