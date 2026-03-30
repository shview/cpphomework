#pragma once
#include "Global.h"
#include "Snake.h"
#include "Boss.h"

struct DataPoint {
    sf::CircleShape shape;
    DataPoint(sf::Vector2f pos) {
        shape.setRadius(7.f);
        shape.setFillColor(sf::Color::Cyan);
        shape.setOrigin({ 7.f, 7.f });
        shape.setPosition(pos);
    }
};

struct Shockwave {
    sf::CircleShape shape;
    float radius = 0.f;
    float maxRadius = 180.f;
    bool isAlive = true;
    Shockwave(sf::Vector2f pos) {
        shape.setPosition(pos);
        shape.setFillColor(sf::Color::Transparent);
        shape.setOutlineColor(sf::Color::White);
        shape.setOutlineThickness(3.f);
    }
    void update(float dt) {
        radius += 500.f * dt;
        shape.setRadius(radius);
        shape.setOrigin({ radius, radius });
        if (radius >= maxRadius) isAlive = false;
    }
};

struct SnakeProjectile {
    sf::Sprite sprite;
    sf::Vector2f pos;
    sf::Vector2f vel;
    float damage;

    SnakeProjectile(const sf::Texture& tex, sf::Vector2f p, float dmg) : sprite(tex) {
        sprite.setOrigin({ tex.getSize().x / 2.f, tex.getSize().y / 2.f });
        pos = p;
        damage = dmg;
        vel = { 0.f, -400.f };
    }

    void update(float dt, sf::Vector2f target) {
        sf::Vector2f desired = target - pos;
        float dist = std::hypot(desired.x, desired.y);
        if (dist > 0.1f) {
            desired = (desired / dist) * 700.f;
            sf::Vector2f steer = desired - vel;
            vel += steer * 4.0f * dt;
        }
        pos += vel * dt;
        sprite.setPosition(pos);
        sprite.setRotation(sf::degrees(std::atan2(vel.y, vel.x) * 180.f / 3.14159f));
    }
};

class Game {
private:
    sf::RenderWindow window;
    sf::Font font;
    GameState state;
    GameStats stats;

    Difficulty currentDiff;
    Level currentLevel;
    int menuSelection;
    int unlockedLevels;

    sf::Texture bulletTex01, bulletTex02;
    sf::Texture snakeHeadTex;
    std::vector<sf::Texture> snakeBodyTexs;
    int currentSkinIndex;
    sf::Texture snakeAttackDotTex;

    sf::Texture heartOutTex, heartFillTex;
    sf::Texture boss1Stay, boss1Cast, boss1Suffer;
    sf::Texture boss2Stay, boss2Cast, boss2Suffer, boss2End, boss2BG;
    sf::Texture honestSpecialTex, himeSpecialTex, bubbleTex;

    sf::Texture previewHimeTex, previewHonestTex;
    std::optional<sf::Text> recommendText;

    Snake player;
    Boss boss;
    Boss boss2;

    std::vector<DataPoint> dataPoints;
    std::vector<Shockwave> shockwaves;
    std::vector<SnakeProjectile> snakeProjectiles;

    float spawnTimer;
    sf::Clock clock;

    // --- 主页预览动画与双 Boss 换位变量 ---
    float honestPreviewTimer = 0.f;
    int honestPreviewFrame = 0;
    float himePreviewTimer = 0.f;
    int himePreviewFrame = 0;

    float bossSwapTimer = 0.f;
    bool isSwapping = false;
    float swapProgress = 0.f;
    sf::Vector2f himeStartPos, honestStartPos;
    sf::Vector2f himeTargetPos, honestTargetPos;

    void processEvents();
    void update(float dt);
    void render();
    void centerText(sf::Text& text, float y);
    void loadResources();
    void startLevel();

public:
    Game();
    void run();
};