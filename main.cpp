#include <SFML/Graphics.hpp> // SFML 的核心图形库（窗口、形状、文本等）
#include <SFML/Window.hpp>   // SFML 的窗口与输入处理库（键盘操作、事件）
#include <deque>             // C++ 标准库：双端队列，非常适合需要频繁在头尾增删数据的贪吃蛇轨迹
#include <vector>            // C++ 标准库：动态数组，用于存储弹幕、数据点等数量变化的对象
#include <cmath>             // C++ 数学库：提供 sin, cos, hypot(求斜边/距离) 等数学计算
#include <algorithm>         // C++ 算法库：提供 std::max, std::min, std::clamp, std::remove_if 等
#include <ctime>             // 用于初始化随机数种子
#include <iostream>          // 用于在控制台输出警告或错误信息
#include <string>            // 字符串处理

// ==========================================
// 1. 基础结构体与全局枚举 (定义游戏世界的基础元素)
// ==========================================

// 记录轨迹点的数据结构。蛇头走过的地方都会生成一个 Pose 存入队列。
struct Pose { sf::Vector2f position; };

// 弹幕结构体：包含一个圆形形状以及它的飞行速度（X和Y方向的分量）
struct Bullet { sf::CircleShape shape; sf::Vector2f velocity; };

// 随机生成的“数据点”（吃了能变长） [cite: 8]
struct DataPoint {
    sf::CircleShape shape;
    // 构造函数：创建时指定位置
    DataPoint(sf::Vector2f pos) {
        shape.setRadius(7.f);                   // 半径 7 像素
        shape.setFillColor(sf::Color::Cyan);    // 青色
        shape.setOrigin({ 7.f, 7.f });          // 将原点移到圆心，方便后续计算距离
        shape.setPosition(pos);                 // 设置在屏幕上的绝对位置
    }
};

// 冲击波结构体：用于消除屏幕上的弹幕 [cite: 8]
struct Shockwave {
    sf::CircleShape shape;
    float radius = 0.f;          // 当前半径
    float maxRadius = 180.f;     // 冲击波能扩散的最大半径
    bool isAlive = true;         // 生命周期标志，如果为 false 则应该被销毁

    Shockwave(sf::Vector2f pos) {
        shape.setPosition(pos);
        shape.setFillColor(sf::Color::Transparent); // 内部透明
        shape.setOutlineColor(sf::Color::White);    // 白色边缘
        shape.setOutlineThickness(3.f);             // 边缘宽度
    }

    // 每帧更新冲击波状态
    void update(float dt) {
        radius += 500.f * dt;                       // 半径以每秒 500 像素扩大
        shape.setRadius(radius);
        shape.setOrigin({ radius, radius });        // 随着半径变大，中心点也要同步移动，保持居中扩散
        if (radius >= maxRadius) isAlive = false;   // 超过最大范围，标记死亡
    }
};

// 游戏状态枚举：控制游戏处于哪个界面 
enum class GameState { Menu, LevelSelect, DiffSelect, Playing, GameOver, Win };
// 难度枚举：控制敌人血量和射速 [cite: 12]
enum class Difficulty { Easy, Normal, Hard };
// 关卡枚举：控制 Boss 的行为模式
enum class Level { Level1, Level2 };

// 结算统计数据：用于游戏结束时展示玩家的成绩
struct GameStats {
    float timeElapsed = 0.f; // 存活/通关时间 [cite: 12]
    int damageTaken = 0;     // 受伤次数
    int maxLength = 2;       // 达到过的最大长度
} stats;                     // 定义一个全局变量 stats

// 辅助函数：将 SFML 的文字对象的原点设置在中心，方便实现“居中对齐”排版
void centerText(sf::Text& text, float y) {
    sf::FloatRect bounds = text.getLocalBounds();
    // bounds.size.x 和 size.y 是 SFML 3.0 的新标准写法，获取文本的宽和高
    text.setOrigin({ bounds.size.x / 2.0f, bounds.size.y / 2.0f });
    text.setPosition({ 400.f, y }); // 窗口宽 800，400 就是正中间
}

// ==========================================
// 2. 玩家类：蛇 (处理主角的所有逻辑)
// ==========================================
class Snake {
public:
    std::deque<Pose> trail;    // 核心数据结构：记录蛇头走过的历史位置
    int bodyCount = 2;         // 初始有两节附加身体 [cite: 8]
    const int maxBody = 10;    // 最大长度限制
    int gap = 5;               // 身体节与节之间，在队列中相隔几个历史点（决定了身体的紧凑程度）

    // 移动速度定义
    float normalSpeed = 260.f; // 正常速度
    float slowSpeed = 90.f;    // 慢速/解析时的速度 [cite: 8]
    float dashSpeed = 1400.f;  // 冲刺爆发速度 [cite: 9]

    sf::Vector2f headPos{ 400.f, 450.f }; // 蛇头当前绝对坐标
    sf::Vector2f currentDir{ 0.f, -1.f }; // 蛇头当前朝向（默认朝上）

    // 玩家状态标志位
    bool isParsing = false;    // 是否按住 D 处于解析模式 [cite: 8]
    bool isSlowing = false;    // 是否处于慢速状态（按了空格或D，或撞墙） [cite: 8]
    bool isInvincible = false; // 是否无敌（冲刺或受伤后触发） [cite: 9]
    float invTimer = 0.f;      // 无敌剩余时间

    float parseProgress = 0.f; // 解析进度条 (0-100)
    int health = 3;            // 玩家生命值 [cite: 8]

    bool isDashing = false;    // 是否正在冲刺
    float dashTimer = 0.f;     // 冲刺动作剩余时间 [cite: 9]
    int dashCharges = 2;       // 当前可用的冲刺次数 [cite: 9]
    float dashCooldown = 0.f;  // 冲刺冷却恢复计时器

    float energy = 100.f;      // 能量值，用于释放 Q 技能冲击波 [cite: 8]

    sf::RectangleShape headShape; // 蛇头的图形
    sf::RectangleShape bodyShape; // 蛇身的图形

    // 构造函数：初始化图形大小和颜色
    Snake() {
        headShape.setSize({ 20.f, 20.f });
        headShape.setOrigin({ 10.f, 10.f });
        headShape.setFillColor(sf::Color::White);

        bodyShape.setSize({ 12.f, 12.f });
        bodyShape.setOrigin({ 6.f, 6.f });
        bodyShape.setFillColor(sf::Color(180, 180, 180));
    }

    // 重置玩家状态（用于“再来一局”）
    void reset() {
        headPos = { 400.f, 450.f };
        bodyCount = 2;
        health = 3;
        energy = 100.f;
        dashCharges = 2;
        trail.clear();
        isDashing = false;
        isInvincible = false;
        parseProgress = 0.f;
    }

    // 处理玩家受伤逻辑
    void takeDamage() {
        health--;                   // 扣血 [cite: 8]
        stats.damageTaken++;        // 记录受伤次数（统计用）
        isInvincible = true;        // 触发无敌 [cite: 8]
        invTimer = 1.0f;            // 无敌持续 1 秒
        bodyCount = std::max(2, bodyCount - 2); // 死亡惩罚：掉落两节身体
    }

    // 每一帧更新玩家的数据
    void update(float dt, sf::Vector2u winSize, float& bossHealth) {
        // --- 1. 自动回复与倒计时处理 ---
        if (isInvincible) {
            invTimer -= dt;
            if (invTimer <= 0.f) isInvincible = false;
        }

        // 能量随时间自动恢复，上限 100 [cite: 8]
        energy = std::min(100.f, energy + 12.f * dt);

        // 冲刺点数冷却恢复逻辑（每 2.5 秒恢复一点） [cite: 9]
        if (dashCharges < 2) {
            dashCooldown += dt;
            if (dashCooldown >= 2.5f) {
                dashCharges++;
                dashCooldown = 0.f;
            }
        }

        // --- 2. 判定当前移动模式与速度 ---
        float currentMoveSpeed = normalSpeed;

        // 检测是否撞到屏幕边界
        bool atEdge = (headPos.x <= 15.f || headPos.x >= (float)winSize.x - 15.f ||
            headPos.y <= 15.f || headPos.y >= (float)winSize.y - 15.f);

        if (isDashing) {
            currentMoveSpeed = dashSpeed; // 冲刺期间强制使用冲刺速度
            dashTimer -= dt;
            if (dashTimer <= 0.f) isDashing = false; // 冲刺结束
        }
        else {
            // 如果按下 Space、D 或者撞墙，进入减速判定 [cite: 8]
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || atEdge) {
                currentMoveSpeed = slowSpeed;
                isSlowing = true;
                isParsing = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D); // 只有按下 D 才算解析
            }
            else {
                isSlowing = false;
                isParsing = false;
            }
        }

        sf::Vector2f prevPos = headPos; // 记录移动前的位置，用于计算移动距离

        // --- 3. 处理按键输入与位置更新 ---
        if (!isDashing) {
            sf::Vector2f inputDir(0.f, 0.f);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))         inputDir.y = -1.f;
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))  inputDir.y = 1.f;
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))  inputDir.x = -1.f;
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) inputDir.x = 1.f;

            bool hasInput = (inputDir.x != 0.f || inputDir.y != 0.f);
            if (hasInput) {
                currentDir = inputDir; // 记录最后的朝向
            }

            // 核心逻辑：如果没有被减速（正常行驶），或者虽然被减速但按了方向键，才移动。
            // 否则（按了空格且没按方向），停留在原地。 [cite: 8]
            if (!isSlowing || hasInput) {
                headPos += currentDir * currentMoveSpeed * dt;
            }
        }
        else {
            // 冲刺期间无法改变方向，只能沿着被锁定的方向高速移动
            headPos += currentDir * currentMoveSpeed * dt;
        }

        // 使用 std::clamp 将蛇头强制限制在窗口范围内 [cite: 8]
        headPos.x = std::clamp(headPos.x, 12.f, (float)winSize.x - 12.f);
        headPos.y = std::clamp(headPos.y, 12.f, (float)winSize.y - 12.f);

        // --- 4. 轨迹记录 (重难点) ---
        // 获取这一帧实际移动的距离
        float distMoved = std::hypot(headPos.x - prevPos.x, headPos.y - prevPos.y);
        if (distMoved > 0.1f) {
            // 如果速度太快（如冲刺），两帧之间移动距离过大，会导致轨迹出现断层，身体跟不上。
            // 解决方案：插值。计算出这段距离应该被分割成多少个点（这里设定每 3 个像素记录一个点）。
            int steps = static_cast<int>(std::ceil(distMoved / 3.0f));
            for (int i = 1; i <= steps; ++i) {
                // 利用线性插值计算出中间的坐标，塞进历史轨迹队列。保证轨迹极其丝滑。
                sf::Vector2f interpPos = prevPos + (headPos - prevPos) * ((float)i / steps);
                trail.push_front({ interpPos });
            }
        }

        // 控制队列长度：只需要保留足够当前身体长度使用的历史坐标即可，防止内存爆炸
        if (trail.size() > (size_t)(maxBody + 2) * gap) {
            trail.resize((maxBody + 2) * gap);
        }

        // --- 5. 解析攻击逻辑 ---
        if (isParsing && bodyCount >= 3) {
            // 解析速度与身体长度成正比 [cite: 8]
            parseProgress += (30.f + (bodyCount - 3) * 15.f) * dt;
            if (parseProgress >= 100.f) {
                // 进度满 100 时对 Boss 造成伤害，伤害量受当前长度影响
                bossHealth -= (10.f + bodyCount * 8.f);
                parseProgress = 0.f;  // 重置进度
                bodyCount = 2;        // 消耗掉多余的身体作为攻击代价
                trail.clear();        // 清除轨迹重新开始排队
            }
        }
        else {
            // 松开 D 键时，解析进度会迅速衰减
            parseProgress = std::max(0.f, parseProgress - 50.f * dt);
        }
    }

    // 将玩家绘制到窗口上
    void draw(sf::RenderWindow& window) {
        // 冲刺残影特效：提取轨迹中的历史点，绘制半透明的自己 [cite: 9]
        if (isDashing) {
            // 往回找最多 15 个点，每隔 3 个点画一个
            for (size_t i = 0; i < std::min((size_t)15, trail.size()); i += 3) {
                sf::RectangleShape ghost = headShape;
                ghost.setPosition(trail[i].position);
                // 颜色设为青色，透明度随距离变淡 (Alpha 值: 120 - i * 6)
                ghost.setFillColor(sf::Color(0, 255, 255, 120 - i * 6));
                window.draw(ghost);
            }
        }

        // 绘制身体：只有不在“停止/变形”状态下才绘制长长的身体 [cite: 8]
        if (!isSlowing) {
            for (int i = 1; i <= bodyCount; ++i) {
                size_t idx = i * gap; // 根据间距获取在轨迹队列中的索引
                if (idx < trail.size()) {
                    bodyShape.setPosition(trail[idx].position);
                    window.draw(bodyShape);
                }
            }
        }

        // 绘制头部
        headShape.setPosition(headPos);
        // 如果处于无敌状态，通过透明与不透明的高频切换实现“闪烁”效果
        if (isInvincible && static_cast<int>(invTimer * 15) % 2 == 0) {
            headShape.setFillColor(sf::Color::Transparent);
        }
        else {
            // 解析时头变黄，否则是白色
            headShape.setFillColor(isParsing ? sf::Color::Yellow : sf::Color::White);
        }
        window.draw(headShape);

        // 绘制头顶的解析进度条 [cite: 8]
        if (isParsing && bodyCount >= 3) {
            // 根据百分比动态计算进度条的宽度 (总宽 40 像素)
            sf::RectangleShape bar({ 40.f * (parseProgress / 100.f), 4.f });
            bar.setPosition({ headPos.x - 20.f, headPos.y - 25.f });
            bar.setFillColor(sf::Color::Yellow);
            window.draw(bar);
        }
    }
};

// ==========================================
// 3. 敌人类：Boss (包含多阶段攻击模式)
// ==========================================
class Boss {
public:
    float rotationAngle = 0.f;  // 发射器的基础旋转角度 [cite: 10]
    float fireTimer = 0.f;      // 发射子弹计时器
    float patternTimer = 0.f;   // 模式切换计时器
    int currentPattern = 0;     // 当前所处的攻击模式阶段

    std::vector<Bullet> bullets;// 屏幕上所有的 Boss 子弹
    sf::Vector2f pos{ 400.f, 200.f }; // Boss 的坐标
    float health = 100.f;
    float maxHealth = 100.f;

    // 初始化 Boss 状态
    void init(Difficulty diff) {
        bullets.clear();
        rotationAngle = 0.f;
        patternTimer = 0.f;
        currentPattern = 0;
        pos = { 400.f, 200.f }; // 居中偏上 [cite: 11]

        // 根据选择的难度设定血量 [cite: 12]
        if (diff == Difficulty::Easy) maxHealth = 150.f;
        else if (diff == Difficulty::Normal) maxHealth = 300.f;
        else maxHealth = 500.f;
        health = maxHealth;
    }

    // 每帧更新 Boss 行为
    void update(float dt, Difficulty diff, Level lvl) {
        patternTimer += dt;
        // 每 7 秒强制切换到下一种攻击模式（循环 0,1,2）
        if (patternTimer > 7.0f) {
            currentPattern = (currentPattern + 1) % 3;
            patternTimer = 0.f;
        }

        // 根据难度设定基础射速（时间间隔越短，射速越快） [cite: 12]
        float baseFireRate = (diff == Difficulty::Hard) ? 0.08f : ((diff == Difficulty::Normal) ? 0.12f : 0.18f);
        fireTimer += dt;

        // 如果达到了开火时间
        if (fireTimer > baseFireRate) {
            fireTimer = 0.f;
            if (currentPattern == 0) {
                // 模式 0：十字旋转风车弹幕
                rotationAngle += 18.f;
                for (int i : {0, 90, 180, 270}) {
                    fireBullet(rotationAngle + i, 220.f);
                }
            }
            else if (currentPattern == 1) {
                // 模式 1：双螺旋狂暴散弹
                rotationAngle += 35.f;
                fireBullet(rotationAngle, 280.f);
                fireBullet(rotationAngle + 180.f, 280.f);
            }
            else if (currentPattern == 2) {
                // 模式 2：周期性的全方位爆破扩散弹
                // 这里用取模运算制造“一阵一阵”的节奏感
                if (static_cast<int>(patternTimer * 10) % 8 == 0) {
                    for (int i = 0; i < 360; i += 30) {
                        fireBullet(i + rotationAngle, 180.f);
                    }
                }
                rotationAngle += 5.f;
            }
        }

        // 第二关特有逻辑：Boss 沿 X 轴做正弦波运动
        if (lvl == Level::Level2) {
            pos.x = 400.f + std::sin(patternTimer * 1.5f) * 200.f;
        }

        // 更新所有屏幕上子弹的位置
        for (auto it = bullets.begin(); it != bullets.end(); ) {
            it->shape.move(it->velocity * dt);
            sf::Vector2f p = it->shape.getPosition();
            // 如果子弹彻底飞出了屏幕外，将其从数组中删除以节约内存
            if (p.x < -30 || p.x > 830 || p.y < -30 || p.y > 630) it = bullets.erase(it);
            else ++it;
        }
    }

    // 根据指定的角度和速度生成一颗子弹
    void fireBullet(float angleDeg, float speed) {
        // 将角度转为弧度制，供 sin/cos 函数计算
        float angleRad = angleDeg * 3.14159f / 180.f;
        Bullet b;
        b.shape.setRadius(5.f);
        b.shape.setFillColor(sf::Color(255, 50, 50));
        b.shape.setOrigin({ 5.f, 5.f });
        b.shape.setPosition(pos);
        // 三角函数分解速度向量：x=cos(a)*V, y=sin(a)*V
        b.velocity = { std::cos(angleRad) * speed, std::sin(angleRad) * speed };
        bullets.push_back(b); // 加入弹幕池
    }

    void draw(sf::RenderWindow& window) {
        // 画 Boss 本体
        sf::CircleShape core(35.f);
        core.setOrigin({ 35.f, 35.f });
        core.setPosition(pos);

        // 核心颜色随当前攻击模式动态变化
        if (currentPattern == 0) core.setFillColor(sf::Color(150, 0, 0));
        else if (currentPattern == 1) core.setFillColor(sf::Color(200, 50, 0));
        else core.setFillColor(sf::Color(100, 0, 150));

        window.draw(core);
        // 画出他发射的所有子弹
        for (auto& b : bullets) window.draw(b.shape);
    }
};

// ==========================================
// 4. 游戏主循环与底层框架
// ==========================================
int main() {
    // 设置随机种子，保证每次运行生成的随机数不同
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    // 创建 800x600 的抗锯齿渲染窗口
    sf::RenderWindow window(sf::VideoMode({ 800, 600 }), "Last Command - Extended Edition");
    window.setFramerateLimit(60); // 锁帧 60 帧，防止显卡过热狂飙

    // 加载字体。如果没有同级目录的 arial.ttf 文件会报错
    sf::Font font;
    if (!font.openFromFile("arial.ttf")) {
        std::cout << "Warning: Arial.ttf not found. Text will not render correctly.\n";
    }

    // 实例化游戏核心对象
    Snake player;
    Boss boss;
    std::vector<DataPoint> dataPoints;
    std::vector<Shockwave> shockwaves;

    float spawnTimer = 0.f; // 控制数据点生成的计时器
    sf::Clock clock;        // 核心时钟，用于计算帧差 dt

    // 初始化状态机
    GameState state = GameState::Menu;
    Difficulty currentDiff = Difficulty::Normal;
    Level currentLevel = Level::Level1;

    int menuSelection = 0; // 记录菜单中当前高亮的选项索引

    // 只要窗口没关，游戏主循环就一直跑
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds(); // 获取上一帧到现在的真实时间差

        // --- 事件处理队列 (负责捕获键盘按压、鼠标点击等操作系统级事件) ---
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close(); // 点叉关闭

            // 只有当有键盘“按下”时才触发
            if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {

                // 位于主菜单界面
                if (state == GameState::Menu) {
                    if (keyEvent->code == sf::Keyboard::Key::Up) menuSelection = 0;
                    if (keyEvent->code == sf::Keyboard::Key::Down) menuSelection = 1;
                    if (keyEvent->code == sf::Keyboard::Key::Enter) {
                        if (menuSelection == 0) state = GameState::LevelSelect;
                        else window.close();
                        menuSelection = 0; // 重置选择位
                    }
                }
                // 位于选关界面
                else if (state == GameState::LevelSelect) {
                    if (keyEvent->code == sf::Keyboard::Key::Up) menuSelection = 0;
                    if (keyEvent->code == sf::Keyboard::Key::Down) menuSelection = 1;
                    if (keyEvent->code == sf::Keyboard::Key::Enter) {
                        currentLevel = (menuSelection == 0) ? Level::Level1 : Level::Level2;
                        state = GameState::DiffSelect;
                        menuSelection = 0;
                    }
                }
                // 位于难度选择界面
                else if (state == GameState::DiffSelect) {
                    if (keyEvent->code == sf::Keyboard::Key::Up) menuSelection = std::max(0, menuSelection - 1);
                    if (keyEvent->code == sf::Keyboard::Key::Down) menuSelection = std::min(2, menuSelection + 1);
                    if (keyEvent->code == sf::Keyboard::Key::Enter) {
                        if (menuSelection == 0) currentDiff = Difficulty::Easy;
                        else if (menuSelection == 1) currentDiff = Difficulty::Normal;
                        else currentDiff = Difficulty::Hard;

                        // 正式进入游戏前，重置所有状态和数据
                        player.reset();
                        boss.init(currentDiff);
                        dataPoints.clear();
                        shockwaves.clear();
                        stats = GameStats(); // 清空上次记录
                        state = GameState::Playing;
                    }
                }
                // 正在玩游戏时的按键逻辑
                else if (state == GameState::Playing) {
                    // Q 键释放冲击波 [cite: 8]
                    if (keyEvent->code == sf::Keyboard::Key::Q && player.energy >= 35.f) {
                        player.energy -= 35.f; // 扣除消耗
                        shockwaves.push_back(Shockwave(player.headPos)); // 在原地生成一个冲击波
                    }
                    // 左 Shift 冲刺 [cite: 9]
                    if (keyEvent->code == sf::Keyboard::Key::LShift && player.dashCharges > 0 && !player.isDashing) {
                        player.dashCharges--; // 消耗一次冲刺点
                        player.isDashing = true;
                        player.dashTimer = 0.15f;    // 冲刺动作只持续极短的 0.15 秒
                        player.isInvincible = true;
                        player.invTimer = 0.40f;     // 但无敌期会持续 0.4 秒，提供容错
                    }
                }
                // 游戏结束（死亡/胜利）结算界面 
                else if (state == GameState::GameOver || state == GameState::Win) {
                    if (keyEvent->code == sf::Keyboard::Key::Enter) {
                        state = GameState::Menu; // 回到主菜单
                        menuSelection = 0;
                    }
                }
            }
        }

        // --- 逻辑更新 (只有当处于游玩状态才计算碰撞、移动等) ---
        if (state == GameState::Playing) {
            stats.timeElapsed += dt; // 累加游戏时长

            // 生成食物（数据点）
            if (dataPoints.empty()) {
                spawnTimer += dt;
                if (spawnTimer > 1.2f) { // 每隔 1.2 秒生成一个
                    sf::Vector2f spawnPos;
                    // do-while 循环确保生成的点不会和 Boss 的中心靠得太近
                    do {
                        spawnPos = { (float)(rand() % 700 + 50), (float)(rand() % 400 + 100) };
                    } while (std::hypot(spawnPos.x - boss.pos.x, spawnPos.y - boss.pos.y) < 120.f);

                    dataPoints.push_back(DataPoint(spawnPos));
                    spawnTimer = 0.f;
                }
            }

            // 更新两大实体
            player.update(dt, window.getSize(), boss.health);
            boss.update(dt, currentDiff, currentLevel);

            // 记录成就：历史最大长度
            stats.maxLength = std::max(stats.maxLength, player.bodyCount);

            // 更新场上所有的冲击波状态
            for (auto it = shockwaves.begin(); it != shockwaves.end(); ) {
                it->update(dt);
                if (!it->isAlive) it = shockwaves.erase(it);
                else ++it;
            }

            // 【核心算法】碰撞检测 1：玩家是否吃到了数据点 [cite: 8]
            // std::hypot(x,y) 用于求两点间的直线距离，性能极好。公式相当于 sqrt(x^2 + y^2)
            for (auto it = dataPoints.begin(); it != dataPoints.end(); ) {
                if (std::hypot(player.headPos.x - it->shape.getPosition().x, player.headPos.y - it->shape.getPosition().y) < 20.f) {
                    if (player.bodyCount < player.maxBody) player.bodyCount++; // 体长增加
                    it = dataPoints.erase(it); // 摧毁已被吃掉的数据点
                }
                else ++it;
            }

            // 【核心算法】碰撞检测 2：冲击波是否扫到了子弹 [cite: 8]
            for (auto& sw : shockwaves) {
                // 使用 std::remove_if 和 Lambda 表达式，一行代码完成条件筛选和删除
                boss.bullets.erase(std::remove_if(boss.bullets.begin(), boss.bullets.end(), [&](const Bullet& b) {
                    // 如果子弹和冲击波中心的距离 < 冲击波的当前半径，就属于被扫到了，返回 true 删除它
                    return std::hypot(sw.shape.getPosition().x - b.shape.getPosition().x, sw.shape.getPosition().y - b.shape.getPosition().y) < sw.radius;
                    }), boss.bullets.end());
            }

            // 【核心算法】碰撞检测 3：玩家是否撞到了子弹 [cite: 8]
            if (!player.isInvincible) {
                for (auto it = boss.bullets.begin(); it != boss.bullets.end(); ) {
                    // 如果距离小于 12 像素，判定为击中蛇头
                    if (std::hypot(player.headPos.x - it->shape.getPosition().x, player.headPos.y - it->shape.getPosition().y) < 12.f) {
                        player.takeDamage(); // 扣血
                        shockwaves.push_back(Shockwave(player.headPos)); // 被动触发一个保命冲击波消除周围弹幕
                        it = boss.bullets.erase(it); // 子弹被身体消耗掉
                        break; // 每次只处理一颗子弹的伤害，避免瞬间被秒杀
                    }
                    else ++it;
                }
            }

            // 胜负判定条件 [cite: 8, 11]
            if (player.health <= 0) state = GameState::GameOver;
            if (boss.health <= 0) state = GameState::Win;
        }

        // --- 渲染部分 (将算好的所有数据画到屏幕上) ---
        window.clear(sf::Color(15, 15, 20)); // 每帧开头先用深蓝色刷掉旧画面

        if (state == GameState::Playing) {
            // 注意绘制的先后顺序，先画的会在底层
            boss.draw(window);
            for (auto& dp : dataPoints) window.draw(dp.shape);
            for (auto& sw : shockwaves) window.draw(sw.shape);
            player.draw(window); // 玩家画在最后，确保总是显示在最顶层

            // UI: 画玩家血量（左上角一排绿方块） [cite: 8]
            for (int i = 0; i < player.health; ++i) {
                sf::RectangleShape hb({ 16.f, 16.f });
                hb.setPosition({ 20.f + i * 22.f, 20.f }); // 根据索引 i 算出不同的 x 坐标
                hb.setFillColor(sf::Color::Green);
                window.draw(hb);
            }

            // UI: 画冲刺点数（血条下方的小黄圆） [cite: 9]
            for (int i = 0; i < player.dashCharges; ++i) {
                sf::CircleShape dashDot(6.f);
                dashDot.setPosition({ 20.f + i * 16.f, 45.f });
                dashDot.setFillColor(sf::Color::Yellow);
                window.draw(dashDot);
            }

            // UI: 画能量条（基于 100.f 的百分比条） [cite: 8]
            sf::RectangleShape eb({ 100.f * (player.energy / 100.f), 6.f });
            eb.setPosition({ 20.f, 65.f });
            eb.setFillColor(sf::Color::Cyan);
            window.draw(eb);

            // UI: 画 Boss 血条 (右上角背景红底) [cite: 11]
            sf::RectangleShape bhpBG({ 300.f, 12.f });
            bhpBG.setPosition({ 480.f, 20.f });
            bhpBG.setFillColor(sf::Color(50, 0, 0));
            window.draw(bhpBG);

            // 画实际血量百分比
            sf::RectangleShape bhp({ 300.f * std::max(0.f, boss.health / boss.maxHealth), 12.f });
            bhp.setPosition({ 480.f, 20.f });
            bhp.setFillColor(sf::Color::Red);
            window.draw(bhp);
        }
        else {
            // UI: 非游玩界面（各种菜单）的统一文本渲染流程
            sf::Text title(font, "", 40);
            sf::Text opt1(font, "", 24);
            sf::Text opt2(font, "", 24);
            sf::Text opt3(font, "", 24);

            if (state == GameState::Menu) {
                title.setString("LAST COMMAND - CLONE");
                opt1.setString("Start Game");
                opt2.setString("Exit");
                // 根据 currentSelection 动态修改颜色实现“选中高亮”效果
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
                // 胜利或失败结算数据打印 
                title.setString(state == GameState::Win ? "MISSION ACCOMPLISHED" : "SYSTEM FAILURE");
                title.setFillColor(state == GameState::Win ? sf::Color::Green : sf::Color::Red);

                // std::to_string 将刚才在游戏里记录的数字转换成文字
                opt1.setString("Time: " + std::to_string((int)stats.timeElapsed) + "s");
                opt2.setString("Max Length: " + std::to_string(stats.maxLength));
                opt3.setString("Damage Taken: " + std::to_string(stats.damageTaken));

                sf::Text hint(font, "Press ENTER to return", 18);
                centerText(hint, 450.f);
                window.draw(hint);
            }

            // 经过前面配置后，统一按居中坐标绘制到屏幕上
            centerText(title, 150.f); window.draw(title);
            centerText(opt1, 280.f);  window.draw(opt1);
            centerText(opt2, 330.f);  window.draw(opt2);
            if (opt3.getString() != "") { centerText(opt3, 380.f); window.draw(opt3); }
        }

        window.display(); // 最后一步：将后台画好的全部图形一次性推送到屏幕显示器上
    }

    return 0; // 程序完美结束
}
