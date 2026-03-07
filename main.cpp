#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <deque>
#include <vector>
#include <cmath>
#include <algorithm>

// 1. 基础结构体：记录轨迹和弹幕数据
struct Pose {
    sf::Vector2f position;
    float rotation;
};

struct Bullet {
    sf::CircleShape shape;
    sf::Vector2f velocity;
};

// 2. 蛇类：实现 的移动与状态逻辑
class Snake {
public:
    std::deque<Pose> trail;
    int bodyCount = 3;        // 初始三截本体 
    int gap = 12;             // 身体间距（帧数）
    float normalSpeed = 250.f;
    float slowSpeed = 80.f;   // 按下Space或D时的减速 

    sf::Vector2f headPos{ 400.f, 300.f };
    bool isParsing = false;   // 是否处于解析模式 
    bool isInvincible = false;// 无敌状态 [cite: 8, 9]

    void update(float dt, sf::Vector2u winSize) {
        float currentSpeed = normalSpeed;

        // 检测减速按键 
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
            currentSpeed = slowSpeed;
            isParsing = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
        }
        else {
            isParsing = false;
        }

        // 基础四方向移动 
        sf::Vector2f dir(0.f, 0.f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))    dir.y -= 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))  dir.y += 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))  dir.x -= 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) dir.x += 1.f;

        // 归一化速度方向，防止斜向移动过快
        if (dir.x != 0 || dir.y != 0) {
            float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            headPos += (dir / length) * currentSpeed * dt;
        }

        // 边界处理 
        headPos.x = std::clamp(headPos.x, 20.f, (float)winSize.x - 20.f);
        headPos.y = std::clamp(headPos.y, 20.f, (float)winSize.y - 20.f);

        // 记录轨迹
        trail.push_front({ headPos, 0.f });
        if (trail.size() > (bodyCount + 1) * gap) {
            trail.pop_back();
        }
    }

    void draw(sf::RenderWindow& window) {
        // 绘制身体 (只在非停止状态显示多节，或根据需求改变形态) 
        for (int i = 1; i <= bodyCount; ++i) {
            size_t idx = i * gap;
            if (idx < trail.size()) {
                sf::RectangleShape bodyPart(sf::Vector2f(20.f, 20.f));
                bodyPart.setOrigin({ 10.f, 10.f });
                bodyPart.setPosition(trail[idx].position);
                bodyPart.setFillColor(sf::Color(150, 150, 150));
                window.draw(bodyPart);
            }
        }
        // 绘制头部
        sf::RectangleShape head(sf::Vector2f(24.f, 24.f));
        head.setOrigin({ 12.f, 12.f });
        head.setPosition(headPos);
        head.setFillColor(isParsing ? sf::Color::Yellow : sf::Color::White);
        window.draw(head);
    }
};

// 3. 简单的弹幕发射器实现 
class Boss {
public:
    float rotationAngle = 0.f;
    float fireTimer = 0.f;
    std::vector<Bullet> bullets;

    void update(float dt, sf::Vector2f center) {
        rotationAngle += 90.f * dt; // 发射器旋转速度
        fireTimer += dt;

        if (fireTimer > 0.15f) { // 发射频率
            fireTimer = 0.f;
            // 创建两个对称的旋转发射口 [cite: 10]
            for (int i : {0, 180}) {
                float angle = (rotationAngle + i) * 3.14159f / 180.f;
                Bullet b;
                b.shape.setRadius(5.f);
                b.shape.setFillColor(sf::Color::Red);
                b.shape.setPosition(center + sf::Vector2f(std::cos(angle), std::sin(angle)) * 50.f);
                b.velocity = sf::Vector2f(std::cos(angle), std::sin(angle)) * 300.f;
                bullets.push_back(b);
            }
        }

        // 更新所有子弹位置
        for (auto it = bullets.begin(); it != bullets.end(); ) {
            it->shape.move(it->velocity * dt);
            // 简单出界检测
            sf::Vector2f p = it->shape.getPosition();
            if (p.x < -50 || p.x > 900 || p.y < -50 || p.y > 700) {
                it = bullets.erase(it);
            }
            else {
                ++it;
            }
        }
    }
};

// 4. 程序入口
int main() {
    sf::RenderWindow window(sf::VideoMode({ 800, 600 }), "Last Command Clone - HW");
    window.setFramerateLimit(60);

    Snake player;
    Boss boss;
    sf::Clock clock;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
        }

        float dt = clock.restart().asSeconds();

        // 更新逻辑
        player.update(dt, window.getSize());
        boss.update(dt, sf::Vector2f(400.f, 300.f)); // 敌人处于中心 [cite: 11]

        // 渲染
        window.clear(sf::Color(10, 10, 10)); // 深色背景

        player.draw(window);
        for (auto& b : boss.bullets) window.draw(b.shape);

        window.display();
    }
    return 0;
}