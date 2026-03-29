#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdint>
#include <optional>
#include <string>

enum class Difficulty { Easy, Normal, Hard };
enum class Level { Level1, Level2 };

enum class BossState { Spawning, PhaseTransition, Normal, Hit, Dying, Dead };

struct AnimInfo {
    const sf::Texture* tex = nullptr;
    int cols = 1;
    int rows = 1;
    int totalFrames = 1;
    float fps = 12.f;
};

struct Bullet {
    sf::Sprite sprite;
    sf::Vector2f velocity;
    Bullet(const sf::Texture& tex, sf::Vector2f vel) : sprite(tex), velocity(vel) {}
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

    std::optional<sf::Sprite> bossSprite;
    std::optional<sf::Sprite> heartOutlineSprite;
    std::optional<sf::Sprite> heartFillSprite;

    // 修复 error C2512：改为延迟初始化
    std::optional<sf::Text> pctText;

    AnimInfo animStay;
    AnimInfo animCast;
    AnimInfo animSuffer;

    AnimInfo currentAnim;
    int currentFrame;
    float animTimer;

    float rotationAngle;
    float fireTimer;
    float patternTimer;

    BossState state;
    float stateTimer;
    sf::Vector2f baseScale;

    std::vector<Bullet> bullets;

    void updateBullets(float dt);
    void setAnimation(const AnimInfo& info);
    void updateSpriteRect();

public:
    Boss();

    void init(Difficulty diff,
        const sf::Font& font,
        const sf::Texture* heartOut, const sf::Texture* heartFill,
        const sf::Texture* stayTex, const sf::Texture* castTex, const sf::Texture* sufferTex,
        const sf::Texture* bulletTex01, const sf::Texture* bulletTex02);

    void update(float dt, Difficulty diff, Level lvl);
    void fireBullet(float angleDeg, float speed, int bulletType);
    void draw(sf::RenderWindow& window);

    float getHealth() const { return health; }
    float getMaxHealth() const { return maxHealth; }
    sf::Vector2f getPosition() const { return pos; }
    float getHitboxRadius() const { return hitboxRadius; }
    std::vector<Bullet>& getBullets() { return bullets; }
    int getCurrentPhase() const { return currentPhase; }

    bool isDead() const { return state == BossState::Dead; }
    bool isDying() const { return state == BossState::Dying; }

    void takeDamage(float amount);
};