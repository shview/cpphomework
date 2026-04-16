#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>
#include <cstdlib>

// 【修改点】增加了 Settings 状态
enum class GameState { Menu, SkinSelect, LevelSelect, DiffSelect, Settings, Playing, Paused, GameOver, Win };
enum class Difficulty { Easy, Normal, Hard };
enum class Level { Level1, Level2, Level3 };

struct GameStats {
    float timeElapsed = 0.f;
    int damageTaken = 0;
    int maxLength = 2;
};

struct Pose { sf::Vector2f position; };

struct AnimInfo {
    const sf::Texture* tex = nullptr;
    int cols = 1;
    int rows = 1;
    int totalFrames = 1;
    float fps = 10.f;
};

class GameFeelManager {
public:
    float shakeIntensity = 0.f;
    float freezeTime = 0.f;

    void triggerHeavyHit() {
        shakeIntensity = 12.0f;
        freezeTime = 0.06f;
    }

    void triggerLightHit() {
        shakeIntensity = 4.0f;
    }

    void update(float dt) {
        if (freezeTime > 0.f) freezeTime -= dt;
        if (shakeIntensity > 0.f) {
            shakeIntensity -= 30.f * dt;
            if (shakeIntensity < 0.f) shakeIntensity = 0.f;
        }
    }

    bool isFrozen() const { return freezeTime > 0.f; }

    sf::Vector2f getShakeOffset() {
        if (shakeIntensity <= 0.f) return { 0.f, 0.f };
        float offsetX = ((std::rand() % 100) / 100.f - 0.5f) * 2.f * shakeIntensity;
        float offsetY = ((std::rand() % 100) / 100.f - 0.5f) * 2.f * shakeIntensity;
        return { offsetX, offsetY };
    }
};