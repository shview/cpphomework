#include "Boss.h"
#include <algorithm>

// 构造函数，赋予所有计时器初始默认值
Boss::Boss() : hitboxRadius(20.f), health(100.f), maxHealth(100.f), currentPhase(0), state(BossState::Normal), targetShotsFired(0), targetShotDelay(0.f) {}

// 根据外部传入的 Config 彻底装配当前 Boss
void Boss::init(Difficulty diff, Level lvl, const sf::Font& font,
    const sf::Texture* heartOut, const sf::Texture* heartFill,
    const sf::Texture* bulletTex01, const sf::Texture* bulletTex02,
    const BossConfig& cfg, SoundManager* sm) {

    // 1. 清空上一局可能残留的子弹与状态
    bullets.clear();
    warnings.clear();
    rotationAngle = 0.f;
    patternTimer = 0.f;
    specialTimer = 0.f;
    bubbleTimer = 0.f;
    targetAttackTimer = 0.f;
    targetShotsFired = 0;
    targetShotDelay = 0.f;
    currentPhase = 0;
    soundManager = sm;

    // 2. 载入核心配置与物理位置
    config = cfg;
    basePosX = config.startPosX;
    basePosY = config.startPosY;
    pos = { basePosX, basePosY };

    hitboxRadius = config.bossHitboxRadius; // 动态受击半径(Honest小，Hime大)

    // 3. 难度动态平衡：分配各阶段最大血量
    if (diff == Difficulty::Easy) maxHealth = 35.f;
    else if (diff == Difficulty::Normal) maxHealth = 65.f;
    else maxHealth = 100.f;
    health = maxHealth;

    // 4. 绑定通用贴图
    bulletTexture01 = bulletTex01;
    bulletTexture02 = bulletTex02;
    heartOutlineTex = heartOut;
    heartFillTex = heartFill;

    // 5. 初始化背后的悬浮光环 (如 Hime 身后的图案)
    if (config.bgObjectTex) {
        bgObjectSprite.emplace(*config.bgObjectTex);
        bgObjectSprite->setOrigin({ config.bgObjectTex->getSize().x / 2.0f, config.bgObjectTex->getSize().y / 2.0f });
        bgObjectSprite->setPosition(pos);
    }
    else { bgObjectSprite.reset(); } // 如果没配图就干掉

    // 6. 初始化 Boss 本体切片贴图
    if (config.castTex) {
        bossSprite.emplace(*config.castTex);
        bossSprite->setPosition(pos);
    }

    // 7. 初始化血条 UI (外框与可裁剪的红色填充液)
    if (heartOut) {
        heartOutlineSprite.emplace(*heartOut);
        heartOutlineSprite->setOrigin({ heartOut->getSize().x / 2.0f, heartOut->getSize().y / 2.0f });
        heartOutlineSprite->setPosition(pos);
    }
    if (heartFill) {
        heartFillSprite.emplace(*heartFill);
        heartFillSprite->setPosition(pos);
    }

    // 8. 绑定数字血量文本格式
    pctText.emplace(font);
    pctText->setCharacterSize(14);
    pctText->setStyle(sf::Text::Bold);
    pctText->setFillColor(sf::Color::Red);

    // 9. 设置开局状态为 Spawning(召唤中)，并播放起手施法动画
    state = BossState::Spawning;
    stateTimer = static_cast<float>(config.animCast.totalFrames) / config.animCast.fps;
    setAnimation(config.animCast);
}

// 供 "Revive: Restart Phase" (阶段重置复活) 调用的纯净状态恢复函数
void Boss::resetToCurrentPhase() {
    health = maxHealth;
    bullets.clear();
    warnings.clear();
    state = BossState::Normal;
    fireTimer = 0.f;
    patternTimer = 0.f;
    specialTimer = 0.f;
    bubbleTimer = 0.f;
    targetAttackTimer = 0.f;
    targetShotsFired = 0;
    targetShotDelay = 0.f;
    setAnimation(config.animStay); // 恢复待机动画
}

// 切换当前播放的动画集合
void Boss::setAnimation(const AnimInfo& info) {
    if (currentAnim.tex != info.tex && info.tex != nullptr) {
        currentAnim = info;
        currentFrame = 0;
        animTimer = 0.f;
        bossSprite->setTexture(*info.tex);
        updateSpriteRect();
    }
}

// 根据当前播放的帧数(currentFrame)，通过矩阵计算出对应的贴图区域并裁剪 (Sprite Sheet Slicing)
void Boss::updateSpriteRect() {
    if (!currentAnim.tex) return;
    int w = currentAnim.tex->getSize().x / currentAnim.cols;
    int h = currentAnim.tex->getSize().y / currentAnim.rows;
    int fx = (currentFrame % currentAnim.cols) * w;
    int fy = (currentFrame / currentAnim.cols) * h;
    bossSprite->setTextureRect(sf::IntRect({ fx, fy }, { w, h }));
    bossSprite->setOrigin({ w / 2.0f, h / 2.0f });
}

// 被玩家的 SnakeProjectile 击中
void Boss::takeDamage(float amount) {
    // 各种无敌帧与演出状态期间锁定血量，不允许输出
    if (state == BossState::Dying || state == BossState::Dead || state == BossState::Spawning || state == BossState::PhaseTransition || state == BossState::PhaseWait) return;

    health -= amount;
    if (health <= 0.f) {
        health = 0.f;
        // 如果不是最后一个阶段(2)，则进入锁血等待期 (等双Boss凑齐一起进下阶段)
        if (currentPhase < 2) {
            state = BossState::PhaseWait;
            warnings.clear(); // 清空预警以防转阶段初见杀
            if (soundManager) soundManager->playSound("boss_hurt");
        }
        else {
            // 已经是最后阶段，进入彻底死亡演出
            state = BossState::Dying;
            stateTimer = static_cast<float>(config.animEnd.totalFrames) / config.animEnd.fps;
            setAnimation(config.animEnd);
            if (soundManager) soundManager->playSound("boss_dead");
        }
        return;
    }

    // 正常扣血，给予轻微的视觉受击硬直 (不打断核心攻击逻辑，仅切动画)
    if (state != BossState::Hit) {
        state = BossState::Hit;
        stateTimer = static_cast<float>(config.animSuffer.totalFrames) / config.animSuffer.fps;
        setAnimation(config.animSuffer);
        if (soundManager) soundManager->playSound("boss_hurt");
    }
}

// 被 Game 主循环调用：真正推进到下一个高难度攻击阶段
void Boss::advancePhase() {
    if (state == BossState::PhaseWait) {
        currentPhase++;
        health = maxHealth;

        // 瞬间抹平视觉 UI 残留，防止旧血量闪屏
        if (pctText && heartFillTex) {
            pctText->setString(std::to_string(static_cast<int>(health)) + " / " + std::to_string(static_cast<int>(maxHealth)));
            sf::Vector2u texSize = heartFillTex->getSize();
            heartFillSprite->setTextureRect(sf::IntRect({ 0, 0 }, { static_cast<int>(texSize.x), static_cast<int>(texSize.y) }));
            heartFillSprite->setOrigin({ texSize.x / 2.0f, static_cast<float>(texSize.y / 2.0f) });
        }

        // 播放新阶段施法霸体动画
        state = BossState::PhaseTransition;
        stateTimer = static_cast<float>(config.animCast.totalFrames) / config.animCast.fps;
        setAnimation(config.animCast);
    }
}

// 【预警机制核心】在屏幕上生成一条红色半透明区域，几秒后实体化为致命子弹
void Boss::spawnWarning(sf::Vector2f startPos, sf::Vector2f size, sf::Vector2f vel, float rot, float maxTime, int type, bool attached) {
    WarningArea w;
    w.shape.setSize(size);
    // 【关键几何学】：将渲染原点设为矩形的“最左侧边缘的垂直中心”，
    // 这样当应用 Rotation 时，红线只会向 Boss 的“正前方”延伸，不会穿身刺到背后。
    w.shape.setOrigin({ 0.f, size.y / 2.f });
    w.shape.setPosition(startPos);
    w.shape.setRotation(sf::degrees(rot));
    w.timer = 0.f;
    w.maxTime = maxTime;
    w.bulletType = type;
    w.startPos = startPos;
    w.velocity = vel;
    w.rotation = rot;
    w.attachedToBoss = attached;
    w.beepedHalfway = false; // 音频标记复位
    warnings.push_back(w);

    if (soundManager) soundManager->playSound("warning"); // 发出第一声警告音
}

// Boss AI 循环，涵盖了所有移动、攻击和阶段判定
void Boss::update(float dt, Difficulty diff, Level lvl, sf::Vector2f playerPos, sf::Vector2f playerDir, sf::Vector2f playerBodyPos) {

    // 1. 播放器逻辑：更新帧动画
    if (currentAnim.tex) {
        animTimer += dt;
        float frameTime = 1.0f / currentAnim.fps;
        if (animTimer >= frameTime) {
            animTimer -= frameTime;
            // 死亡动画播到最后一帧就定格，不循环
            if (state == BossState::Dying && currentFrame == currentAnim.totalFrames - 1) {}
            else currentFrame = (currentFrame + 1) % currentAnim.totalFrames;
            updateSpriteRect();
        }
    }

    // 2. 拦截僵直状态：如果在死亡过程，只允许原有的子弹飞行
    if (state == BossState::Dying) {
        stateTimer -= dt;
        if (stateTimer <= 0.f) state = BossState::Dead;
        updateBullets(dt);
        return;
    }
    if (state == BossState::Dead) return;

    // 3. 拦截阶段等待：锁定不动
    if (state == BossState::PhaseWait) {
        updateBullets(dt);
        return;
    }

    // 4. 处理出生与转阶段动画（带有一闪一闪的光效无敌）
    if (state == BossState::Spawning || state == BossState::PhaseTransition) {
        stateTimer -= dt;
        if (heartOutlineSprite) heartOutlineSprite->setColor(static_cast<int>(stateTimer * 10) % 2 == 0 ? sf::Color(255, 255, 255, 100) : sf::Color::White);
        if (stateTimer <= 0.f) {
            state = BossState::Normal;
            if (heartOutlineSprite) heartOutlineSprite->setColor(sf::Color::White);
            setAnimation(config.animStay); // 动画播完切回待机
        }
        updateBullets(dt);
        return;
    }

    // 5. 受击硬直：UI 血心变为红色
    if (state == BossState::Hit) {
        stateTimer -= dt;
        if (heartFillSprite) heartFillSprite->setColor(sf::Color(255, 100, 100));
        if (stateTimer <= 0.f) {
            state = BossState::Normal;
            if (heartFillSprite) heartFillSprite->setColor(sf::Color::White);
            setAnimation(config.animStay);
        }
    }

    // 6. UI 逻辑：根据血量百分比裁剪红心填充贴图 (底部保留，顶部截断)
    if (heartFillSprite && heartFillTex && pctText) {
        float pct = std::clamp(health / maxHealth, 0.f, 1.f);
        sf::Vector2u texSize = heartFillTex->getSize();
        int visibleH = static_cast<int>(texSize.y * pct);
        int topY = texSize.y - visibleH;
        heartFillSprite->setTextureRect(sf::IntRect({ 0, topY }, { static_cast<int>(texSize.x), visibleH }));
        heartFillSprite->setOrigin({ texSize.x / 2.0f, static_cast<float>(texSize.y / 2.0f - topY) });
        // 刷新悬浮文本
        pctText->setString(std::to_string(static_cast<int>(health)) + " / " + std::to_string(static_cast<int>(maxHealth)));
        sf::FloatRect bounds = pctText->getLocalBounds();
        pctText->setOrigin({ bounds.size.x / 2.0f, bounds.size.y / 2.0f });
        pctText->setPosition({ pos.x, pos.y + 40.f });
    }

    // 7. 物理运动逻辑
    patternTimer += dt;
    // 如果是 Level 2 (单Hime)，给她加一个全屏的正弦波左右摇摆游走
    if (lvl == Level::Level2) pos.x = basePosX + std::sin(patternTimer * 1.5f) * 200.f;
    else pos.x = basePosX;
    pos.y = basePosY; // basePosX 和 Y 在双 Boss 时由 Game 主循环强制插值接管

    // 同步渲染组件坐标
    if (bgObjectSprite) bgObjectSprite->setPosition(pos);
    if (bossSprite) bossSprite->setPosition(pos);
    if (heartOutlineSprite) heartOutlineSprite->setPosition(pos);
    if (heartFillSprite) heartFillSprite->setPosition(pos);

    // =====================================
    // ⚔️ 弹幕发射体系 ⚔️
    // =====================================

    // A. 普通弹幕 (Basic Attacks)
    // 根据难度查表得到当前冷却阈值
    float baseFireRate = (diff == Difficulty::Hard) ? 0.16f : ((diff == Difficulty::Normal) ? 0.24f : 0.36f);
    fireTimer += dt;

    if (fireTimer > baseFireRate) {
        fireTimer = 0.f;
        // 使用 Phase 来决定当前的攻击阵列 (0=四面开花, 1=十字旋转, 2=双螺旋)
        int attackMode = currentPhase;
        if (currentPhase == 0) attackMode = 2;
        else if (currentPhase == 1) attackMode = 0;
        else if (currentPhase == 2) attackMode = 1;

        // -1 表示根据配置自带逻辑选子弹
        int finalBullet = config.forceBulletType != -1 ? config.forceBulletType : 0;

        if (attackMode == 0) {
            rotationAngle += 18.f;
            for (int i : {0, 90, 180, 270}) fireBullet(rotationAngle + (float)i, 220.f, finalBullet);
        }
        else if (attackMode == 1) {
            rotationAngle += 35.f;
            fireBullet(rotationAngle, 280.f, finalBullet);
            fireBullet(rotationAngle + 180.f, 280.f, finalBullet);
        }
        else if (attackMode == 2) {
            // 每隔 8 帧爆发一次全屏圆环
            if (static_cast<int>(patternTimer * 10) % 8 == 0) {
                int overrideB = config.forceBulletType != -1 ? config.forceBulletType : 1;
                for (int i = 0; i < 360; i += 30) fireBullet((float)i + rotationAngle, 180.f, overrideB);
            }
            rotationAngle += 5.f;
        }
    }

    // B. 环境大招：直线/横扫特攻阵列 (Special Sweeps)
    specialTimer += dt;
    if (config.bossId == 1 && specialTimer > 10.0f) {
        // Honest：天降正义垂直激光
        specialTimer = 0.f;
        std::vector<float> spawnedX;
        for (int i = 0; i < 3 + currentPhase; ++i) {
            float rx;
            bool overlap = true;
            int attempts = 0;
            // 【防重叠核心】：检测新生成的 X 坐标，如果和之前生成的柱子距离不足 80px，则重新抽签
            while (overlap && attempts < 10) {
                rx = 50.f + (std::rand() % 700);
                overlap = false;
                for (float x : spawnedX) { if (std::abs(x - rx) < 80.f) { overlap = true; break; } }
                attempts++;
            }
            spawnedX.push_back(rx);
            // 预警宽度 30px
            spawnWarning({ rx, -50.f }, { 1500.f, 30.f }, { 0.f, 500.f }, 90.f, 1.5f, 10);
        }
    }
    else if (config.bossId == 2 && specialTimer > 12.0f) {
        // Hime：左右夹击横扫大扇
        specialTimer = 0.f;
        std::vector<float> spawnedY;
        for (int i = 0; i < 2 + currentPhase; ++i) {
            bool fromLeft = (std::rand() % 2 == 0);
            float ry;
            bool overlap = true;
            int attempts = 0;
            // 纵向防重叠检测
            while (overlap && attempts < 10) {
                ry = 100.f + (std::rand() % 400);
                overlap = false;
                for (float y : spawnedY) { if (std::abs(y - ry) < 80.f) { overlap = true; break; } }
                attempts++;
            }
            spawnedY.push_back(ry);
            // 预警宽度高达 130px 模拟扇子体积
            if (fromLeft) spawnWarning({ -100.f, ry }, { 2000.f, 130.f }, { 600.f, 0.f }, 0.f, 1.5f, 11);
            else spawnWarning({ 900.f, ry }, { 2000.f, 130.f }, { -600.f, 0.f }, 180.f, 1.5f, 11);
        }
    }

    // C. 针对玩家的狙击特攻 (Targeted Snipes)
    // 两个 Boss 会使用完全不对称、不同节奏的逻辑来逼迫玩家走位
    targetAttackTimer += dt;
    if (config.bossId == 2) {
        // Hime: 顺发大面积极限封锁
        if (targetAttackTimer >= 12.0f) {
            targetAttackTimer -= 12.0f;
            int specialType = 11;

            // 计算指向玩家蛇身中心的基础角度
            sf::Vector2f dirBody = playerBodyPos - pos;
            float distBody = std::hypot(dirBody.x, dirBody.y);
            if (distBody > 0.1f) dirBody /= distBody;
            float baseRot = std::atan2(dirBody.y, dirBody.x) * 180.f / 3.14159f;

            float offsetAngle = 35.f; // 左右封锁线的绝对夹角

            if (soundManager) soundManager->playSound("aim");

            // 1发瞄准中心
            spawnWarning(pos, { 2500.f, 130.f }, dirBody * 650.f, baseRot, 1.5f, specialType, true);
            // 2发封锁侧翼
            sf::Vector2f dirLeft(std::cos((baseRot + offsetAngle) * 3.14159f / 180.f), std::sin((baseRot + offsetAngle) * 3.14159f / 180.f));
            spawnWarning(pos, { 2500.f, 130.f }, dirLeft * 650.f, baseRot + offsetAngle, 1.5f, specialType, true);
            sf::Vector2f dirRight(std::cos((baseRot - offsetAngle) * 3.14159f / 180.f), std::sin((baseRot - offsetAngle) * 3.14159f / 180.f));
            spawnWarning(pos, { 2500.f, 130.f }, dirRight * 650.f, baseRot - offsetAngle, 1.5f, specialType, true);
        }
    }
    else if (config.bossId == 1) {
        // Honest: 高延迟的精准三连点射 (附带走位预判)
        // targetAttackTimer 15秒时打第一枪，然后利用 targetShotDelay 每秒打一枪
        if (targetShotsFired == 0 && targetAttackTimer >= 15.0f) {
            targetShotsFired = 1;
            targetShotDelay = 0.f;

            // 提取玩家运动方向，超前预判 100 个像素
            sf::Vector2f dirVec = (playerDir.x == 0.f && playerDir.y == 0.f) ? sf::Vector2f(1.f, 0.f) : playerDir;
            sf::Vector2f targetPos = playerPos + dirVec * 100.f;
            sf::Vector2f dir = targetPos - pos;
            float dist = std::hypot(dir.x, dir.y);
            if (dist > 0.1f) dir /= dist;
            float rot = std::atan2(dir.y, dir.x) * 180.f / 3.14159f;

            if (soundManager) soundManager->playSound("aim");
            spawnWarning(pos, { 2500.f, 30.f }, dir * 650.f, rot, 1.5f, 10, true);

        }
        else if (targetShotsFired > 0) {
            targetShotDelay += dt;
            // 冷却满 1 秒，打出下一发
            if (targetShotDelay >= 1.0f) {
                targetShotDelay -= 1.0f;
                targetShotsFired++;

                // 每开一枪都重新抓取玩家实时状态进行新一轮预判
                sf::Vector2f dirVec = (playerDir.x == 0.f && playerDir.y == 0.f) ? sf::Vector2f(1.f, 0.f) : playerDir;
                sf::Vector2f targetPos = playerPos + dirVec * 100.f;
                sf::Vector2f dir = targetPos - pos;
                float dist = std::hypot(dir.x, dir.y);
                if (dist > 0.1f) dir /= dist;
                float rot = std::atan2(dir.y, dir.x) * 180.f / 3.14159f;

                if (soundManager) soundManager->playSound("aim");
                spawnWarning(pos, { 2500.f, 30.f }, dir * 650.f, rot, 1.5f, 10, true);

                // 打满 3 发，进入下一次的大循环
                if (targetShotsFired >= 3) {
                    targetShotsFired = 0;
                    // 回退 12 秒，这样保证大招循环周期与 Hime (12s) 一致
                    targetAttackTimer -= 12.0f;
                }
            }
        }
    }

    // D. 混乱环境：急速穿梭泡泡 (Hyper Bubbles)
    bubbleTimer += dt;
    if (bubbleTimer > 20.f) {
        bubbleTimer = 0.f;
        std::vector<sf::Vector2f> spawnedBubbles; // 记录生成点防重叠

        for (int i = 0; i < 2 + currentPhase; ++i) {
            int edge = std::rand() % 4; // 抽取从屏幕四边的哪一边出发
            sf::Vector2f startP, endP;
            bool overlap = true;
            int attempts = 0;

            while (overlap && attempts < 10) {
                // 强制让泡泡横平竖直飞 (Y坐标对齐 或 X坐标对齐)
                if (edge == 0) { startP = { -50.f, 50.f + (float)(std::rand() % 500) }; endP = { 850.f, startP.y }; }
                else if (edge == 1) { startP = { 850.f, 50.f + (float)(std::rand() % 500) }; endP = { -50.f, startP.y }; }
                else if (edge == 2) { startP = { 50.f + (float)(std::rand() % 700), -50.f }; endP = { startP.x, 650.f }; }
                else { startP = { 50.f + (float)(std::rand() % 700), 650.f }; endP = { startP.x, -50.f }; }

                overlap = false;
                for (const auto& sb : spawnedBubbles) {
                    if (std::hypot(sb.x - startP.x, sb.y - startP.y) < 80.f) {
                        overlap = true;
                        break;
                    }
                }
                attempts++;
            }
            spawnedBubbles.push_back(startP);

            sf::Vector2f dir = endP - startP;
            float dist = std::hypot(dir.x, dir.y);
            if (dist > 0.1f) dir /= dist;
            float rot = std::atan2(dir.y, dir.x) * 180.f / 3.14159f;

            // 极短预警 0.8s，极快飞行速度 1800，专门用来封玩家走位后路
            spawnWarning(startP, { 2500.f, 50.f }, dir * 1800.f, rot, 0.8f, 12);
        }
    }

    // 独立循环：推进所有的子弹和预警线
    updateBullets(dt);
}

// 物理逻辑：子弹飞行与预警线实体化
void Boss::updateBullets(float dt) {
    // 遍历所有场上子弹，飞出屏幕范围就将其释放回收
    for (auto it = bullets.begin(); it != bullets.end(); ) {
        it->sprite.move(it->velocity * dt);
        sf::Vector2f p = it->sprite.getPosition();
        if (p.x < -100 || p.x > 900 || p.y < -100 || p.y > 700) it = bullets.erase(it);
        else ++it;
    }

    // 遍历所有正在倒计时的预警红线
    for (auto it = warnings.begin(); it != warnings.end(); ) {
        // 如果是从 Boss 手里发射的，那么 Boss 走位时，预警线要一直跟着同步偏移
        if (it->attachedToBoss) {
            it->startPos = pos;
            it->shape.setPosition(pos);
        }

        // 半程音效提示：给玩家的紧张感加料
        if (!it->beepedHalfway && it->timer >= it->maxTime / 2.f) {
            if (soundManager) soundManager->playSound("warning");
            it->beepedHalfway = true;
        }

        it->timer += dt;
        // 预警时间结束，开始实体化，变为真子弹投入战场
        if (it->timer >= it->maxTime) {
            const sf::Texture* tex = nullptr;
            if (it->bulletType == 10) tex = config.texHonestSpecial;
            else if (it->bulletType == 11) tex = config.texHimeSpecial;
            else if (it->bulletType == 12) tex = config.texBubble;

            if (tex) {
                Bullet b(*tex, it->velocity, it->bulletType);
                sf::Vector2u ts = tex->getSize();
                b.sprite.setOrigin({ ts.x / 2.f, ts.y / 2.f });
                b.sprite.setPosition(it->startPos);

                // 根据类型套用旋转角度，并附带不同的爆发声
                if (it->bulletType == 10 || it->bulletType == 11) {
                    b.sprite.setRotation(sf::degrees(it->rotation + 90.f));
                    if (soundManager) soundManager->playSound("special");
                }
                else {
                    b.sprite.setRotation(sf::degrees(it->rotation));
                    if (soundManager) soundManager->playSound("bubble");
                }
                bullets.push_back(b);
            }
            // 实体化后消除该红线
            it = warnings.erase(it);
        }
        else {
            ++it;
        }
    }
}

// 快速生成常规的漫天弹幕 (Bullet Hell)
void Boss::fireBullet(float angleDeg, float speed, int bulletType) {
    const sf::Texture* currentTex = (bulletType == 0) ? bulletTexture01 : bulletTexture02;
    if (!currentTex) return;

    // 三角函数分解速度向量
    float angleRad = angleDeg * 3.14159f / 180.f;
    Bullet b(*currentTex, { std::cos(angleRad) * speed, std::sin(angleRad) * speed }, bulletType);
    sf::Vector2u texSize = currentTex->getSize();
    b.sprite.setOrigin({ texSize.x / 2.0f, texSize.y / 2.0f });
    b.sprite.setPosition(pos);
    b.sprite.setRotation(sf::degrees(angleDeg));
    bullets.push_back(b);

    // 【混合混音】：普通弹幕数量巨大，音量必须极大幅度压低(25)，否则会造成“声污染”掩盖重点音效
    if (soundManager) soundManager->playSound("bullet", 25.f);
}

// 所有 Boss 组件与特效渲染
void Boss::draw(sf::RenderWindow& window) {
    // 只有在非死亡且非阶段锁死等待时，才绘制肉身和UI
    if (state != BossState::Dead && state != BossState::PhaseWait) {
        if (bgObjectSprite) window.draw(*bgObjectSprite);
        if (bossSprite) window.draw(*bossSprite);
        if (state != BossState::Spawning && state != BossState::Dying) {
            if (heartFillSprite) window.draw(*heartFillSprite);
            if (heartOutlineSprite) window.draw(*heartOutlineSprite);
            if (pctText) window.draw(*pctText);
        }
    }

    // 预警线的半透明颜色随着时间通过奇偶帧快速闪烁，带来危机感
    for (auto& w : warnings) {
        if (static_cast<int>(w.timer * 15) % 2 == 0) w.shape.setFillColor(sf::Color(255, 0, 0, 100));
        else w.shape.setFillColor(sf::Color(255, 0, 0, 50));
        window.draw(w.shape);
    }

    // 绘制所有飞行中的致命子弹
    for (auto& b : bullets) window.draw(b.sprite);
}