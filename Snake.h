#pragma once
#include "Global.h"
#include <deque>
#include <optional>

class Snake {
public:
    // 玩家的运动轨迹双端队列 (Deque)。用于实现类似传统贪吃蛇的“跟随效果”。
    // 记录蛇头走过的历史坐标，尾巴根据间距 (gap) 在此队列中取点渲染。
    std::deque<Pose> trail;

    int bodyCount = 2;          // 当前长度 (蛇头 + 额外数据节数，基础为2)
    const int maxBody = 10;     // 最大长度限制
    int gap = 5;                // 相邻身体节点在 trail 轨迹中读取位置的索引间隔

    // 核心移速设定
    float normalSpeed = 239.6f;  // 默认平跑移速
    float slowSpeed = 82.9f;     // 聚焦减速状态下移速
    float dashSpeed = 1290.2f;   // L-Shift 冲刺期间极速

    sf::Vector2f headPos;        // 蛇头当前的绝对物理位置
    sf::Vector2f currentDir;     // 蛇头当前的朝向向量 (归一化)

    // 各类状态标志位
    bool isParsing;              // 是否正在按住 D 键蓄力解析
    bool isSlowing;              // 是否正在按住 Space 键减速
    bool isInvincible;           // 当前是否无敌 (受伤后或冲刺中)
    bool isDashing;              // 当前是否正在冲刺状态中

    // 计时器与资源变量
    float invTimer;              // 无敌剩余时间
    float parseProgress;         // 解析进度 (0~100)
    float dashTimer;             // 冲刺持续剩余时间
    float dashCooldown;          // 冲刺次数恢复的读条进度
    int health;                  // 当前生命值
    int dashCharges;             // 当前拥有的冲刺次数 (上限2)
    float energy;                // Q技能所需的能量槽 (0~100)

    // 特殊机制：屏幕穿梭相关控制变量
    float wrapCooldown = 0.f;        // 穿梭屏幕的冷却时间 (上限 15s)
    bool triggerBorderFlash = false; // 是否需要通知UI层闪烁蓝框警告
    bool wasAtEdge = false;          // 记录上一帧是否在边缘，防止冷却时贴墙连续触发警告音

    // 渲染资源 (使用 std::optional 确保安全的生命周期延迟初始化)
    std::optional<sf::Sprite> headSprite;
    std::optional<sf::Sprite> bodySprite;

    Snake();

    // 初始化贴图资源
    void initSprites(const sf::Texture& hTex, const sf::Texture& bTex);

    // 重置所有玩家属性 (用于重开、复活等)
    void reset();

    // 玩家承受直接伤害
    void takeDamage(int amount = 1);

    // 核心物理、输入处理与机制更新，返回值为：本帧解析完成后消耗的数据节数
    int update(float dt, sf::Vector2u winSize);

    // 渲染自身及其 UI(如解析进度条) 到指定窗口
    void draw(sf::RenderWindow& window);
};