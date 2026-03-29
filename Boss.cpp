#include "Boss.h"
#include <algorithm>

Boss::Boss() : hitboxRadius(20.f), health(100.f), maxHealth(100.f), currentPhase(0), state(BossState::Normal) {}

void Boss::init(Difficulty diff, Level lvl, const sf::Font& font,
    const sf::Texture* heartOut, const sf::Texture* heartFill,
    const sf::Texture* bulletTex01, const sf::Texture* bulletTex02,
    const BossConfig& cfg) {
    bullets.clear();
    warnings.clear(); // 清空预警
    rotationAngle = 0.f;
    patternTimer = 0.f;
    specialTimer = 0.f;
    bubbleTimer = 0.f;
    currentPhase = 0;
    pos = { 400.f, 200.f };
    config = cfg;

    if (diff == Difficulty::Easy) maxHealth = 150.f;
    else if (diff == Difficulty::Normal) maxHealth = 300.f;
    else maxHealth = 500.f;
    health = maxHealth;

    bulletTexture01 = bulletTex01;
    bulletTexture02 = bulletTex02;
    heartOutlineTex = heartOut;
    heartFillTex = heartFill;

    if (config.bgObjectTex) {
        bgObjectSprite.emplace(*config.bgObjectTex);
        bgObjectSprite->setOrigin({ config.bgObjectTex->getSize().x / 2.0f, config.bgObjectTex->getSize().y / 2.0f });
        bgObjectSprite->setPosition(pos);
    }
    else { bgObjectSprite.reset(); }

    if (config.castTex) {
        bossSprite.emplace(*config.castTex);
        bossSprite->setPosition(pos);
    }

    if (heartOut) {
        heartOutlineSprite.emplace(*heartOut);
        heartOutlineSprite->setOrigin({ heartOut->getSize().x / 2.0f, heartOut->getSize().y / 2.0f });
        heartOutlineSprite->setPosition(pos);
    }
    if (heartFill) {
        heartFillSprite.emplace(*heartFill);
        heartFillSprite->setPosition(pos);
    }

    pctText.emplace(font);
    pctText->setCharacterSize(14);
    pctText->setStyle(sf::Text::Bold);
    pctText->setFillColor(sf::Color::Red);

    state = BossState::Spawning;
    stateTimer = static_cast<float>(config.animCast.totalFrames) / config.animCast.fps;
    setAnimation(config.animCast);
}

void Boss::setAnimation(const AnimInfo& info) {
    if (currentAnim.tex != info.tex && info.tex != nullptr) {
        currentAnim = info;
        currentFrame = 0;
        animTimer = 0.f;
        bossSprite->setTexture(*info.tex);
        updateSpriteRect();
    }
}

void Boss::updateSpriteRect() {
    if (!currentAnim.tex) return;
    int w = currentAnim.tex->getSize().x / currentAnim.cols;
    int h = currentAnim.tex->getSize().y / currentAnim.rows;
    int fx = (currentFrame % currentAnim.cols) * w;
    int fy = (currentFrame / currentAnim.cols) * h;
    bossSprite->setTextureRect(sf::IntRect({ fx, fy }, { w, h }));
    bossSprite->setOrigin({ w / 2.0f, h / 2.0f });
}

void Boss::takeDamage(float amount) {
    if (state == BossState::Dying || state == BossState::Dead || state == BossState::Spawning || state == BossState::PhaseTransition) return;

    health -= amount;
    if (health <= 0.f) {
        health = 0.f;
        if (currentPhase < 2) {
            currentPhase++;
            health = maxHealth;
            state = BossState::PhaseTransition;
            stateTimer = static_cast<float>(config.animCast.totalFrames) / config.animCast.fps;
            bullets.clear();
            warnings.clear(); // 换阶段清空场上警告
            setAnimation(config.animCast);
        }
        else {
            state = BossState::Dying;
            stateTimer = static_cast<float>(config.animEnd.totalFrames) / config.animEnd.fps;
            setAnimation(config.animEnd);
        }
        return;
    }

    if (state != BossState::Hit) {
        state = BossState::Hit;
        stateTimer = static_cast<float>(config.animSuffer.totalFrames) / config.animSuffer.fps;
        setAnimation(config.animSuffer);
    }
}

void Boss::update(float dt, Difficulty diff, Level lvl) {
    if (currentAnim.tex) {
        animTimer += dt;
        float frameTime = 1.0f / currentAnim.fps;
        if (animTimer >= frameTime) {
            animTimer -= frameTime;
            if (state == BossState::Dying && currentFrame == currentAnim.totalFrames - 1) {}
            else currentFrame = (currentFrame + 1) % currentAnim.totalFrames;
            updateSpriteRect();
        }
    }

    if (state == BossState::Dying) {
        stateTimer -= dt;
        if (stateTimer <= 0.f) state = BossState::Dead;
        updateBullets(dt);
        return;
    }
    if (state == BossState::Dead) return;

    if (state == BossState::Spawning || state == BossState::PhaseTransition) {
        stateTimer -= dt;
        if (heartOutlineSprite) heartOutlineSprite->setColor(static_cast<int>(stateTimer * 10) % 2 == 0 ? sf::Color(255, 255, 255, 100) : sf::Color::White);
        if (stateTimer <= 0.f) {
            state = BossState::Normal;
            if (heartOutlineSprite) heartOutlineSprite->setColor(sf::Color::White);
            setAnimation(config.animStay);
        }
        updateBullets(dt);
        return;
    }

    if (state == BossState::Hit) {
        stateTimer -= dt;
        if (heartFillSprite) heartFillSprite->setColor(sf::Color(255, 100, 100));
        if (stateTimer <= 0.f) {
            state = BossState::Normal;
            if (heartFillSprite) heartFillSprite->setColor(sf::Color::White);
            setAnimation(config.animStay);
        }
    }

    // 更新心形 UI
    if (heartFillSprite && heartFillTex && pctText) {
        float pct = std::clamp(health / maxHealth, 0.f, 1.f);
        sf::Vector2u texSize = heartFillTex->getSize();
        int visibleH = static_cast<int>(texSize.y * pct);
        int topY = texSize.y - visibleH;
        heartFillSprite->setTextureRect(sf::IntRect({ 0, topY }, { static_cast<int>(texSize.x), visibleH }));
        heartFillSprite->setOrigin({ texSize.x / 2.0f, static_cast<float>(texSize.y / 2.0f - topY) });
        pctText->setString(std::to_string(static_cast<int>(pct * 100)) + "%");
        sf::FloatRect bounds = pctText->getLocalBounds();
        pctText->setOrigin({ bounds.size.x / 2.0f, bounds.size.y / 2.0f });
        pctText->setPosition(pos);
    }

    patternTimer += dt;
    if (lvl == Level::Level2) { pos.x = 400.f + std::sin(patternTimer * 1.5f) * 200.f; }

    if (bgObjectSprite) bgObjectSprite->setPosition(pos);
    if (bossSprite) bossSprite->setPosition(pos);
    if (heartOutlineSprite) heartOutlineSprite->setPosition(pos);
    if (heartFillSprite) heartFillSprite->setPosition(pos);

    // ==========================================
    // 普通攻击系统 (变慢 1.5 倍)
    // ==========================================
    float baseFireRate = ((diff == Difficulty::Hard) ? 0.16f : ((diff == Difficulty::Normal) ? 0.24f : 0.36f)) * 1.5f;
    fireTimer += dt;

    if (fireTimer > baseFireRate) {
        fireTimer = 0.f;
        if (currentPhase == 0) {
            if (static_cast<int>(patternTimer * 10) % 8 == 0) {
                for (int i = 0; i < 360; i += 30) fireBullet((float)i + rotationAngle, 180.f, 1);
            }
            rotationAngle += 5.f;
        }
        else if (currentPhase == 1) {
            rotationAngle += 18.f;
            for (int i : {0, 90, 180, 270}) fireBullet(rotationAngle + (float)i, 220.f, 0);
        }
        else if (currentPhase == 2) {
            rotationAngle += 35.f;
            fireBullet(rotationAngle, 280.f, 0);
            fireBullet(rotationAngle + 180.f, 280.f, 0);
        }
    }

    // ==========================================
    // 专属与全场特殊攻击系统
    // ==========================================
    specialTimer += dt;
    bubbleTimer += dt;

    // 1. 泡泡攻击 (每 20 秒一次)
    if (bubbleTimer >= 20.f) {
        bubbleTimer = 0.f;
        int bubbleCount = (currentPhase == 0) ? 2 : ((currentPhase == 1) ? 4 : 7);
        for (int i = 0; i < bubbleCount; ++i) {
            WarningArea w;
            w.bulletType = 12; // 泡泡类型
            w.maxTime = 1.5f; w.timer = 1.5f; // 1.5秒预警

            // 随机在四周边缘生成
            int edge = rand() % 4;
            sf::Vector2f target;
            if (edge == 0) { w.startPos = { (float)(rand() % 800), -30.f }; target = { (float)(rand() % 800), 630.f }; }
            else if (edge == 1) { w.startPos = { (float)(rand() % 800), 630.f }; target = { (float)(rand() % 800), -30.f }; }
            else if (edge == 2) { w.startPos = { -30.f, (float)(rand() % 600) }; target = { 830.f, (float)(rand() % 600) }; }
            else { w.startPos = { 830.f, (float)(rand() % 600) }; target = { -30.f, (float)(rand() % 600) }; }

            sf::Vector2f dir = target - w.startPos;
            float len = std::hypot(dir.x, dir.y);
            dir /= len;
            w.velocity = dir * 900.f; // 极速
            w.rotation = std::atan2(dir.y, dir.x) * 180.f / 3.14159f;

            w.shape.setSize({ len, 24.f });
            w.shape.setOrigin({ 0.f, 12.f });
            w.shape.setPosition(w.startPos);
            w.shape.setRotation(sf::degrees(w.rotation));
            w.shape.setFillColor(sf::Color(255, 50, 50, 80));
            warnings.push_back(w);
        }
    }

    // 2. Boss专属攻击
    float specialRate = (diff == Difficulty::Hard) ? 1.0f : ((diff == Difficulty::Normal) ? 1.5f : 2.0f);
    if (config.bossId == 2) specialRate *= 2.0f; // Hime 扇子数量少，发射间隔慢一倍

    if (specialTimer > specialRate) {
        specialTimer = 0.f;
        if (config.bossId == 1) {
            // Honest: 垂直下落弹幕
            int count = 0;
            for (auto& w : warnings) if (w.bulletType == 10) count++;
            for (auto& b : bullets) if (b.type == 10) count++;
            if (count < 25) { // 允许最大数量极多
                WarningArea w;
                w.bulletType = 10;
                w.maxTime = 1.0f; w.timer = 1.0f;
                float spawnX = (rand() % 760) + 20.f;
                w.startPos = { spawnX, -50.f };
                w.velocity = { 0.f, 350.f + (int)diff * 80.f };
                w.rotation = 90.f; // 向下
                w.shape.setSize({ 20.f, 650.f });
                w.shape.setOrigin({ 10.f, 0.f });
                w.shape.setPosition({ spawnX, 0.f });
                w.shape.setFillColor(sf::Color(255, 0, 0, 100));
                warnings.push_back(w);
            }
        }
        else if (config.bossId == 2) {
            // Hime: 左右横扫大扇子
            int count = 0;
            for (auto& w : warnings) if (w.bulletType == 11) count++;
            for (auto& b : bullets) if (b.type == 11) count++;
            if (count < 2) { // 场上最多两个
                WarningArea w;
                w.bulletType = 11;
                w.maxTime = 1.5f; w.timer = 1.5f;
                float spawnY = (rand() % 500) + 50.f;
                bool leftToRight = (rand() % 2 == 0);
                w.startPos = { leftToRight ? -50.f : 850.f, spawnY };
                w.velocity = { leftToRight ? (350.f + (int)diff * 80.f) : -(350.f + (int)diff * 80.f), 0.f };
                w.rotation = leftToRight ? 0.f : 180.f;
                w.shape.setSize({ 850.f, 60.f });
                w.shape.setOrigin({ 0.f, 30.f });
                w.shape.setPosition({ 0.f, spawnY });
                w.shape.setFillColor(sf::Color(255, 0, 0, 100));
                warnings.push_back(w);
            }
        }
    }

    updateBullets(dt);
}

void Boss::updateBullets(float dt) {
    // 1. 更新预警线
    for (auto it = warnings.begin(); it != warnings.end(); ) {
        it->timer -= dt;
        // 让预警线产生频闪特效
        float blink = std::abs(std::sin(it->timer * 20.f)) * 100.f + 50.f;
        // 修复后（将 sf::Uint8 改为 std::uint8_t）：
        it->shape.setFillColor(sf::Color(255, 50, 50, static_cast<std::uint8_t>(blink)));
        // 预警结束，发射真正的实体弹幕
        if (it->timer <= 0.f) {
            const sf::Texture* tex = nullptr;
            if (it->bulletType == 10) tex = config.texHonestSpecial;
            else if (it->bulletType == 11) tex = config.texHimeSpecial;
            else if (it->bulletType == 12) tex = config.texBubble;

            if (tex) {
                Bullet b(*tex, it->velocity, it->bulletType);
                b.sprite.setOrigin({ tex->getSize().x / 2.f, tex->getSize().y / 2.f });
                b.sprite.setPosition(it->startPos);

                // 由于原图全部朝上，通过原角度 + 90 度将其对齐到飞行方向
                b.sprite.setRotation(sf::degrees(it->rotation + 90.f));

                // 对 Honest 专属弹幕稍微缩小一点，体现“量多体积小”
                if (it->bulletType == 10) b.sprite.setScale({ 0.7f, 0.7f });
                bullets.push_back(b);
            }
            it = warnings.erase(it);
        }
        else {
            ++it;
        }
    }

    // 2. 更新实体弹幕
    for (auto it = bullets.begin(); it != bullets.end(); ) {
        it->sprite.move(it->velocity * dt);
        sf::Vector2f p = it->sprite.getPosition();
        // 增加判定范围，防止巨大弹幕瞬间消失
        if (p.x < -100 || p.x > 900 || p.y < -100 || p.y > 700) it = bullets.erase(it);
        else ++it;
    }
}

void Boss::fireBullet(float angleDeg, float speed, int bulletType) {
    const sf::Texture* currentTex = (bulletType == 0) ? bulletTexture01 : bulletTexture02;
    if (!currentTex) return;
    float angleRad = angleDeg * 3.14159f / 180.f;
    Bullet b(*currentTex, { std::cos(angleRad) * speed, std::sin(angleRad) * speed }, 0); // 普通弹幕 Type=0
    sf::Vector2u texSize = currentTex->getSize();
    b.sprite.setOrigin({ texSize.x / 2.0f, texSize.y / 2.0f });
    b.sprite.setPosition(pos);
    b.sprite.setRotation(sf::degrees(angleDeg));
    bullets.push_back(b);
}

void Boss::draw(sf::RenderWindow& window) {
    // 渲染底层警告线
    for (auto& w : warnings) window.draw(w.shape);

    if (state != BossState::Dead) {
        if (bgObjectSprite) window.draw(*bgObjectSprite);
        if (bossSprite) window.draw(*bossSprite);
        if (state != BossState::Spawning && state != BossState::Dying) {
            if (heartFillSprite) window.draw(*heartFillSprite);
            if (heartOutlineSprite) window.draw(*heartOutlineSprite);
            if (pctText) window.draw(*pctText);
        }
    }
    // 渲染顶层弹幕
    for (auto& b : bullets) window.draw(b.sprite);
}