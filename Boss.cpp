#include "Boss.h"
#include <cmath>

Boss::Boss() : hitboxRadius(35.f), health(100.f), maxHealth(100.f),
bulletTexture01(nullptr), bulletTexture02(nullptr), rotationAngle(0.f),
fireTimer(0.f), patternTimer(0.f), currentPattern(0),
state(BossState::Normal), hitTimer(0.f), deathTimer(0.f), baseScale(1.f, 1.f) {
}

void Boss::init(Difficulty diff, const sf::Texture* bossTex, const sf::Texture* bulletTex01, const sf::Texture* bulletTex02) {
    bullets.clear();
    rotationAngle = 0.f;
    patternTimer = 0.f;
    currentPattern = 0;
    pos = { 400.f, 200.f };
    state = BossState::Normal;
    hitTimer = 0.f;
    deathTimer = 0.f;

    if (diff == Difficulty::Easy) maxHealth = 150.f;
    else if (diff == Difficulty::Normal) maxHealth = 300.f;
    else maxHealth = 500.f;
    health = maxHealth;

    // 绑定两张子弹贴图
    bulletTexture01 = bulletTex01;
    bulletTexture02 = bulletTex02;

    if (bossTex) {
        bossSprite.emplace(*bossTex);
        sf::Vector2u texSize = bossTex->getSize();
        bossSprite->setOrigin({ texSize.x / 2.0f, texSize.y / 2.0f });
        bossSprite->setPosition(pos);
        bossSprite->setScale({ 1.f, 1.f });
        bossSprite->setColor(sf::Color::White);
    }
    else {
        bossSprite.reset();
    }
}

void Boss::takeDamage(float amount) {
    if (state == BossState::Dying || state == BossState::Dead) return;

    health -= amount;
    if (health <= 0.f) {
        health = 0.f;
        state = BossState::Dying;
        deathTimer = 2.0f;
    }
    else {
        state = BossState::Hit;
        hitTimer = 0.15f;
    }
}

void Boss::update(float dt, Difficulty diff, Level lvl) {
    if (state == BossState::Dying) {
        deathTimer -= dt;
        if (deathTimer <= 0.f) {
            state = BossState::Dead;
        }
        else if (bossSprite) {
            bossSprite->rotate(sf::degrees(500.f * dt));
            float scale = std::max(0.f, deathTimer / 2.0f);
            bossSprite->setScale({ scale, scale });
            bossSprite->setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(scale * 255)));
        }
        updateBullets(dt);
        return;
    }

    if (state == BossState::Dead) return;

    if (state == BossState::Hit) {
        hitTimer -= dt;
        if (bossSprite) bossSprite->setColor(sf::Color::White);
        if (hitTimer <= 0.f) {
            state = BossState::Normal;
        }
    }

    patternTimer += dt;
    if (patternTimer > 7.0f) {
        currentPattern = (currentPattern + 1) % 3;
        patternTimer = 0.f;
    }

    if (lvl == Level::Level2) {
        pos.x = 400.f + std::sin(patternTimer * 1.5f) * 200.f;
    }

    if (bossSprite) bossSprite->setPosition(pos);

    if (state == BossState::Normal && bossSprite) {
        if (currentPattern == 0) bossSprite->setColor(sf::Color(255, 150, 150));
        else if (currentPattern == 1) bossSprite->setColor(sf::Color(255, 200, 150));
        else bossSprite->setColor(sf::Color(200, 150, 255));
    }

    // 降低射速：间隔时间翻倍
    float baseFireRate = (diff == Difficulty::Hard) ? 0.16f : ((diff == Difficulty::Normal) ? 0.24f : 0.36f);
    fireTimer += dt;

    if (fireTimer > baseFireRate) {
        fireTimer = 0.f;
        if (currentPattern == 0) {
            rotationAngle += 18.f;
            for (int i : {0, 90, 180, 270}) fireBullet(rotationAngle + (float)i, 220.f, 0); // 用贴图0
        }
        else if (currentPattern == 1) {
            rotationAngle += 35.f;
            fireBullet(rotationAngle, 280.f, 0); // 用贴图0
            fireBullet(rotationAngle + 180.f, 280.f, 0);
        }
        else if (currentPattern == 2) {
            if (static_cast<int>(patternTimer * 10) % 8 == 0) {
                for (int i = 0; i < 360; i += 30) fireBullet((float)i + rotationAngle, 180.f, 1); // 用贴图1
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
        if (p.x < -30 || p.x > 830 || p.y < -30 || p.y > 630) {
            it = bullets.erase(it);
        }
        else {
            ++it;
        }
    }
}

void Boss::fireBullet(float angleDeg, float speed, int bulletType) {
    // 动态选择贴图
    const sf::Texture* currentTex = (bulletType == 0) ? bulletTexture01 : bulletTexture02;
    if (!currentTex) return;

    float angleRad = angleDeg * 3.14159f / 180.f;
    sf::Vector2f vel = { std::cos(angleRad) * speed, std::sin(angleRad) * speed };

    Bullet b(*currentTex, vel);
    sf::Vector2u texSize = currentTex->getSize();

    b.sprite.setOrigin({ texSize.x / 2.0f, texSize.y / 2.0f });
    b.sprite.setPosition(pos);

    // 子弹精准朝向飞行方向 (基于 SFML 默认 0度为向右)
    // 如果你的导弹原图是向上的，改为 angleDeg + 90.f
    b.sprite.setRotation(sf::degrees(angleDeg));

    bullets.push_back(b);
}

void Boss::draw(sf::RenderWindow& window) {
    if (state != BossState::Dead && bossSprite) {
        window.draw(*bossSprite);
    }
    for (auto& b : bullets) {
        window.draw(b.sprite);
    }
}