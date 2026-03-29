#pragma once
#include "Global.h"
#include <vector>
#include <optional>

enum class BossState { Spawning, PhaseTransition, Normal, Hit, Dying, Dead };

struct Bullet {
    sf::Sprite sprite;
    sf::Vector2f velocity;
    int type; // 0: ÆÕÍ¨, 10: Honest×¨Êô, 11: Hime×¨Êô, 12: ÅÝÅÝ
    Bullet(const sf::Texture& tex, sf::Vector2f vel, int t = 0) : sprite(tex), velocity(vel), type(t) {}
};

// ÐÂÔö£º¹¥»÷Ô¤¾¯ÇøÓò
struct WarningArea {
    sf::RectangleShape shape;
    float timer;
    float maxTime;
    int bulletType;
    sf::Vector2f startPos;
    sf::Vector2f velocity;
    float rotation;
};

struct BossConfig {
    int bossId; // 1: Honest, 2: Hime
    const sf::Texture* stayTex;
    const sf::Texture* castTex;
    const sf::Texture* sufferTex;
    const sf::Texture* endTex;
    const sf::Texture* bgObjectTex;

    // ÌØÊâ¹¥»÷ÌùÍ¼
    const sf::Texture* texHonestSpecial;
    const sf::Texture* texHimeSpecial;
    const sf::Texture* texBubble;

    AnimInfo animStay;
    AnimInfo animCast;
    AnimInfo animSuffer;
    AnimInfo animEnd;
};

class Boss {
private:
    sf::Vector2f pos;
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

    // ÐÂÔö£ºÌØÊâ¹¥»÷¼ÆÊ±Æ÷
    float specialTimer;
    float bubbleTimer;

    BossState state;
    float stateTimer;

    std::vector<Bullet> bullets;
    std::vector<WarningArea> warnings; // Ô¤¾¯ÏßÈÝÆ÷

    void updateBullets(float dt);
    void setAnimation(const AnimInfo& info);
    void updateSpriteRect();

public:
    Boss();
    void init(Difficulty diff, Level lvl, const sf::Font& font,
        const sf::Texture* heartOut, const sf::Texture* heartFill,
        const sf::Texture* bulletTex01, const sf::Texture* bulletTex02,
        const BossConfig& cfg);

    void update(float dt, Difficulty diff, Level lvl);
    void fireBullet(float angleDeg, float speed, int bulletType);
    void draw(sf::RenderWindow& window);

    float getHealth() const { return health; }
    float getMaxHealth() const { return maxHealth; }
    sf::Vector2f getPosition() const { return pos; }
    float getHitboxRadius() const { return hitboxRadius; }
    std::vector<Bullet>& getBullets() { return bullets; }
    int getCurrentPhase() const { return currentPhase; }

    bool isDying() const { return state == BossState::Dying; }
    bool isDead() const { return state == BossState::Dead; }

    void takeDamage(float amount);
};