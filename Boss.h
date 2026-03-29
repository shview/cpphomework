#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdint>
#include <optional>

enum class Difficulty { Easy, Normal, Hard };
enum class Level { Level1, Level2 };
enum class BossState { Normal, Hit, Dying, Dead };

struct Bullet {
    sf::Sprite sprite;
    sf::Vector2f velocity;
    // SFML 3.0 要求 Sprite 必须由 Texture 初始化
    Bullet(const sf::Texture& tex, sf::Vector2f vel) : sprite(tex), velocity(vel) {}
};

class Boss {
private:
    sf::Vector2f pos;
    float health;
    float maxHealth;
    float hitboxRadius;

    std::optional<sf::Sprite> bossSprite;

    // 两种子弹的贴图指针
    const sf::Texture* bulletTexture01;
    const sf::Texture* bulletTexture02;

    float rotationAngle;
    float fireTimer;
    float patternTimer;
    int currentPattern;

    BossState state;
    float hitTimer;
    float deathTimer;
    sf::Vector2f baseScale;

    std::vector<Bullet> bullets;

    void updateBullets(float dt);

public:
    Boss();

    // 初始化时传入两种子弹贴图
    void init(Difficulty diff, const sf::Texture* bossTex, const sf::Texture* bulletTex01, const sf::Texture* bulletTex02);
    void update(float dt, Difficulty diff, Level lvl);

    // 发射子弹时指定类型 (0 或 1)
    void fireBullet(float angleDeg, float speed, int bulletType);
    void draw(sf::RenderWindow& window);

    float getHealth() const { return health; }
    float getMaxHealth() const { return maxHealth; }
    sf::Vector2f getPosition() const { return pos; }
    float getHitboxRadius() const { return hitboxRadius; }
    std::vector<Bullet>& getBullets() { return bullets; }

    bool isDead() const { return state == BossState::Dead; }
    bool isDying() const { return state == BossState::Dying; }

    void takeDamage(float amount);
};