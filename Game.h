#pragma once
#include "Global.h"
#include "Snake.h"
#include "Boss.h"

// 【修改点】加入了特效光圈，透明度随时间消散
struct DataPoint {
    sf::CircleShape shape;
    sf::CircleShape glow;
    int type;
    float timer;
    float maxLifetime;
    bool isFading;

    DataPoint(sf::Vector2f pos, int t = 0) : type(t), timer(0.f), maxLifetime(10.f) {
        shape.setRadius(7.f);
        // type 2 为散落的橙色碎片
        shape.setFillColor(t == 1 ? sf::Color::Green : (t == 2 ? sf::Color(255, 165, 0) : sf::Color::Cyan));
        shape.setOrigin({ 7.f, 7.f });
        shape.setPosition(pos);

        glow.setRadius(12.f);
        glow.setFillColor(sf::Color::Transparent);
        glow.setOutlineColor(shape.getFillColor());
        glow.setOutlineThickness(2.f);
        glow.setOrigin({ 12.f, 12.f });
        glow.setPosition(pos);

        isFading = (t == 2);
    }

    void update(float dt) {
        timer += dt;
        sf::Color c = shape.getFillColor();
        sf::Color gc = glow.getOutlineColor();
        float alpha = 255.f;

        if (isFading) {
            alpha = std::max(0.f, 255.f * (1.f - timer / maxLifetime));
            c.a = static_cast<std::uint8_t>(alpha);
            shape.setFillColor(c);
        }

        // 光圈闪烁效果
        gc.a = static_cast<std::uint8_t>(alpha * (0.3f + 0.7f * std::abs(std::sin(timer * 6.f))));        glow.setOutlineColor(gc);
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
    GameFeelManager feelManager;

    Difficulty currentDiff;
    Level currentLevel;
    int menuSelection;
    int unlockedLevels;
    int bgStyle; // 【新增】0:网格, 1:星空, 2:代码雨, 3:声波圈

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
    float healSpawnTimer;
    sf::Clock clock;

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