#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>

enum class GameState { Menu, SkinSelect, LevelSelect, DiffSelect, Playing, Paused, GameOver, Win };
enum class Difficulty { Easy, Normal, Hard };
enum class Level { Level1, Level2, Level3 }; // 删除第四关，保留三关

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
    float fps = 10.f; // 全局默认动画帧率改为 10 帧
};