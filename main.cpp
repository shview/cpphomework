#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <deque>
#include <vector>
#include <cmath>
#include <algorithm>
#include <ctime>
#include <iostream>
#include <string>

// ==========================================
// 1. 基础结构体与辅助工具
// ==========================================

// 记录坐标的点，用于蛇身跟随轨迹
struct Pose {
    sf::Vector2f position;
};

// 掉落的“数据”点：吃掉它会增加蛇的长度
struct DataPoint {
    sf::CircleShape shape;
    DataPoint(sf::Vector2f pos) {
        shape.setRadius(8.f);
        shape.setFillColor(sf::Color::Cyan);
        shape.setOrigin({ 8.f, 8.f }); // 设置中心点，方便碰撞计算
        shape.setPosition(pos);
    }
};

// Boss 发射的子弹
struct Bullet {
    sf::CircleShape shape;
    sf::Vector2f velocity;
};

// 冲击波：按下 Q 或受击时触发，能消除范围内子弹
struct Shockwave {
    sf::CircleShape shape;
    float radius = 0.f;
    float maxRadius = 160.f;
    bool isAlive = true;

    Shockwave(sf::Vector2f pos) {
        shape.setPosition(pos);
        shape.setFillColor(sf::Color::Transparent);
        shape.setOutlineColor(sf::Color::White);
        shape.setOutlineThickness(3.f);
    }

    void update(float dt) {
        radius += 450.f * dt; // 冲击波扩散速度
        shape.setRadius(radius);
        shape.setOrigin({ radius, radius });
        if (radius >= maxRadius) isAlive = false; // 达到最大半径后消失
    }
};

// ==========================================
// 2. 玩家类：蛇
// ==========================================

class Snake {
public:
    std::deque<Pose> trail;    // 核心：存储头部的历史位置
    int bodyCount = 2;         // 当前身体长度（不含头）
    const int maxBody = 10;    // 最大长度上限
    int gap = 6;               // 身体节与节之间的历史点间隔（越小越紧凑）

    float normalSpeed = 260.f; // 正常速度
    float slowSpeed = 90.f;    // 慢速/解析速度
    float dashSpeed = 1300.f;  // 冲刺速度

    sf::Vector2f headPos{ 400.f, 450.f }; // 起始位置
    sf::Vector2f currentDir{ 0.f, -1.f }; // 初始方向（向上）

    // 状态量
    bool isParsing = false;    // 是否处于解析模式
    bool isSlowing = false;    // 是否减速（Space/解析/碰边）
    bool isInvincible = false; // 是否无敌
    float invTimer = 0.f;      // 无敌剩余时间

    float parseProgress = 0.f; // 解析进度 0-100
    int health = 3;            // 玩家生命值

    bool isDashing = false;    // 是否正在冲刺
    float dashTimer = 0.f;     // 冲刺持续计时
    int dashCharges = 2;       // 冲刺可用次数
    float dashCooldown = 0.f;  // 冲刺回复计时

    float energy = 100.f;      // 能量值

    sf::RectangleShape headShape;
    sf::RectangleShape bodyShape;

    Snake() {
        // 初始化头部形状 (白色方块)
        headShape.setSize({ 20.f, 20.f });
        headShape.setOrigin({ 10.f, 10.f });
        headShape.setFillColor(sf::Color::White);

        // 初始化身体形状 (灰色方块)
        bodyShape.setSize({ 16.f, 16.f });
        bodyShape.setOrigin({ 8.f, 8.f });
        bodyShape.setFillColor(sf::Color(200, 200, 200));
    }

    // 重置玩家状态（用于重新开始游戏）
    void reset() {
        headPos = { 400.f, 450.f };
        bodyCount = 2;
        health = 3;
        energy = 100.f;
        dashCharges = 2;
        trail.clear();
        isDashing = false;
        isInvincible = false;
    }

    void update(float dt, sf::Vector2u winSize, float& bossHealth) {
        // --- 1. 自动回复与计时器 ---
        if (isInvincible) {
            invTimer -= dt;
            if (invTimer <= 0.f) isInvincible = false;
        }
        energy = std::min(100.f, energy + 12.f * dt); // 随时间回能

        // 冲刺次数恢复逻辑
        if (dashCharges < 2) {
            dashCooldown += dt;
            if (dashCooldown >= 2.5f) {
                dashCharges++;
                dashCooldown = 0.f;
            }
        }

        // --- 2. 确定当前速度模式 ---
        float currentMoveSpeed = normalSpeed;

        // 判定：是否碰到边界
        bool atEdge = (headPos.x <= 15.f || headPos.x >= (float)winSize.x - 15.f ||
            headPos.y <= 15.f || headPos.y >= (float)winSize.y - 15.f);

        if (isDashing) {
            currentMoveSpeed = dashSpeed;
            dashTimer -= dt;
            if (dashTimer <= 0.f) isDashing = false;
        }
        else {
            // 按下 Space 或 D，或者碰到边界，都会进入减速(体长变为1)状态
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

        // --- 3. 移动逻辑 ---
        if (!isDashing) {
            sf::Vector2f inputDir(0.f, 0.f);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))         inputDir.y = -1.f;
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))  inputDir.y = 1.f;
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))  inputDir.x = -1.f;
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) inputDir.x = 1.f;

            if (inputDir.x != 0.f || inputDir.y != 0.f) {
                currentDir = inputDir; // 仅在有输入时更新方向
                headPos += currentDir * currentMoveSpeed * dt;
            }
        }
        else {
            // 冲刺时强制沿当前方向移动
            headPos += currentDir * currentMoveSpeed * dt;
        }

        // 限制在窗口内
        headPos.x = std::clamp(headPos.x, 12.f, (float)winSize.x - 12.f);
        headPos.y = std::clamp(headPos.y, 12.f, (float)winSize.y - 12.f);

        // --- 4. 解析攻击逻辑 ---
        if (isParsing && bodyCount >= 3) {
            // 长度越长，解析越快
            parseProgress += (25.f + (bodyCount - 3) * 15.f) * dt;
            if (parseProgress >= 100.f) {
                bossHealth -= 20.f; // 造成伤害
                parseProgress = 0.f;
                bodyCount = 2;      // 解析消耗掉多余身体
                trail.clear();
            }
        }
        else {
            parseProgress = std::max(0.f, parseProgress - 40.f * dt); // 不解析时进度衰减
        }

        // --- 5. 更新轨迹记录 ---
        trail.push_front({ headPos });
        // 保持轨迹长度刚好足够所有身体节数使用即可，节省内存
        if (trail.size() > (size_t)(bodyCount + 1) * gap) trail.pop_back();
    }

    void draw(sf::RenderWindow& window) {
        // 只有不处于“停下/解析”状态时才绘制身体
        if (!isSlowing) {
            for (int i = 1; i <= bodyCount; ++i) {
                size_t idx = i * gap;
                if (idx < trail.size()) {
                    bodyShape.setPosition(trail[idx].position);
                    window.draw(bodyShape);
                }
            }
        }

        // 绘制头
        headShape.setPosition(headPos);
        // 无敌时通过闪烁（透明度切换）来反馈
        if (isInvincible && static_cast<int>(invTimer * 12) % 2 == 0) {
            headShape.setFillColor(sf::Color::Transparent);
        }
        else {
            headShape.setFillColor(isParsing ? sf::Color::Yellow : sf::Color::White);
        }
        window.draw(headShape);

        // 绘制解析进度条（仅在解析时显示在头上方）
        if (isParsing && bodyCount >= 3) {
            sf::RectangleShape bar({ 40.f * (parseProgress / 100.f), 4.f });
            bar.setPosition({ headPos.x - 20.f, headPos.y - 25.f });
            bar.setFillColor(sf::Color::Yellow);
            window.draw(bar);
        }
    }
};

// ==========================================
// 3. 敌人类：Boss
// ==========================================

class Boss {
public:
    float rotationAngle = 0.f;
    float fireTimer = 0.f;
    std::vector<Bullet> bullets;
    sf::Vector2f pos{ 400.f, 220.f };
    float health = 100.f;

    void reset() {
        health = 100.f;
        bullets.clear();
        rotationAngle = 0.f;
    }

    void update(float dt) {
        rotationAngle += 100.f * dt; // 旋转弹幕发射器
        fireTimer += dt;

        // 定时发射弹幕
        if (fireTimer > 0.15f) {
            fireTimer = 0.f;
            for (int i : {0, 90, 180, 270}) { // 十字发射
                float angle = (rotationAngle + i) * 3.14159f / 180.f;
                Bullet b;
                b.shape.setRadius(6.f);
                b.shape.setFillColor(sf::Color::Red);
                b.shape.setOrigin({ 6.f, 6.f });
                b.shape.setPosition(pos);
                b.velocity = { std::cos(angle) * 240.f, std::sin(angle) * 240.f };
                bullets.push_back(b);
            }
        }

        // 更新子弹位置并清理出界子弹
        for (auto it = bullets.begin(); it != bullets.end(); ) {
            it->shape.move(it->velocity * dt);
            sf::Vector2f p = it->shape.getPosition();
            if (p.x < -30 || p.x > 830 || p.y < -30 || p.y > 630) it = bullets.erase(it);
            else ++it;
        }
    }

    void draw(sf::RenderWindow& window) {
        sf::CircleShape core(40.f);
        core.setOrigin({ 40.f, 40.f });
        core.setPosition(pos);
        core.setFillColor(sf::Color(180, 0, 0));
        window.draw(core);
        for (auto& b : bullets) window.draw(b.shape);
    }
};

// ==========================================
// 4. 游戏状态控制与 UI 系统
// ==========================================

enum class GameState { Menu, Playing, GameOver, Win };

int main() {
    std::srand((unsigned int)std::time(nullptr));
    sf::RenderWindow window(sf::VideoMode({ 800, 600 }), "Last Command Study Edition");
    window.setFramerateLimit(60);

    // 加载字体 (请确保目录下有该文件)
    sf::Font font;
    if (!font.openFromFile("arial.ttf")) {
        std::cout << "Error: Could not load font. Please place arial.ttf in the folder.\n";
    }

    Snake player;
    Boss boss;
    std::vector<DataPoint> dataPoints;
    std::vector<Shockwave> shockwaves;
    float spawnTimer = 0.f;
    sf::Clock clock;
    GameState state = GameState::Menu;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();

        // --- 事件处理 ---
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();

            if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                if (state == GameState::Menu) {
                    if (keyEvent->code == sf::Keyboard::Key::Enter) state = GameState::Playing;
                }
                else if (state == GameState::Playing) {
                    // Q 冲击波
                    if (keyEvent->code == sf::Keyboard::Key::Q && player.energy >= 30.f) {
                        player.energy -= 30.f;
                        shockwaves.push_back(Shockwave(player.headPos));
                    }
                    // LShift 冲刺
                    if (keyEvent->code == sf::Keyboard::Key::LShift && player.dashCharges > 0 && !player.isDashing) {
                        player.dashCharges--;
                        player.isDashing = true;
                        player.dashTimer = 0.12f;
                        player.isInvincible = true;
                        player.invTimer = 0.45f;
                    }
                }
                else { // GameOver 或 Win 状态
                    if (keyEvent->code == sf::Keyboard::Key::R) { // 重来
                        player.reset();
                        boss.reset();
                        dataPoints.clear();
                        shockwaves.clear();
                        state = GameState::Playing;
                    }
                    if (keyEvent->code == sf::Keyboard::Key::M) { // 返回菜单
                        player.reset();
                        boss.reset();
                        state = GameState::Menu;
                    }
                }
            }
        }

        // --- 逻辑更新 ---
        if (state == GameState::Playing) {
            // 数据点生成
            if (dataPoints.empty()) {
                spawnTimer += dt;
                if (spawnTimer > 1.0f) {
                    dataPoints.push_back(DataPoint({ (float)(rand() % 700 + 50), (float)(rand() % 400 + 100) }));
                    spawnTimer = 0.f;
                }
            }

            player.update(dt, window.getSize(), boss.health);
            boss.update(dt);

            // 更新冲击波
            for (auto it = shockwaves.begin(); it != shockwaves.end(); ) {
                it->update(dt);
                if (!it->isAlive) it = shockwaves.erase(it);
                else ++it;
            }

            // 碰撞：捡数据
            for (auto it = dataPoints.begin(); it != dataPoints.end(); ) {
                if (std::hypot(player.headPos.x - it->shape.getPosition().x, player.headPos.y - it->shape.getPosition().y) < 22.f) {
                    if (player.bodyCount < player.maxBody) player.bodyCount++;
                    it = dataPoints.erase(it);
                }
                else ++it;
            }

            // 碰撞：冲击波消弹幕
            for (auto& sw : shockwaves) {
                boss.bullets.erase(std::remove_if(boss.bullets.begin(), boss.bullets.end(), [&](const Bullet& b) {
                    return std::hypot(sw.shape.getPosition().x - b.shape.getPosition().x, sw.shape.getPosition().y - b.shape.getPosition().y) < sw.radius;
                    }), boss.bullets.end());
            }

            // 碰撞：玩家受击
            if (!player.isInvincible) {
                for (auto it = boss.bullets.begin(); it != boss.bullets.end(); ) {
                    if (std::hypot(player.headPos.x - it->shape.getPosition().x, player.headPos.y - it->shape.getPosition().y) < 14.f) {
                        player.health--;
                        player.isInvincible = true;
                        player.invTimer = 1.0f;
                        shockwaves.push_back(Shockwave(player.headPos)); // 受伤被动触发冲击波
                        it = boss.bullets.erase(it);
                        break;
                    }
                    else ++it;
                }
            }

            // 检查胜负
            if (player.health <= 0) state = GameState::GameOver;
            if (boss.health <= 0) state = GameState::Win;
        }

        // --- 渲染部分 ---
        window.clear(sf::Color(10, 10, 15));

        if (state == GameState::Menu) {
            sf::Text title(font, "LAST COMMAND CLONE", 40);
            title.setPosition({ 200, 200 });
            window.draw(title);
            sf::Text hint(font, "Press ENTER to Start Game", 20);
            hint.setPosition({ 260, 350 });
            window.draw(hint);
        }
        else if (state == GameState::Playing) {
            boss.draw(window);
            for (auto& dp : dataPoints) window.draw(dp.shape);
            for (auto& sw : shockwaves) window.draw(sw.shape);
            player.draw(window);

            // --- UI: 玩家血量与能量数值 ---
            for (int i = 0; i < player.health; ++i) {
                sf::RectangleShape hb({ 18.f, 18.f });
                hb.setPosition({ 20.f + i * 24.f, 20.f });
                hb.setFillColor(sf::Color::Green);
                window.draw(hb);
            }
            // 能量条
            sf::RectangleShape eb({ 100.f * (player.energy / 100.f), 8.f });
            eb.setPosition({ 20.f, 45.f });
            eb.setFillColor(sf::Color::Cyan);
            window.draw(eb);

            // 能量数字显示
            sf::Text nrgText(font, "EN: " + std::to_string((int)player.energy), 14);
            nrgText.setPosition({ 130.f, 40.f });
            window.draw(nrgText);

            // --- UI: Boss 血条与数值 ---
            sf::RectangleShape bhpBG({ 200.f, 15.f });
            bhpBG.setPosition({ 580.f, 20.f });
            bhpBG.setFillColor(sf::Color(50, 0, 0));
            window.draw(bhpBG);

            sf::RectangleShape bhp({ std::max(0.f, boss.health * 2.f), 15.f });
            bhp.setPosition({ 580.f, 20.f });
            bhp.setFillColor(sf::Color::Red);
            window.draw(bhp);

            sf::Text bText(font, "BOSS HP: " + std::to_string((int)boss.health), 16);
            bText.setPosition({ 580.f, 40.f });
            window.draw(bText);
        }
        else {
            // 结算界面
            sf::Text endText(font, (state == GameState::Win ? "YOU WIN!" : "GAME OVER"), 50);
            endText.setFillColor(state == GameState::Win ? sf::Color::Green : sf::Color::Red);
            endText.setPosition({ 280, 200 });
            window.draw(endText);

            sf::Text retryHint(font, "Press 'R' to Retry\nPress 'M' to Main Menu", 20);
            retryHint.setPosition({ 300, 350 });
            window.draw(retryHint);
        }

        window.display();
    }

    return 0;
}
