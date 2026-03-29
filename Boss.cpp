#include "Boss.h"
#include <cmath>
#include <algorithm>

Boss::Boss() : hitboxRadius(20.f), health(100.f), maxHealth(100.f), currentPhase(0),
bulletTexture01(nullptr), bulletTexture02(nullptr), heartOutlineTex(nullptr), heartFillTex(nullptr),
rotationAngle(0.f), fireTimer(0.f), patternTimer(0.f),
state(BossState::Normal), stateTimer(0.f), baseScale(1.f, 1.f), currentFrame(0), animTimer(0.f) {
}

void Boss::init(Difficulty diff,
    const sf::Font& font,
    const sf::Texture* heartOut, const sf::Texture* heartFill,
    const sf::Texture* stayTex, const sf::Texture* castTex, const sf::Texture* sufferTex,
    const sf::Texture* bulletTex01, const sf::Texture* bulletTex02) {
    bullets.clear();
    rotationAngle = 0.f;
    patternTimer = 0.f;
    currentPhase = 0;
    pos = { 400.f, 200.f };

    if (diff == Difficulty::Easy) maxHealth = 150.f;
    else if (diff == Difficulty::Normal) maxHealth = 300.f;
    else maxHealth = 500.f;
    health = maxHealth;

    bulletTexture01 = bulletTex01;
    bulletTexture02 = bulletTex02;
    heartOutlineTex = heartOut;
    heartFillTex = heartFill;

    // 动画设定
    animStay = { stayTex, 8, 1, 8, 10.f };
    animCast = { castTex, 8, 3, 18, 12.f };
    animSuffer = { sufferTex, 8, 4, 28, 15.f };

    if (castTex) {
        bossSprite.emplace(*castTex);
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
    pctText->setStyle(sf::Text::Bold);     // 文本加粗
    pctText->setFillColor(sf::Color::Red); // 文本变红

    state = BossState::Spawning;
    stateTimer = 2.0f;
    setAnimation(animCast);
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
    int intFy = (currentFrame / currentAnim.cols) * h;
    bossSprite->setTextureRect(sf::IntRect({ fx, intFy }, { w, h }));
    bossSprite->setOrigin({ w / 2.0f, h / 2.0f });
}

void Boss::takeDamage(float amount) {
    if (state == BossState::Dying || state == BossState::Dead || state == BossState::Spawning || state == BossState::PhaseTransition) return;

    health -= amount;

    // 如果血量打空了，直接中断任何动画进入换阶段/死亡
    if (health <= 0.f) {
        health = 0.f;
        if (currentPhase < 2) {
            currentPhase++;
            health = maxHealth;
            state = BossState::PhaseTransition;
            stateTimer = 2.0f;
            bullets.clear();
            setAnimation(animCast);
        }
        else {
            state = BossState::Dying;
            stateTimer = 2.0f;
        }
        return;
    }

    // 如果当前不在受击状态，则切入受击状态；如果已经在播放了，就不重置时间，确保播完
    if (state != BossState::Hit) {
        state = BossState::Hit;
        // 动态计算受击动画播完需要的时间：总帧数 / 帧率 (28 / 15 ≈ 1.86秒)
        stateTimer = static_cast<float>(animSuffer.totalFrames) / animSuffer.fps;
        setAnimation(animSuffer);
    }
}

void Boss::update(float dt, Difficulty diff, Level lvl) {
    if (currentAnim.tex) {
        animTimer += dt;
        float frameTime = 1.0f / currentAnim.fps;
        if (animTimer >= frameTime) {
            animTimer -= frameTime;
            currentFrame = (currentFrame + 1) % currentAnim.totalFrames;
            updateSpriteRect();
        }
    }

    if (state == BossState::Dying) {
        stateTimer -= dt;
        if (stateTimer <= 0.f) state = BossState::Dead;
        else if (bossSprite) {
            bossSprite->rotate(sf::degrees(500.f * dt));
            float scale = std::max(0.f, stateTimer / 2.0f);
            bossSprite->setScale({ scale, scale });
            bossSprite->setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(scale * 255)));
        }
        updateBullets(dt);
        return;
    }

    if (state == BossState::Dead) return;

    if (state == BossState::Spawning || state == BossState::PhaseTransition) {
        stateTimer -= dt;
        if (heartOutlineSprite) {
            heartOutlineSprite->setColor(static_cast<int>(stateTimer * 10) % 2 == 0 ? sf::Color(255, 255, 255, 100) : sf::Color::White);
        }
        if (stateTimer <= 0.f) {
            state = BossState::Normal;
            if (heartOutlineSprite) heartOutlineSprite->setColor(sf::Color::White);
            setAnimation(animStay);
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
            setAnimation(animStay);
        }
    }

    // 修复：加入 pctText 的指针检查
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
    if (lvl == Level::Level2) {
        pos.x = 400.f + std::sin(patternTimer * 1.5f) * 200.f;
    }

    if (bossSprite) bossSprite->setPosition(pos);
    if (heartOutlineSprite) heartOutlineSprite->setPosition(pos);
    if (heartFillSprite) heartFillSprite->setPosition(pos);

    float baseFireRate = (diff == Difficulty::Hard) ? 0.16f : ((diff == Difficulty::Normal) ? 0.24f : 0.36f);
    fireTimer += dt;

    if (fireTimer > baseFireRate) {
        fireTimer = 0.f;
        if (currentPhase == 0) {
            rotationAngle += 18.f;
            for (int i : {0, 90, 180, 270}) fireBullet(rotationAngle + (float)i, 220.f, 0);
        }
        else if (currentPhase == 1) {
            rotationAngle += 35.f;
            fireBullet(rotationAngle, 280.f, 0);
            fireBullet(rotationAngle + 180.f, 280.f, 0);
        }
        else if (currentPhase == 2) {
            if (static_cast<int>(patternTimer * 10) % 8 == 0) {
                for (int i = 0; i < 360; i += 30) fireBullet((float)i + rotationAngle, 180.f, 1);
            }
            rotationAngle += 5.f;
        }
    }

    updateBullets(dt);
}

void Boss::updateBullets(float dt) {
    for (auto it = bullets.begin(); it != bullets.end(); ) {
        it->sprite.move(it->velocity * dt);
        sf::Vector2f p = it->sprite.getPosition();
        if (p.x < -30 || p.x > 830 || p.y < -30 || p.y > 630) it = bullets.erase(it);
        else ++it;
    }
}

void Boss::fireBullet(float angleDeg, float speed, int bulletType) {
    const sf::Texture* currentTex = (bulletType == 0) ? bulletTexture01 : bulletTexture02;
    if (!currentTex) return;

    float angleRad = angleDeg * 3.14159f / 180.f;
    sf::Vector2f vel = { std::cos(angleRad) * speed, std::sin(angleRad) * speed };
    Bullet b(*currentTex, vel);
    sf::Vector2u texSize = currentTex->getSize();
    b.sprite.setOrigin({ texSize.x / 2.0f, texSize.y / 2.0f });
    b.sprite.setPosition(pos);
    b.sprite.setRotation(sf::degrees(angleDeg));
    bullets.push_back(b);
}

void Boss::draw(sf::RenderWindow& window) {
    if (state != BossState::Dead) {
        if (bossSprite) window.draw(*bossSprite);

        if (state != BossState::Spawning && state != BossState::Dying) {
            if (heartFillSprite) window.draw(*heartFillSprite);
            if (heartOutlineSprite) window.draw(*heartOutlineSprite);
            // 修复：加入 pctText 的指针检查
            if (pctText) window.draw(*pctText);
        }
    }
    for (auto& b : bullets) window.draw(b.sprite);
}