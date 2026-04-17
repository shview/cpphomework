#pragma once
#include "Global.h"
#include <vector>
#include <optional>

// ==========================================
// 【Boss 状态机枚举】
// ==========================================
// 决定了 Boss 当前的行为模式与播放的动画序列
enum class BossState {
    Spawning,        // 刚出场的生成/召唤阶段 (无敌，通常播放虚影到实体的过度)
    PhaseTransition, // 阶段转换的过度动画 (无敌)
    Normal,          // 正常游走与攻击状态
    Hit,             // 受到伤害的受击硬直状态 (闪烁红光)
    Dying,           // 最后一管血被打空的死亡动画阶段
    Dead,            // 彻底死亡，尸体消失
    PhaseWait        // 当前阶段血条打空，等待外部协调(多Boss时)进入下一阶段
};

// ==========================================
// 【普通弹幕结构体】
// ==========================================
struct Bullet {
    sf::Sprite sprite;     // 弹幕的精灵图
    sf::Vector2f velocity; // 每秒移动的X/Y像素速度
    int type;              // 子弹类型 (0,1为普攻，10,11为Boss特攻，12为泡泡)

    Bullet(const sf::Texture& tex, sf::Vector2f vel, int t = 0) : sprite(tex), velocity(vel), type(t) {}
};

// ==========================================
// 【红线预警区域结构体】
// ==========================================
// 用于在发射高危大体积弹幕前，给玩家提供 1.5s 或 0.8s 的视觉反应时间
struct WarningArea {
    sf::RectangleShape shape; // 预警的矩形(通常极长，代表激光轨迹)
    float timer;              // 当前已存在的预警时间
    float maxTime;            // 预警多久后正式发射子弹
    int bulletType;           // 预警结束后，将生成什么类型的子弹
    sf::Vector2f startPos;    // 预警线的起始物理坐标
    sf::Vector2f velocity;    // 转化成子弹后的初速度
    float rotation;           // 射击的绝对角度
    bool attachedToBoss = false; // 是否“粘”在Boss身上 (Boss移动，预警线跟着移动)
    bool beepedHalfway = false;  // 音频控制：预警时间过半时是否已经播放过急促的“滴”声
};

// ==========================================
// 【Boss 配置表 (模板)】
// ==========================================
// 数据驱动结构。通过填入不同的配置，可以复用同一个 Boss 类，
// 生成外观、体积、技能完全不同的实体 (如 Boss 1: Honest, Boss 2: Hime)
struct BossConfig {
    int bossId; // 身份标识：1为Honest(蓝色，垂直特攻), 2为Hime(红色，横向扫射)

    // 各类动作的精灵图集指针
    const sf::Texture* stayTex;    // 待机图集
    const sf::Texture* castTex;    // 施法/攻击图集
    const sf::Texture* sufferTex;  // 受击图集
    const sf::Texture* endTex;     // 死亡图集
    const sf::Texture* bgObjectTex;// Boss背后的特效悬浮物 (Hime专属)

    // Boss发射的不同类型子弹的贴图
    const sf::Texture* texHonestSpecial; // Honest 的长条水晶
    const sf::Texture* texHimeSpecial;   // Hime 的巨型扇子
    const sf::Texture* texBubble;        // 全屏穿梭的急速泡泡

    // 各类动作的帧动画播放参数 (行数、列数、总帧数、帧率)
    AnimInfo animStay;
    AnimInfo animCast;
    AnimInfo animSuffer;
    AnimInfo animEnd;

    // 初始登场的逻辑中心点
    float startPosX = 400.f;
    float startPosY = 200.f;

    // 是否强制Boss只发射某一种外观的普攻 (-1为根据阶段切换，0或1为固定)
    int forceBulletType = -1;

    // Boss 本体的受击判定肉体大小 (Honest 较瘦为 35，Hime 带扇子较宽为 45)
    float bossHitboxRadius = 25.f;
};

class Boss {
private:
    sf::Vector2f pos;     // 当前物理坐标
    float basePosX;       // 游走动画的基准X轴 (例如双Boss互相绕圈时的原点)
    float basePosY;       // 游走动画的基准Y轴
    float health;         // 当前血量
    float maxHealth;      // 当前阶段的满血量
    float hitboxRadius;   // 当前应用的受击判定半径
    int currentPhase;     // 当前所处阶段 (0, 1, 2)

    // 缓存的常用贴图指针
    const sf::Texture* bulletTexture01;
    const sf::Texture* bulletTexture02;
    const sf::Texture* heartOutlineTex;
    const sf::Texture* heartFillTex;

    // 渲染层实体 (使用了 optional 以便在不需要时保持空状态，节省开销)
    std::optional<sf::Sprite> bgObjectSprite;     // 背景悬浮物
    std::optional<sf::Sprite> bossSprite;         // Boss 本体切片
    std::optional<sf::Sprite> heartOutlineSprite; // 血条外框 (心形)
    std::optional<sf::Sprite> heartFillSprite;    // 血条填充 (根据血量裁剪)
    std::optional<sf::Text> pctText;              // 悬浮显示的数字血量

    BossConfig config;      // 保存自己出生时被赋予的配置
    AnimInfo currentAnim;   // 当前正在播放的动画序列信息
    int currentFrame;       // 动画播到了第几帧
    float animTimer;        // 帧切换计时器

    // 各类攻击的独立冷却计时器
    float rotationAngle;       // 普攻发射的旋转基准角
    float fireTimer;           // 普攻冷却
    float patternTimer;        // 走位波浪(Sine波)计时
    float specialTimer;        // 直线/横向阵列特攻冷却
    float bubbleTimer;         // 泡泡乱窜特攻冷却
    float targetAttackTimer;   // 瞄准玩家的狙击特攻冷却

    // Honest 专属的“延迟三连发狙击”控制变量
    int targetShotsFired = 0;  // 这一轮已经开了几枪
    float targetShotDelay = 0.f; // 两枪之间的短冷却

    BossState state;           // 当前所处状态
    float stateTimer;          // 状态倒计时 (用于硬直恢复、死亡动画播放完毕等)

    // 场上属于该 Boss 的弹幕与预警线容器
    std::vector<Bullet> bullets;
    std::vector<WarningArea> warnings;

    SoundManager* soundManager = nullptr; // 绑定的全局音频管理器指针

    // 内部私有功能函数
    void updateBullets(float dt);
    void setAnimation(const AnimInfo& info);
    void updateSpriteRect();
    void spawnWarning(sf::Vector2f startPos, sf::Vector2f size, sf::Vector2f vel, float rot, float maxTime, int type, bool attached = false);

public:
    Boss();

    // 实例化 Boss (注入配置、字体、贴图、难度等所有上下文)
    void init(Difficulty diff, Level lvl, const sf::Font& font,
        const sf::Texture* heartOut, const sf::Texture* heartFill,
        const sf::Texture* bulletTex01, const sf::Texture* bulletTex02,
        const BossConfig& cfg, SoundManager* sm = nullptr);

    // 核心心跳函数
    void update(float dt, Difficulty diff, Level lvl, sf::Vector2f playerPos, sf::Vector2f playerDir, sf::Vector2f playerBodyPos);
    // 生成普通子弹
    void fireBullet(float angleDeg, float speed, int bulletType);
    // 将自身所有组件渲染到屏幕
    void draw(sf::RenderWindow& window);

    // 对外暴露的状态交互接口
    BossState getState() const { return state; }
    void advancePhase();           // 推进到下一阶段
    void resetToCurrentPhase();    // 阶段重置复活时调用
    void clearDanmaku() { bullets.clear(); warnings.clear(); } // F5 开发者指令：清空弹幕

    // 属性读取与物理位置操作接口
    float getHealth() const { return health; }
    float getMaxHealth() const { return maxHealth; }
    sf::Vector2f getPosition() const { return pos; }

    float getBasePosX() const { return basePosX; }
    float getBasePosY() const { return basePosY; }
    void setBasePosX(float x) { basePosX = x; pos.x = x; }
    void setBasePosY(float y) { basePosY = y; pos.y = y; }
    void setPosition(sf::Vector2f p) { pos = p; basePosX = p.x; basePosY = p.y; }

    // F1 开发者指令：强制秒杀
    void kill() { health = 0.f; state = BossState::Dead; bullets.clear(); warnings.clear(); }

    float getHitboxRadius() const { return hitboxRadius; }
    std::vector<Bullet>& getBullets() { return bullets; }
    int getCurrentPhase() const { return currentPhase; }

    bool isDying() const { return state == BossState::Dying; }
    bool isDead() const { return state == BossState::Dead; }

    // 受到玩家的解析攻击
    void takeDamage(float amount);
};