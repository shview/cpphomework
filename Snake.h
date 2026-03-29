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

    float normalSpeed = 260.f;
    float slowSpeed = 90.f;
    float dashSpeed = 1400.f;

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
    void takeDamage();
    // 返回是否完成解析（触发发射子弹）
    bool update(float dt, sf::Vector2u winSize);
    void draw(sf::RenderWindow& window);
};