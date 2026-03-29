#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <deque>
#include <vector>
#include <cmath>
#include <algorithm>
#include <ctime>
#include <iostream>
#include <string>
#include <optional>
#include "Boss.h"

struct Pose { sf::Vector2f position; };

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

enum class GameState { Menu, LevelSelect, DiffSelect, Playing, GameOver, Win };

struct GameStats {
    float timeElapsed = 0.f;
    int damageTaken = 0;
    int maxLength = 2;
} stats;

void centerText(sf::Text& text, float y) {
    sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin({ bounds.size.x / 2.0f, bounds.size.y / 2.0f });
    text.setPosition({ 400.f, y });
}

// ================== 玩家 Snake 类 ==================
class Snake {
public:
    std::deque<Pose> trail;
    int bodyCount = 2;
    const int maxBody = 10;
    int gap = 5;

    float normalSpeed = 260.f;
    float slowSpeed = 90.f;
    float dashSpeed = 1400.f;

    sf::Vector2f headPos{ 400.f, 450.f };
    sf::Vector2f currentDir{ 1.f, 0.f }; // 默认向右

    bool isParsing = false;
    bool isSlowing = false;
    bool isInvincible = false;
    float invTimer = 0.f;

    float parseProgress = 0.f;
    int health = 3;

    bool isDashing = false;
    float dashTimer = 0.f;
    int dashCharges = 2;
    float dashCooldown = 0.f;

    float energy = 100.f;

    // SFML 3.0: 贴图载体
    std::optional<sf::Sprite> headSprite;
    std::optional<sf::Sprite> bodySprite;

    Snake() {}

    // 初始化时绑定贴图，并设置中心点
    void initSprites(const sf::Texture& hTex, const sf::Texture& bTex) {
        headSprite.emplace(hTex);
        bodySprite.emplace(bTex);
        headSprite->setOrigin({ hTex.getSize().x / 2.0f, hTex.getSize().y / 2.0f });
        bodySprite->setOrigin({ bTex.getSize().x / 2.0f, bTex.getSize().y / 2.0f });
    }

    void reset() {
        headPos = { 400.f, 450.f };
        currentDir = { 1.f, 0.f }; // 重置时也向右
        bodyCount = 2;
        health = 3;
        energy = 100.f;
        dashCharges = 2;
        trail.clear();
        isDashing = false;
        isInvincible = false;
        parseProgress = 0.f;
    }

    void takeDamage() {
        health--;
        stats.damageTaken++;
        isInvincible = true;
        invTimer = 1.0f;
        bodyCount = std::max(2, bodyCount - 2);
    }

    void update(float dt, sf::Vector2u winSize, Boss& boss) {
        if (isInvincible) {
            invTimer -= dt;
            if (invTimer <= 0.f) isInvincible = false;
        }

        energy = std::min(100.f, energy + 12.f * dt);

        if (dashCharges < 2) {
            dashCooldown += dt;
            if (dashCooldown >= 2.5f) {
                dashCharges++;
                dashCooldown = 0.f;
            }
        }

        float currentMoveSpeed = normalSpeed;
        bool atEdge = (headPos.x <= 15.f || headPos.x >= (float)winSize.x - 15.f ||
            headPos.y <= 15.f || headPos.y >= (float)winSize.y - 15.f);

        if (isDashing) {
            currentMoveSpeed = dashSpeed;
            dashTimer -= dt;
            if (dashTimer <= 0.f) isDashing = false;
        }
        else {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || atEdge) {
                currentMoveSpeed = slowSpeed;
                isSlowing = true;
                isParsing = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
            }
            else {
                isSlowing = false;
                isParsing = false;
            }
        }

        sf::Vector2f prevPos = headPos;

        if (!isDashing) {
            sf::Vector2f inputDir(0.f, 0.f);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))         inputDir.y = -1.f;
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))  inputDir.y = 1.f;
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))  inputDir.x = -1.f;
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) inputDir.x = 1.f;

            bool hasInput = (inputDir.x != 0.f || inputDir.y != 0.f);
            if (hasInput) currentDir = inputDir;

            if (!isSlowing || hasInput) {
                headPos += currentDir * currentMoveSpeed * dt;
            }
        }
        else {
            headPos += currentDir * currentMoveSpeed * dt;
        }

        // --- 核心：根据行进方向计算蛇头的旋转角度 ---
        if (headSprite) {
            float angle = 0.f;
            if (currentDir.x > 0.f) angle = 0.f;         // 右
            else if (currentDir.x < 0.f) angle = 180.f;  // 左
            else if (currentDir.y > 0.f) angle = 90.f;   // 下
            else if (currentDir.y < 0.f) angle = -90.f;  // 上 (270)
            headSprite->setRotation(sf::degrees(angle));
        }
        // --------------------------------------------

        headPos.x = std::clamp(headPos.x, 15.f, (float)winSize.x - 15.f);
        headPos.y = std::clamp(headPos.y, 15.f, (float)winSize.y - 15.f);

        float distMoved = std::hypot(headPos.x - prevPos.x, headPos.y - prevPos.y);
        if (distMoved > 0.1f) {
            int steps = static_cast<int>(std::ceil(distMoved / 3.0f));
            for (int i = 1; i <= steps; ++i) {
                sf::Vector2f interpPos = prevPos + (headPos - prevPos) * ((float)i / steps);
                trail.push_front({ interpPos });
            }
        }

        if (trail.size() > (size_t)(maxBody + 2) * gap) trail.resize((maxBody + 2) * gap);

        if (isParsing && bodyCount >= 3) {
            parseProgress += (30.f + (bodyCount - 3) * 15.f) * dt;
            if (parseProgress >= 100.f) {
                boss.takeDamage(10.f + bodyCount * 8.f);
                parseProgress = 0.f;
                bodyCount = 2;
                trail.clear();
            }
        }
        else {
            parseProgress = std::max(0.f, parseProgress - 50.f * dt);
        }
    }

    void draw(sf::RenderWindow& window) {
        if (!headSprite || !bodySprite) return; // 贴图没加载成功时不绘制

        // 冲刺残影
        if (isDashing) {
            for (size_t i = 0; i < std::min((size_t)15, trail.size()); i += 3) {
                sf::Sprite ghost = *headSprite;
                ghost.setPosition(trail[i].position);
                ghost.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(120 - i * 6)));
                window.draw(ghost);
            }
        }

        // 绘制身体
        if (!isSlowing) {
            for (int i = 1; i <= bodyCount; ++i) {
                size_t idx = i * gap;
                if (idx < trail.size()) {
                    bodySprite->setPosition(trail[idx].position);
                    window.draw(*bodySprite);
                }
            }
        }

        // 绘制头部及状态变色
        headSprite->setPosition(headPos);
        if (isInvincible && static_cast<int>(invTimer * 15) % 2 == 0) {
            headSprite->setColor(sf::Color(255, 255, 255, 0)); // 隐身闪烁
        }
        else {
            // 解析时染成偏黄色，正常时白色（保持贴图原色）
            headSprite->setColor(isParsing ? sf::Color(255, 255, 0) : sf::Color::White);
        }
        window.draw(*headSprite);

        // 解析进度条
        if (isParsing && bodyCount >= 3) {
            sf::RectangleShape bar({ 40.f * (parseProgress / 100.f), 4.f });
            bar.setPosition({ headPos.x - 20.f, headPos.y - 25.f });
            bar.setFillColor(sf::Color::Yellow);
            window.draw(bar);
        }
    }
};

// ================== Main ==================
int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    sf::RenderWindow window(sf::VideoMode({ 800, 600 }), "Last Command - Modded");
    window.setFramerateLimit(60);

    // ================== 在 main 函数中，修改加载资源的部分 ==================
    sf::Font font;
    (void)font.openFromFile("arial.ttf"); // 加上 (void) 消除警告

    sf::Texture bulletTex01, bulletTex02;
    (void)bulletTex01.loadFromFile("assets/bulletBoss01.png");
    (void)bulletTex02.loadFromFile("assets/bulletBoss02.png");

    sf::Texture snakeHeadTex, snakeBodyTex;
    (void)snakeHeadTex.loadFromFile("assets/snakeSkinHead.png");
    (void)snakeBodyTex.loadFromFile("assets/snakeSkinBody.png");

    sf::Texture bossStayTex, bossCastTex, bossSufferTex;
    (void)bossStayTex.loadFromFile("assets/BossHonest_Stay.png");
    (void)bossCastTex.loadFromFile("assets/BossHonest_Cast01.png");
    (void)bossSufferTex.loadFromFile("assets/BossHonest_Suffer.png");

    sf::Texture heartOutTex, heartFillTex;
    (void)heartOutTex.loadFromFile("assets/levelHeart20x22OutlineForHonest.png");
    (void)heartFillTex.loadFromFile("assets/levelHeart20x22.png");
    // =======================================================================

    Snake player;
    player.initSprites(snakeHeadTex, snakeBodyTex);

    Boss boss;
    std::vector<DataPoint> dataPoints;
    std::vector<Shockwave> shockwaves;

    float spawnTimer = 0.f;
    sf::Clock clock;

    GameState state = GameState::Menu;
    Difficulty currentDiff = Difficulty::Normal;
    Level currentLevel = Level::Level1;
    int menuSelection = 0;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();

            if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                // ... 菜单逻辑保持不变 ...
                if (state == GameState::Menu) {
                    if (keyEvent->code == sf::Keyboard::Key::Up) menuSelection = 0;
                    if (keyEvent->code == sf::Keyboard::Key::Down) menuSelection = 1;
                    if (keyEvent->code == sf::Keyboard::Key::Enter) {
                        if (menuSelection == 0) state = GameState::LevelSelect;
                        else window.close();
                        menuSelection = 0;
                    }
                }
                else if (state == GameState::LevelSelect) {
                    if (keyEvent->code == sf::Keyboard::Key::Up) menuSelection = 0;
                    if (keyEvent->code == sf::Keyboard::Key::Down) menuSelection = 1;
                    if (keyEvent->code == sf::Keyboard::Key::Enter) {
                        currentLevel = (menuSelection == 0) ? Level::Level1 : Level::Level2;
                        state = GameState::DiffSelect;
                        menuSelection = 0;
                    }
                }
                else if (state == GameState::DiffSelect) {
                    if (keyEvent->code == sf::Keyboard::Key::Up) menuSelection = std::max(0, menuSelection - 1);
                    if (keyEvent->code == sf::Keyboard::Key::Down) menuSelection = std::min(2, menuSelection + 1);
                    if (keyEvent->code == sf::Keyboard::Key::Enter) {
                        if (menuSelection == 0) currentDiff = Difficulty::Easy;
                        else if (menuSelection == 1) currentDiff = Difficulty::Normal;
                        else currentDiff = Difficulty::Hard;

                        player.reset();
                        // 初始化 Boss，注入所有贴图资源
                        boss.init(currentDiff, font, &heartOutTex, &heartFillTex,
                            &bossStayTex, &bossCastTex, &bossSufferTex,
                            &bulletTex01, &bulletTex02);

                        dataPoints.clear();
                        shockwaves.clear();
                        stats = GameStats();
                        state = GameState::Playing;
                    }
                }
                else if (state == GameState::Playing) {
                    if (keyEvent->code == sf::Keyboard::Key::Q && player.energy >= 35.f) {
                        player.energy -= 35.f;
                        shockwaves.push_back(Shockwave(player.headPos));
                    }
                    // 冲刺支持 LShift 或 F 键
                    if ((keyEvent->code == sf::Keyboard::Key::LShift || keyEvent->code == sf::Keyboard::Key::F)
                        && player.dashCharges > 0 && !player.isDashing) {
                        player.dashCharges--;
                        player.isDashing = true;
                        player.dashTimer = 0.15f;
                        player.isInvincible = true;
                        player.invTimer = 0.40f;
                    }
                    // F1 一键秒杀当前阶段 (测试用)
                    if (keyEvent->code == sf::Keyboard::Key::F1) {
                        boss.takeDamage(boss.getMaxHealth());
                    }
                }
                else if (state == GameState::GameOver || state == GameState::Win) {
                    if (keyEvent->code == sf::Keyboard::Key::Enter) {
                        state = GameState::Menu;
                        menuSelection = 0;
                    }
                }
            }
        }

        if (state == GameState::Playing) {
            stats.timeElapsed += dt;

            if (!boss.isDead() && !boss.isDying()) {
                if (dataPoints.empty()) {
                    spawnTimer += dt;
                    if (spawnTimer > 1.2f) {
                        sf::Vector2f spawnPos;
                        do {
                            spawnPos = { (float)(rand() % 700 + 50), (float)(rand() % 400 + 100) };
                        } while (std::hypot(spawnPos.x - boss.getPosition().x, spawnPos.y - boss.getPosition().y) < 120.f);

                        dataPoints.push_back(DataPoint(spawnPos));
                        spawnTimer = 0.f;
                    }
                }
            }

            // --- 玩家与 Boss 核心碰撞更新 --- (使用之前的 C++20 erase_if)
            player.update(dt, window.getSize(), boss);
            boss.update(dt, currentDiff, currentLevel);
            stats.maxLength = std::max(stats.maxLength, player.bodyCount);

            for (auto& sw : shockwaves) sw.update(dt);
            (void)std::erase_if(shockwaves, [](const Shockwave& sw) { return !sw.isAlive; });

            (void)std::erase_if(dataPoints, [&](const DataPoint& dp) {
                if (std::hypot(player.headPos.x - dp.shape.getPosition().x, player.headPos.y - dp.shape.getPosition().y) < 20.f) {
                    if (player.bodyCount < player.maxBody) player.bodyCount++;
                    return true;
                }
                return false;
                });

            std::vector<Bullet>& bossBullets = boss.getBullets();
            for (const auto& sw : shockwaves) {
                (void)std::erase_if(bossBullets, [&](const Bullet& b) {
                    return std::hypot(sw.shape.getPosition().x - b.sprite.getPosition().x, sw.shape.getPosition().y - b.sprite.getPosition().y) < sw.radius;
                    });
            }

            if (!player.isInvincible) {
                for (auto it = bossBullets.begin(); it != bossBullets.end(); ) {
                    if (std::hypot(player.headPos.x - it->sprite.getPosition().x, player.headPos.y - it->sprite.getPosition().y) < 12.f) {
                        player.takeDamage();
                        shockwaves.push_back(Shockwave(player.headPos));
                        it = bossBullets.erase(it);
                        break;
                    }
                    else ++it;
                }
            }

            if (player.health <= 0) state = GameState::GameOver;
            if (boss.isDead()) state = GameState::Win;
        }

        window.clear(sf::Color(15, 15, 20));

        if (state == GameState::Playing) {
            boss.draw(window);
            for (auto& dp : dataPoints) window.draw(dp.shape);
            for (auto& sw : shockwaves) window.draw(sw.shape);
            player.draw(window);

            // --- UI 渲染 ---
            // 1. 玩家血量 (绿色方块)
            for (int i = 0; i < player.health; ++i) {
                sf::RectangleShape hb({ 16.f, 16.f });
                hb.setPosition({ 20.f + i * 22.f, 20.f });
                hb.setFillColor(sf::Color::Green);
                window.draw(hb);
            }

            // 2. 冲刺次数指示灯 (拥有次数为亮黄，空缺为暗黄)
            for (int i = 0; i < 2; ++i) {
                sf::CircleShape dashDot(6.f);
                dashDot.setPosition({ 20.f + i * 16.f, 45.f });
                if (i < player.dashCharges) {
                    dashDot.setFillColor(sf::Color::Yellow);
                }
                else {
                    dashDot.setFillColor(sf::Color(100, 100, 0, 150)); // 暗色底槽
                }
                window.draw(dashDot);
            }

            // 3. 冲刺充能进度条 (只有次数不满 2 时显示)
            if (player.dashCharges < 2) {
                sf::RectangleShape dashBarBG({ 30.f, 4.f });
                dashBarBG.setPosition({ 20.f, 59.f });
                dashBarBG.setFillColor(sf::Color(50, 50, 0));
                window.draw(dashBarBG);

                sf::RectangleShape dashBar({ 30.f * (player.dashCooldown / 2.5f), 4.f });
                dashBar.setPosition({ 20.f, 59.f });
                dashBar.setFillColor(sf::Color::Yellow);
                window.draw(dashBar);
            }

            // 4. Q技能能量条 (青色)
            sf::RectangleShape eb({ 100.f * (player.energy / 100.f), 6.f });
            eb.setPosition({ 20.f, 67.f }); // 位置稍微下移，给冲刺条留空间
            eb.setFillColor(sf::Color::Cyan);
            window.draw(eb);

            // --- 新增：Boss 右上角多阶段进度条 ---
            sf::Text phaseText(font, "PHASE", 14);
            phaseText.setPosition({ 620.f, 20.f });
            phaseText.setFillColor(sf::Color::White);
            window.draw(phaseText);

            int currentP = boss.getCurrentPhase();
            for (int i = 0; i < 3; ++i) {
                sf::RectangleShape pb({ 30.f, 12.f });
                pb.setPosition({ 680.f + i * 35.f, 22.f });

                // 如果是已经打过的阶段标红，当前阶段或未来阶段标暗灰
                if (i <= currentP) pb.setFillColor(sf::Color::Red);
                else pb.setFillColor(sf::Color(50, 0, 0));

                // 给当前正在打的阶段加个高亮边框提示
                if (i == currentP) {
                    pb.setOutlineThickness(2.f);
                    pb.setOutlineColor(sf::Color::Yellow);
                }
                window.draw(pb);
            }
        }
        else {
            // ... 非游玩菜单 UI 代码保持不变 ...
            sf::Text title(font, "", 40);
            sf::Text opt1(font, "", 24);
            sf::Text opt2(font, "", 24);
            sf::Text opt3(font, "", 24);

            if (state == GameState::Menu) {
                title.setString("LAST COMMAND - MODDED");
                opt1.setString("Start Game");
                opt2.setString("Exit");
                opt1.setFillColor(menuSelection == 0 ? sf::Color::Yellow : sf::Color::White);
                opt2.setFillColor(menuSelection == 1 ? sf::Color::Yellow : sf::Color::White);
            }
            else if (state == GameState::LevelSelect) {
                title.setString("SELECT LEVEL");
                opt1.setString("Level 1: Static Core");
                opt2.setString("Level 2: Moving Target");
                opt1.setFillColor(menuSelection == 0 ? sf::Color::Yellow : sf::Color::White);
                opt2.setFillColor(menuSelection == 1 ? sf::Color::Yellow : sf::Color::White);
            }
            else if (state == GameState::DiffSelect) {
                title.setString("SELECT DIFFICULTY");
                opt1.setString("Easy");
                opt2.setString("Normal");
                opt3.setString("Hard");
                opt1.setFillColor(menuSelection == 0 ? sf::Color::Yellow : sf::Color::White);
                opt2.setFillColor(menuSelection == 1 ? sf::Color::Yellow : sf::Color::White);
                opt3.setFillColor(menuSelection == 2 ? sf::Color::Yellow : sf::Color::White);
            }
            else if (state == GameState::GameOver || state == GameState::Win) {
                title.setString(state == GameState::Win ? "MISSION ACCOMPLISHED" : "SYSTEM FAILURE");
                title.setFillColor(state == GameState::Win ? sf::Color::Green : sf::Color::Red);
                opt1.setString("Time: " + std::to_string((int)stats.timeElapsed) + "s");
                opt2.setString("Max Length: " + std::to_string(stats.maxLength));
                opt3.setString("Damage Taken: " + std::to_string(stats.damageTaken));
                sf::Text hint(font, "Press ENTER to return", 18);
                centerText(hint, 450.f);
                window.draw(hint);
            }

            centerText(title, 150.f); window.draw(title);
            centerText(opt1, 280.f);  window.draw(opt1);
            centerText(opt2, 330.f);  window.draw(opt2);
            if (opt3.getString() != "") { centerText(opt3, 380.f); window.draw(opt3); }
        }

        window.display();
    }

    return 0;
}