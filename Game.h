#pragma once
#include "Global.h"
#include "Snake.h"
#include "Boss.h"

// ==========================================
// 【数据碎片实体】(拾取物)
// ==========================================
struct DataPoint {
    sf::CircleShape shape; // 内部实体(绿色为回血，青色为普通数据，橙色为断尾掉落)
    sf::CircleShape glow;  // 外部光晕，自带呼吸灯效果
    int type;              // 0: 普通数据(加长度), 1: 回血包, 2: 断尾/受击掉落的临时数据
    float timer;           // 存活计时器
    float maxLifetime;     // 最大存活时间 (目前设定为 10 秒)
    bool isFading;         // 是否具有“随时间消散”的属性 (通常只有掉落的橙色数据会消失)

    DataPoint(sf::Vector2f pos, int t = 0) : type(t), timer(0.f), maxLifetime(10.f) {
        shape.setRadius(7.f);
        // 根据类型赋予不同颜色
        shape.setFillColor(t == 1 ? sf::Color::Green : (t == 2 ? sf::Color(255, 165, 0) : sf::Color::Cyan));
        shape.setOrigin({ 7.f, 7.f });
        shape.setPosition(pos);

        glow.setRadius(12.f);
        glow.setFillColor(sf::Color::Transparent); // 内部透明
        glow.setOutlineColor(shape.getFillColor());// 边框颜色与本体一致
        glow.setOutlineThickness(2.f);
        glow.setOrigin({ 12.f, 12.f });
        glow.setPosition(pos);

        isFading = (t == 2); // 只有玩家战损掉落的数据才会随时间消失
    }

    // 【核心机制：数据点视觉更新】
    void update(float dt) {
        timer += dt;
        sf::Color c = shape.getFillColor();
        sf::Color gc = glow.getOutlineColor();
        float alpha = 255.f;

        // 如果是临时数据，随着时间推移，透明度线性衰减至 0
        if (isFading) {
            alpha = std::max(0.f, 255.f * (1.f - timer / maxLifetime));
            c.a = static_cast<std::uint8_t>(alpha);
            shape.setFillColor(c);
        }
        // 外圈光晕：附加一个绝对值 Sine 波的透明度振荡，实现“呼吸闪烁”效果
        gc.a = static_cast<std::uint8_t>(alpha * (0.3f + 0.7f * std::abs(std::sin(timer * 6.f))));
        glow.setOutlineColor(gc);
    }
};

// ==========================================
// 【冲击波实体】(Q技能大招)
// ==========================================
struct Shockwave {
    sf::CircleShape shape;
    float radius = 0.f;
    float maxRadius = 180.f; // 冲击波最大扩散半径
    bool isAlive = true;     // 是否存活

    Shockwave(sf::Vector2f pos) {
        shape.setPosition(pos);
        shape.setFillColor(sf::Color::Transparent);
        shape.setOutlineColor(sf::Color::White); // 白色激波圈
        shape.setOutlineThickness(3.f);
    }

    void update(float dt) {
        // 以极快的速度 (500px/s) 向外扩散
        radius += 500.f * dt;
        shape.setRadius(radius);
        shape.setOrigin({ radius, radius }); // 保持圆心始终在中心
        if (radius >= maxRadius) isAlive = false; // 达到最大范围后消散
    }
};

// ==========================================
// 【玩家追踪子弹】(解析模式产物)
// ==========================================
struct SnakeProjectile {
    sf::Sprite sprite;
    sf::Vector2f pos;
    sf::Vector2f vel;
    float damage;
    int targetBossId; // 发射时死锁的 Boss 目标 (防止空中乱变向)

    SnakeProjectile(const sf::Texture& tex, sf::Vector2f p, float dmg, int targetId = 1) : sprite(tex) {
        sprite.setOrigin({ tex.getSize().x / 2.f, tex.getSize().y / 2.f });
        pos = p;
        damage = dmg;
        vel = { 0.f, -400.f }; // 初始给予一个向上的速度 (模拟发射的后坐力抛射感)
        targetBossId = targetId;
    }

    // 巡航制导逻辑 (Homing Missile)
    void update(float dt, sf::Vector2f target) {
        sf::Vector2f desired = target - pos;
        float dist = std::hypot(desired.x, desired.y);
        if (dist > 0.1f) {
            // 目标速度设为 700
            desired = (desired / dist) * 700.f;
            // 计算转向力 (Steering force)，平滑改变子弹的飞行轨道
            sf::Vector2f steer = desired - vel;
            vel += steer * 4.0f * dt; // 转向灵敏度为 4.0
        }
        pos += vel * dt;
        sprite.setPosition(pos);
        // 根据当前速度向量，更新贴图旋转角度，让子弹头永远朝向飞行方向
        sprite.setRotation(sf::degrees(std::atan2(vel.y, vel.x) * 180.f / 3.14159f));
    }
};

// ==========================================
// 【游戏主控类 (The Game Engine)】
// ==========================================
class Game {
private:
    sf::RenderWindow window;
    sf::Font font;
    GameState state;
    GameStats stats;
    GameFeelManager feelManager;
    SoundManager soundManager;

    Difficulty currentDiff;
    Level currentLevel;

    // UI 菜单选择游标
    int menuSelection;
    int settingsSelection = 0;
    int audioSelection = 0;

    // UI 音量设置游标 (0~10)
    int sfxVolInt = 10;
    int bgmVolInt = 10;
    int unlockedLevels;
    int bgStyle;
    int bgmStyle = 3;

    // 【国际化(i18n)引擎变量】
    Language currentLang = Language::EN;
    std::unordered_map<std::string, std::pair<std::wstring, std::wstring>> i18n;

    // 全局时间缩放因子 (用于 F2 子弹时间)
    float timeScale = 1.0f;
    // 玩家违规操作(如撞墙)时的红/蓝闪屏倒计时
    float borderFlashTimer = 0.f;

    // 所有纹理资源句柄
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

    // 核心实体
    Snake player;
    Boss boss;
    Boss boss2;

    // 场上存在的动态实体容器
    std::vector<DataPoint> dataPoints;
    std::vector<Shockwave> shockwaves;
    std::vector<SnakeProjectile> snakeProjectiles;

    // 控制随机数据与血包刷新的计时器
    float spawnTimer;
    float healSpawnTimer;
    sf::Clock clock;
    bool lastParsingState = false;

    // UI 界面的 Boss 预览图切片播放器
    float honestPreviewTimer = 0.f;
    int honestPreviewFrame = 0;
    float himePreviewTimer = 0.f;
    int himePreviewFrame = 0;

    // 第三关双 Boss 换位演出(Phase 0)相关变量
    float bossSwapTimer = 0.f;
    bool isSwapping = false;
    float swapProgress = 0.f;
    sf::Vector2f himeStartPos, honestStartPos;
    sf::Vector2f himeTargetPos, honestTargetPos;

    // 内部流程函数
    void processEvents();
    void update(float dt);
    void render();
    void centerText(sf::Text& text, float y); // 工具函数：让文本在屏幕 X 轴居中
    void loadResources();
    void startLevel();
    void restartPhase();
    void getBarString(int vol, std::wstring& outStr); // 工具函数：生成音量进度条 [|||||     ]
    std::wstring T(const std::string& key); // 国际化字典取词函数

public:
    Game();
    void run(); // 对外暴露的唯一主循环入口
};