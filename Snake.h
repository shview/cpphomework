#pragma once
#include "Global.h"
#include <deque>
#include <optional>

class Snake {
public:
    std::deque<Pose> trail;
    int bodyCount = 2;
    const int maxBody = 10;
    int gap = 5;

    // 【修改点】速度再次乘以 0.8 倍
    float normalSpeed = 166.4f;
    float slowSpeed = 57.6f;
    float dashSpeed = 896.f;

    sf::Vector2f headPos;
    sf::Vector2f currentDir;

    bool isParsing, isSlowing, isInvincible, isDashing;
    float invTimer, parseProgress, dashTimer, dashCooldown;
    int health, dashCharges;
    float energy;

    std::optional<sf::Sprite> headSprite;
    std::optional<sf::Sprite> bodySprite;

    Snake();
    void initSprites(const sf::Texture& hTex, const sf::Texture& bTex);
    void reset();

    void takeDamage(int amount = 1);
    bool update(float dt, sf::Vector2u winSize);
    void draw(sf::RenderWindow& window);
};