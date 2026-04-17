#include "Snake.h"
#include <algorithm>

Snake::Snake() { reset(); }

void Snake::initSprites(const sf::Texture& hTex, const sf::Texture& bTex) {
    headSprite.emplace(hTex);
    bodySprite.emplace(bTex);
    // 将精灵原点设置到图片中心，这样旋转计算时才是以几何中心旋转
    headSprite->setOrigin({ hTex.getSize().x / 2.0f, hTex.getSize().y / 2.0f });
    bodySprite->setOrigin({ bTex.getSize().x / 2.0f, bTex.getSize().y / 2.0f });
}

void Snake::reset() {
    // 初始位置：屏幕下方正中央
    headPos = { 400.f, 450.f };
    currentDir = { 1.f, 0.f }; // 默认朝向右侧

    bodyCount = 2;
    health = 3;
    energy = 100.f;         // 开局满能量
    dashCharges = 2;        // 开局满冲刺
    dashCooldown = 0.f;
    wrapCooldown = 0.f;     // 开局允许立刻使用屏幕穿梭
    triggerBorderFlash = false;
    wasAtEdge = false;
    trail.clear();

    isDashing = false;
    isInvincible = false;
    parseProgress = 0.f;
    isParsing = false;
    isSlowing = false;
}

void Snake::takeDamage(int amount) {
    health -= amount;
    // 受伤后强制给予 1.0s 无敌帧，防止被密集弹幕瞬间秒杀
    isInvincible = true;
    invTimer = 1.0f;
    // 受到核心伤害，强制掉光所有额外数据，回归初始长度
    bodyCount = 2;
}

// update 返回值是解析完成时消耗掉的数据量(用于通知 Game 生成子弹)，否则返回 0
int Snake::update(float dt, sf::Vector2u winSize) {
    // 1. 无敌帧倒计时维护
    if (isInvincible) {
        invTimer -= dt;
        if (invTimer <= 0.f) isInvincible = false;
    }

    // 2. 能量槽每秒自动恢复 12 点
    energy = std::min(100.f, energy + 12.f * dt);

    // 3. 冲刺次数自动充能 (当次数不满 2 次时，每 2.5s 恢复 1 次)
    if (dashCharges < 2) {
        dashCooldown += dt;
        if (dashCooldown >= 2.5f) {
            dashCharges++;
            dashCooldown = 0.f;
        }
    }

    // 4. 穿梭屏幕冷却时间维护
    if (wrapCooldown > 0.f) wrapCooldown -= dt;

    float currentMoveSpeed = normalSpeed;

    // 判定当前蛇头是否贴在了屏幕的物理边缘 (预留 15px 的余量防穿模)
    bool currentlyAtEdge = (headPos.x <= 15.f || headPos.x >= (float)winSize.x - 15.f ||
        headPos.y <= 15.f || headPos.y >= (float)winSize.y - 15.f);

    // 5. 状态派发与移速判定
    if (isDashing) {
        currentMoveSpeed = dashSpeed; // 冲刺极速
        dashTimer -= dt;              // 冲刺只有大约 0.15s
        if (dashTimer <= 0.f) isDashing = false;
    }
    else {
        // 如果按下了 Space减速，或按下了 D解析，则进入低速模式
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
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

    // 6. 处理键盘输入，得出物理运动方向
    if (!isDashing) {
        sf::Vector2f inputDir(0.f, 0.f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))         inputDir.y = -1.f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))  inputDir.y = 1.f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))  inputDir.x = -1.f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) inputDir.x = 1.f;

        bool hasInput = (inputDir.x != 0.f || inputDir.y != 0.f);
        if (hasInput) currentDir = inputDir;

        // 如果没有按下减速键，或者按下了减速键但同时有方向输入，则进行物理移动
        if (!isSlowing || hasInput) headPos += currentDir * currentMoveSpeed * dt;
    }
    else {
        // 冲刺状态下不可改变方向，强行沿 currentDir 移动
        headPos += currentDir * currentMoveSpeed * dt;
    }

    // 7. 屏幕穿梭核心逻辑 (Screen Wrapping)
    bool wrapped = false;
    // 只有在边缘 且 穿梭冷却完毕时，才允许发生物理位置的跳变
    if (currentlyAtEdge && wrapCooldown <= 0.f) {
        if (headPos.x <= 15.f && currentDir.x < 0) { headPos.x = winSize.x - 16.f; wrapped = true; }
        else if (headPos.x >= winSize.x - 15.f && currentDir.x > 0) { headPos.x = 16.f; wrapped = true; }
        else if (headPos.y <= 15.f && currentDir.y < 0) { headPos.y = winSize.y - 16.f; wrapped = true; }
        else if (headPos.y >= winSize.y - 15.f && currentDir.y > 0) { headPos.y = 16.f; wrapped = true; }

        // 发生跳变后，立刻触发 15s 极长冷却
        if (wrapped) {
            wrapCooldown = 15.f;
            currentlyAtEdge = false; // 状态解除，视为不再处于边缘
        }
    }

    // 8. 撞墙惩罚逻辑
    // 如果碰到了边缘，但没有成功发生穿梭(冷却中)，则触发硬性截停与视觉警告
    if (currentlyAtEdge && !wrapped) {
        if (!wasAtEdge && wrapCooldown > 0.f) {
            triggerBorderFlash = true; // 发送边框闪烁请求给外部UI渲染
        }
        currentMoveSpeed = slowSpeed;  // 撞墙强制减速惩罚
        isSlowing = true;
        // 如果正在撞墙，强行根据当前D键状态决定是否在解析
        isParsing = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);

        // 强制把物理坐标钳制在合法范围内，防止出界
        headPos.x = std::clamp(headPos.x, 15.f, (float)winSize.x - 15.f);
        headPos.y = std::clamp(headPos.y, 15.f, (float)winSize.y - 15.f);
    }
    wasAtEdge = currentlyAtEdge; // 记录边缘状态供下一帧判断边缘上升沿

    // 9. 计算蛇头朝向的绝对角度并应用到贴图
    if (headSprite) {
        float angle = 0.f;
        if (currentDir.x > 0.f) angle = 0.f;
        else if (currentDir.x < 0.f) angle = 180.f;
        else if (currentDir.y > 0.f) angle = 90.f;
        else if (currentDir.y < 0.f) angle = -90.f;
        headSprite->setRotation(sf::degrees(angle));
    }

    // 10. 轨迹队列维护 (实现贪吃蛇身体跟随效果)
    float distMoved = std::hypot(headPos.x - prevPos.x, headPos.y - prevPos.y);
    // 情况A：正常的平滑移动。需要利用线性插值记录非常密集的点，确保尾部转弯极度平滑
    if (distMoved > 0.1f && distMoved < 400.f) {
        int steps = static_cast<int>(std::ceil(distMoved / 3.0f)); // 每 3 像素记录一个点
        for (int i = 1; i <= steps; ++i) {
            trail.push_front({ prevPos + (headPos - prevPos) * ((float)i / steps) });
        }
    }
    // 情况B：极长位移，代表刚刚发生了“屏幕穿梭”的坐标跳变
    else if (distMoved >= 400.f) {
        // 直接把跳变后的点塞入最前端，丢弃中间的插值点，否则尾巴会在屏幕中间画一条长直线飞过去
        trail.push_front({ headPos });
    }

    // 修剪历史轨迹队列的长度，防止内存泄漏。
    // 容量只保留能支撑当前 maxBody 个身体节点排布的长度即可。
    if (trail.size() > (size_t)(maxBody + 2) * gap) trail.resize((maxBody + 2) * gap);

    // 11. 核心玩法：解析蓄力逻辑 (Parsing)
    if (isParsing && bodyCount >= 3) {
        // 解析进度增加 = 基础 30/s + (超出3节的部分每节奖励 15/s)
        parseProgress += (30.f + (bodyCount - 3) * 15.f) * dt;

        // 当解析进度达到 100% 时，触发开火
        if (parseProgress >= 100.f) {
            parseProgress = 0.f;
            int consumed = bodyCount - 2; // 算出用于转化为攻击力的真实数据量

            // 开火后，蛇身必须还原到最脆弱的2节状态，且清空轨迹让他瞬间收缩
            bodyCount = 2;
            trail.clear();
            return consumed; // 将伤害源数据抛给 Game 主类处理子弹生成
        }
    }
    else {
        // 未按 D 键，或长度不足时，解析进度槽会迅速衰退归零
        parseProgress = std::max(0.f, parseProgress - 50.f * dt);
    }

    return 0; // 本帧没有解析出子弹，返回0
}

void Snake::draw(sf::RenderWindow& window) {
    if (!headSprite || !bodySprite) return;

    // 1. 冲刺时的幻影/残影特效 (Ghost Trail)
    if (isDashing) {
        // 每隔 3 个历史点抽取一次绘制蛇头半透明残影，最多画 5 个，产生动感
        for (size_t i = 0; i < std::min((size_t)15, trail.size()); i += 3) {
            sf::Sprite ghost = *headSprite;
            ghost.setPosition(trail[i].position);
            // Alpha 通道逐级递减
            ghost.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(120 - i * 6)));
            window.draw(ghost);
        }
    }

    // 2. 绘制尾巴身体结构
    // 只有在非减速/非聚焦状态下才渲染尾部，这是对原作设定的视觉还原
    if (!isSlowing) {
        for (int i = 1; i <= bodyCount; ++i) {
            size_t idx = i * gap; // 根据身体索引算出在历史轨迹中的位置
            if (idx < trail.size()) {
                bodySprite->setPosition(trail[idx].position);

                // 为了让身体连接处显得自然，根据当前点与前一个历史点的向量计算旋转角度
                if (idx + 1 < trail.size()) {
                    sf::Vector2f dir = trail[idx].position - trail[idx + 1].position;
                    // 特别注意：如果发生过屏幕穿梭，这个向量会非常大，强行算角度会乱转
                    // 所以只有距离小于400时才更新尾巴角度
                    if (std::hypot(dir.x, dir.y) < 400.f) {
                        float angle = std::atan2(dir.y, dir.x) * 180.f / 3.14159f;
                        bodySprite->setRotation(sf::degrees(angle));
                    }
                }
                window.draw(*bodySprite);
            }
        }
    }

    // 3. 绘制蛇头本身
    headSprite->setPosition(headPos);
    // 无敌状态下实现经典的“每隔15帧闪烁一次(透明)”的效果
    if (isInvincible && static_cast<int>(invTimer * 15) % 2 == 0) {
        headSprite->setColor(sf::Color(255, 255, 255, 0)); // 全透明
    }
    else {
        // 如果正在解析，蛇头会变为黄色警示，否则为正常的原色
        headSprite->setColor(isParsing ? sf::Color(255, 255, 0) : sf::Color::White);
    }
    window.draw(*headSprite);

    // 4. 绘制悬浮在蛇头下方的“解析进度条”
    if (isParsing && bodyCount >= 3) {
        sf::RectangleShape bar({ 40.f * (parseProgress / 100.f), 4.f });
        bar.setPosition({ headPos.x - 20.f, headPos.y - 25.f });
        bar.setFillColor(sf::Color::Yellow);
        window.draw(bar);
    }
}