#include "Boss.h"
#include <algorithm>

Boss::Boss() : hitboxRadius(20.f), health(100.f), maxHealth(100.f), currentPhase(0), state(BossState::Normal) {}

void Boss::init(Difficulty diff, Level lvl, const sf::Font& font,
    const sf::Texture* heartOut, const sf::Texture* heartFill,
    const sf::Texture* bulletTex01, const sf::Texture* bulletTex02,
    const BossConfig& cfg) {
    bullets.clear();
    warnings.clear();
    rotationAngle = 0.f;
    patternTimer = 0.f;
    specialTimer = 0.f;
    bubbleTimer = 0.f;
    targetAttackTimer = 0.f;
    currentPhase = 0;

    config = cfg;
    basePosX = config.startPosX;
    basePosY = config.startPosY;
    pos = { basePosX, basePosY };

    if (diff == Difficulty::Easy) maxHealth = 35.f;
    else if (diff == Difficulty::Normal) maxHealth = 75.f;
    else maxHealth = 125.f;
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
    if (state == BossState::Dying || state == BossState::Dead || state == BossState::Spawning || state == BossState::PhaseTransition || state == BossState::PhaseWait) return;

    health -= amount;
    if (health <= 0.f) {
        health = 0.f;
        if (currentPhase < 2) {
            state = BossState::PhaseWait;
            warnings.clear();
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

void Boss::advancePhase() {
    if (state == BossState::PhaseWait) {
        currentPhase++;
        health = maxHealth;
        state = BossState::PhaseTransition;
        stateTimer = static_cast<float>(config.animCast.totalFrames) / config.animCast.fps;
        setAnimation(config.animCast);
    }
}

void Boss::spawnWarning(sf::Vector2f startPos, sf::Vector2f size, sf::Vector2f vel, float rot, float maxTime, int type, bool attached) {
    WarningArea w;
    w.shape.setSize(size);
    w.shape.setOrigin({ size.x / 2.f, size.y / 2.f });
    w.shape.setPosition(startPos);
    w.shape.setRotation(sf::degrees(rot));
    w.timer = 0.f;
    w.maxTime = maxTime;
    w.bulletType = type;
    w.startPos = startPos;
    w.velocity = vel;
    w.rotation = rot;
    w.attachedToBoss = attached;
    warnings.push_back(w);
}

void Boss::update(float dt, Difficulty diff, Level lvl, sf::Vector2f playerPos, sf::Vector2f playerDir, sf::Vector2f playerBodyPos) {
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

    if (state == BossState::PhaseWait) {
        updateBullets(dt);
        return;
    }

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

    if (heartFillSprite && heartFillTex && pctText) {
        float pct = std::clamp(health / maxHealth, 0.f, 1.f);
        sf::Vector2u texSize = heartFillTex->getSize();
        int visibleH = static_cast<int>(texSize.y * pct);
        int topY = texSize.y - visibleH;
        heartFillSprite->setTextureRect(sf::IntRect({ 0, topY }, { static_cast<int>(texSize.x), visibleH }));
        heartFillSprite->setOrigin({ texSize.x / 2.0f, static_cast<float>(texSize.y / 2.0f - topY) });
        pctText->setString(std::to_string(static_cast<int>(health)) + " / " + std::to_string(static_cast<int>(maxHealth)));
        sf::FloatRect bounds = pctText->getLocalBounds();
        pctText->setOrigin({ bounds.size.x / 2.0f, bounds.size.y / 2.0f });
        pctText->setPosition({ pos.x, pos.y + 40.f });
    }

    patternTimer += dt;
    if (lvl == Level::Level2) pos.x = basePosX + std::sin(patternTimer * 1.5f) * 200.f;
    else pos.x = basePosX;
    pos.y = basePosY;

    if (bgObjectSprite) bgObjectSprite->setPosition(pos);
    if (bossSprite) bossSprite->setPosition(pos);
    if (heartOutlineSprite) heartOutlineSprite->setPosition(pos);
    if (heartFillSprite) heartFillSprite->setPosition(pos);

    float baseFireRate = (diff == Difficulty::Hard) ? 0.16f : ((diff == Difficulty::Normal) ? 0.24f : 0.36f);
    fireTimer += dt;

    if (fireTimer > baseFireRate) {
        fireTimer = 0.f;
        int attackMode = currentPhase;
        if (currentPhase == 0) attackMode = 2;
        else if (currentPhase == 1) attackMode = 0;
        else if (currentPhase == 2) attackMode = 1;

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
            if (static_cast<int>(patternTimer * 10) % 8 == 0) {
                int overrideB = config.forceBulletType != -1 ? config.forceBulletType : 1;
                for (int i = 0; i < 360; i += 30) fireBullet((float)i + rotationAngle, 180.f, overrideB);
            }
            rotationAngle += 5.f;
        }
    }

    specialTimer += dt;
    if (config.bossId == 1 && specialTimer > 10.0f) {
        specialTimer = 0.f;
        for (int i = 0; i < 3 + currentPhase; ++i) {
            float rx = 50.f + (std::rand() % 700);
            spawnWarning({ rx, -50.f }, { 1500.f, 40.f }, { 0.f, 500.f }, 90.f, 1.5f, 10);
        }
    }
    else if (config.bossId == 2 && specialTimer > 12.0f) {
        specialTimer = 0.f;
        bool fromLeft = (std::rand() % 2 == 0);
        float ry = 100.f + (std::rand() % 400);
        if (fromLeft) spawnWarning({ -100.f, ry }, { 2000.f, 80.f }, { 600.f, 0.f }, 0.f, 1.5f, 11);
        else spawnWarning({ 900.f, ry }, { 2000.f, 80.f }, { -600.f, 0.f }, 180.f, 1.5f, 11);
    }

    targetAttackTimer += dt;
    if (targetAttackTimer >= 12.0f) {
        targetAttackTimer = 0.f;
        int specialType = (config.bossId == 1) ? 10 : 11;

        sf::Vector2f dirVec = (playerDir.x == 0.f && playerDir.y == 0.f) ? sf::Vector2f(1.f, 0.f) : playerDir;
        sf::Vector2f targetFront = playerPos + dirVec * 200.f;
        sf::Vector2f targetBehind = playerPos - dirVec * 150.f;
        sf::Vector2f targetBody = playerBodyPos;

        auto spawnAimedWarning = [&](sf::Vector2f target) {
            sf::Vector2f dir = target - pos;
            float dist = std::hypot(dir.x, dir.y);
            if (dist > 0.1f) dir /= dist;
            float rot = std::atan2(dir.y, dir.x) * 180.f / 3.14159f;
            spawnWarning(pos, { 2500.f, 40.f }, dir * 650.f, rot, 1.5f, specialType, true);
            };

        spawnAimedWarning(targetFront);
        spawnAimedWarning(targetBehind);
        spawnAimedWarning(targetBody);
    }

    bubbleTimer += dt;
    if (bubbleTimer > 20.f) {
        bubbleTimer = 0.f;
        for (int i = 0; i < 2 + currentPhase; ++i) {
            int edge = std::rand() % 4;
            sf::Vector2f startP, endP;
            if (edge == 0) { startP = { -50.f, (float)(std::rand() % 600) }; endP = { 850.f, (float)(std::rand() % 600) }; }
            else if (edge == 1) { startP = { 850.f, (float)(std::rand() % 600) }; endP = { -50.f, (float)(std::rand() % 600) }; }
            else if (edge == 2) { startP = { (float)(std::rand() % 800), -50.f }; endP = { (float)(std::rand() % 800), 650.f }; }
            else { startP = { (float)(std::rand() % 800), 650.f }; endP = { (float)(std::rand() % 800), -50.f }; }

            sf::Vector2f dir = endP - startP;
            float dist = std::hypot(dir.x, dir.y);
            if (dist > 0.1f) dir /= dist;
            float rot = std::atan2(dir.y, dir.x) * 180.f / 3.14159f;
            // 【修改点】预警缩短到0.8s，速度加快到1800.f，泡泡变成极其可怕的瞬发攻击
            spawnWarning(startP, { 2500.f, 40.f }, dir * 1800.f, rot, 0.8f, 12);
        }
    }

    updateBullets(dt);
}

void Boss::updateBullets(float dt) {
    for (auto it = bullets.begin(); it != bullets.end(); ) {
        it->sprite.move(it->velocity * dt);
        sf::Vector2f p = it->sprite.getPosition();
        if (p.x < -100 || p.x > 900 || p.y < -100 || p.y > 700) it = bullets.erase(it);
        else ++it;
    }

    for (auto it = warnings.begin(); it != warnings.end(); ) {
        if (it->attachedToBoss) {
            it->startPos = pos;
            it->shape.setPosition(pos);
        }

        it->timer += dt;
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

                if (it->bulletType == 10 || it->bulletType == 11) {
                    b.sprite.setRotation(sf::degrees(it->rotation + 90.f));
                }
                else {
                    b.sprite.setRotation(sf::degrees(it->rotation));
                }
                bullets.push_back(b);
            }
            it = warnings.erase(it);
        }
        else {
            ++it;
        }
    }
}

void Boss::fireBullet(float angleDeg, float speed, int bulletType) {
    const sf::Texture* currentTex = (bulletType == 0) ? bulletTexture01 : bulletTexture02;
    if (!currentTex) return;
    float angleRad = angleDeg * 3.14159f / 180.f;
    Bullet b(*currentTex, { std::cos(angleRad) * speed, std::sin(angleRad) * speed }, bulletType);
    sf::Vector2u texSize = currentTex->getSize();
    b.sprite.setOrigin({ texSize.x / 2.0f, texSize.y / 2.0f });
    b.sprite.setPosition(pos);
    b.sprite.setRotation(sf::degrees(angleDeg));
    bullets.push_back(b);
}

void Boss::draw(sf::RenderWindow& window) {
    if (state != BossState::Dead && state != BossState::PhaseWait) {
        if (bgObjectSprite) window.draw(*bgObjectSprite);
        if (bossSprite) window.draw(*bossSprite);
        if (state != BossState::Spawning && state != BossState::Dying) {
            if (heartFillSprite) window.draw(*heartFillSprite);
            if (heartOutlineSprite) window.draw(*heartOutlineSprite);
            if (pctText) window.draw(*pctText);
        }
    }

    for (auto& w : warnings) {
        if (static_cast<int>(w.timer * 15) % 2 == 0) w.shape.setFillColor(sf::Color(255, 0, 0, 100));
        else w.shape.setFillColor(sf::Color(255, 0, 0, 50));
        window.draw(w.shape);
    }

    for (auto& b : bullets) window.draw(b.sprite);
}