#pragma once
#include "Global.h"
#include <vector>
#include <optional>

// 【修改点】增加 PhaseWait (阶段等待) 状态
enum class BossState { Spawning, PhaseTransition, Normal, Hit, Dying, Dead, PhaseWait };

struct Bullet {
    sf::Sprite sprite;
    sf::Vector2f velocity;
    int type;
    Bullet(const sf::Texture& tex, sf::Vector2f vel, int t = 0) : sprite(tex), velocity(vel), type(t) {}
};

struct WarningArea {
    sf::RectangleShape shape;
    float timer;
    float maxTime;
    int bulletType;
    sf::Vector2f startPos;
    sf::Vector2f velocity;
    float rotation;
    bool attachedToBoss = false;
};

struct BossConfig {
    int bossId;
    const sf::Texture* stayTex;
    const sf::Texture* castTex;
    const sf::Texture* sufferTex;
    const sf::Texture* endTex;
    const sf::Texture* bgObjectTex;

    const sf::Texture* texHonestSpecial;
    const sf::Texture* texHimeSpecial;
    const sf::Texture* texBubble;

    AnimInfo animStay;
    AnimInfo animCast;
    AnimInfo animSuffer;
    AnimInfo animEnd;

    float startPosX = 400.f;
    float startPosY = 200.f;
    int forceBulletType = -1;
};

class Boss {
private:
    sf::Vector2f pos;
    float basePosX;
    float basePosY;
    float health;
    float maxHealth;
    float hitboxRadius;
    int currentPhase;

    const sf::Texture* bulletTexture01;
    const sf::Texture* bulletTexture02;
    const sf::Texture* heartOutlineTex;
    const sf::Texture* heartFillTex;

    std::optional<sf::Sprite> bgObjectSprite;
    std::optional<sf::Sprite> bossSprite;
    std::optional<sf::Sprite> heartOutlineSprite;
    std::optional<sf::Sprite> heartFillSprite;
    std::optional<sf::Text> pctText;

    BossConfig config;
    AnimInfo currentAnim;
    int currentFrame;
    float animTimer;

    float rotationAngle;
    float fireTimer;
    float patternTimer;
    float specialTimer;
    float bubbleTimer;
    float targetAttackTimer;

    BossState state;
    float stateTimer;

    std::vector<Bullet> bullets;
    std::vector<WarningArea> warnings;

    void updateBullets(float dt);
    void setAnimation(const AnimInfo& info);
    void updateSpriteRect();
    void spawnWarning(sf::Vector2f startPos, sf::Vector2f size, sf::Vector2f vel, float rot, float maxTime, int type, bool attached = false);

public:
    Boss();
    void init(Difficulty diff, Level lvl, const sf::Font& font,
        const sf::Texture* heartOut, const sf::Texture* heartFill,
        const sf::Texture* bulletTex01, const sf::Texture* bulletTex02,
        const BossConfig& cfg);

    void update(float dt, Difficulty diff, Level lvl, sf::Vector2f playerPos, sf::Vector2f playerDir, sf::Vector2f playerBodyPos);
    void fireBullet(float angleDeg, float speed, int bulletType);
    void draw(sf::RenderWindow& window);

    // 【修改点】暴露 BossState 接口与进入下一阶段的方法
    BossState getState() const { return state; }
    void advancePhase();

    float getHealth() const { return health; }
    float getMaxHealth() const { return maxHealth; }
    sf::Vector2f getPosition() const { return pos; }

    float getBasePosX() const { return basePosX; }
    float getBasePosY() const { return basePosY; }
    void setBasePosX(float x) { basePosX = x; pos.x = x; }
    void setBasePosY(float y) { basePosY = y; pos.y = y; }

    void setPosition(sf::Vector2f p) { pos = p; basePosX = p.x; basePosY = p.y; }
    void kill() { health = 0.f; state = BossState::Dead; bullets.clear(); warnings.clear(); }
    float getHitboxRadius() const { return hitboxRadius; }
    std::vector<Bullet>& getBullets() { return bullets; }
    int getCurrentPhase() const { return currentPhase; }

    bool isDying() const { return state == BossState::Dying; }
    bool isDead() const { return state == BossState::Dead; }
    void takeDamage(float amount);
};