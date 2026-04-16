#include "Game.h"
#include <iostream>
#include <cmath>
#include <algorithm>

Game::Game() : window(sf::VideoMode({ 1600, 1200 }), "Last Command - Modded"), state(GameState::Menu), menuSelection(0), unlockedLevels(3), currentSkinIndex(0), bgStyle(0), spawnTimer(0.f), healSpawnTimer(0.f) {
    window.setFramerateLimit(60);
    sf::View view({ 400.f, 300.f }, { 800.f, 600.f });
    window.setView(view);
    loadResources();
    player.initSprites(snakeHeadTex, snakeBodyTexs[currentSkinIndex]);
}

void Game::loadResources() {
    (void)font.openFromFile("Cubic.ttf");
    (void)bulletTex01.loadFromFile("assets/bulletBoss01.png");
    (void)bulletTex02.loadFromFile("assets/bulletBoss02.png");
    (void)snakeHeadTex.loadFromFile("assets/snakeSkinHead.png");

    snakeBodyTexs.resize(5);
    for (int i = 0; i < 5; ++i) {
        (void)snakeBodyTexs[i].loadFromFile("assets/snakeSkinBody0" + std::to_string(i + 1) + ".png");
    }
    (void)snakeAttackDotTex.loadFromFile("assets/snakeAttackDot.png");

    (void)heartOutTex.loadFromFile("assets/levelHeart20x22OutlineForHonest.png");
    (void)heartFillTex.loadFromFile("assets/levelHeart20x22.png");

    (void)boss1Stay.loadFromFile("assets/BossHonest_Stay.png");
    (void)boss1Cast.loadFromFile("assets/BossHonest_Cast01.png");
    (void)boss1Suffer.loadFromFile("assets/BossHonest_Suffer.png");

    (void)boss2Stay.loadFromFile("assets/BossHime_Stay.png");
    (void)boss2Cast.loadFromFile("assets/BossHime_Start.png");
    (void)boss2Suffer.loadFromFile("assets/BossHime_Suffer.png");
    (void)boss2End.loadFromFile("assets/BossHime_End.png");
    (void)boss2BG.loadFromFile("assets/BossHime_BG_OBJECT.png");

    (void)honestSpecialTex.loadFromFile("assets/BossHonest_Bullet.png");
    (void)himeSpecialTex.loadFromFile("assets/BossHime_Bullet.png");
    (void)bubbleTex.loadFromFile("assets/Boss_bubbleSmall.png");

    (void)previewHimeTex.loadFromFile("assets/npcSpineCalculationHime.png");
    (void)previewHonestTex.loadFromFile("assets/Honest_idle.png");

    recommendText.emplace(font);
    recommendText->setCharacterSize(18);
    recommendText->setFillColor(sf::Color(255, 255, 255, 180));
    recommendText->setString(L"推荐游玩本作");
    centerText(*recommendText, 550.f);
}

void Game::startLevel() {
    player.reset();
    player.initSprites(snakeHeadTex, snakeBodyTexs[currentSkinIndex]);
    dataPoints.clear();
    shockwaves.clear();
    snakeProjectiles.clear();
    stats = GameStats();

    bossSwapTimer = 0.f;
    isSwapping = false;
    spawnTimer = 0.f;
    healSpawnTimer = 0.f;

    if (currentLevel == Level::Level1) {
        BossConfig cfg;
        cfg.bossId = 1;
        cfg.stayTex = &boss1Stay; cfg.castTex = &boss1Cast; cfg.sufferTex = &boss1Suffer; cfg.endTex = &boss1Suffer;
        cfg.bgObjectTex = nullptr;
        cfg.texHonestSpecial = &honestSpecialTex; cfg.texHimeSpecial = &himeSpecialTex; cfg.texBubble = &bubbleTex;
        cfg.animStay = { &boss1Stay, 8, 1, 8, 10.f };
        cfg.animCast = { &boss1Cast, 8, 3, 18, 10.f };
        cfg.animSuffer = { &boss1Suffer, 8, 4, 28, 10.f };
        cfg.animEnd = cfg.animSuffer;
        cfg.startPosX = 400.f; cfg.startPosY = 200.f;
        cfg.forceBulletType = -1;
        boss.init(currentDiff, currentLevel, font, &heartOutTex, &heartFillTex, &bulletTex01, &bulletTex02, cfg);
        boss2.kill();
    }
    else if (currentLevel == Level::Level2) {
        BossConfig cfg;
        cfg.bossId = 2;
        cfg.stayTex = &boss2Stay; cfg.castTex = &boss2Cast; cfg.sufferTex = &boss2Suffer; cfg.endTex = &boss2End;
        cfg.bgObjectTex = &boss2BG;
        cfg.texHonestSpecial = &honestSpecialTex; cfg.texHimeSpecial = &himeSpecialTex; cfg.texBubble = &bubbleTex;
        cfg.animStay = { &boss2Stay, 8, 1, 8, 10.f };
        cfg.animCast = { &boss2Cast, 6, 2, 11, 10.f };
        cfg.animSuffer = { &boss2Suffer, 6, 2, 11, 10.f };
        cfg.animEnd = { &boss2End, 6, 2, 12, 10.f };
        cfg.startPosX = 400.f; cfg.startPosY = 200.f;
        cfg.forceBulletType = -1;
        boss.init(currentDiff, currentLevel, font, &heartOutTex, &heartFillTex, &bulletTex01, &bulletTex02, cfg);
        boss2.kill();
    }
    else if (currentLevel == Level::Level3) {
        BossConfig cfgHime;
        cfgHime.bossId = 2;
        cfgHime.stayTex = &boss2Stay; cfgHime.castTex = &boss2Cast; cfgHime.sufferTex = &boss2Suffer; cfgHime.endTex = &boss2End;
        cfgHime.bgObjectTex = &boss2BG;
        cfgHime.texHonestSpecial = &honestSpecialTex; cfgHime.texHimeSpecial = &himeSpecialTex; cfgHime.texBubble = &bubbleTex;
        cfgHime.animStay = { &boss2Stay, 8, 1, 8, 10.f };
        cfgHime.animCast = { &boss2Cast, 6, 2, 11, 10.f };
        cfgHime.animSuffer = { &boss2Suffer, 6, 2, 11, 10.f };
        cfgHime.animEnd = { &boss2End, 6, 2, 12, 10.f };
        cfgHime.startPosX = 600.f;
        cfgHime.startPosY = 200.f;
        cfgHime.forceBulletType = 1;
        boss.init(currentDiff, Level::Level3, font, &heartOutTex, &heartFillTex, &bulletTex01, &bulletTex02, cfgHime);

        BossConfig cfgHonest;
        cfgHonest.bossId = 1;
        cfgHonest.stayTex = &boss1Stay; cfgHonest.castTex = &boss1Cast; cfgHonest.sufferTex = &boss1Suffer; cfgHonest.endTex = &boss1Suffer;
        cfgHonest.bgObjectTex = nullptr;
        cfgHonest.texHonestSpecial = &honestSpecialTex; cfgHonest.texHimeSpecial = &himeSpecialTex; cfgHonest.texBubble = &bubbleTex;
        cfgHonest.animStay = { &boss1Stay, 8, 1, 8, 10.f };
        cfgHonest.animCast = { &boss1Cast, 8, 3, 18, 10.f };
        cfgHonest.animSuffer = { &boss1Suffer, 8, 4, 28, 10.f };
        cfgHonest.animEnd = cfgHonest.animSuffer;
        cfgHonest.startPosX = 200.f;
        cfgHonest.startPosY = 200.f;
        cfgHonest.forceBulletType = 0;
        boss2.init(currentDiff, Level::Level3, font, &heartOutTex, &heartFillTex, &bulletTex01, &bulletTex02, cfgHonest);
    }
    state = GameState::Playing;
}

void Game::centerText(sf::Text& text, float y) {
    sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin({ bounds.size.x / 2.0f, bounds.size.y / 2.0f });
    text.setPosition({ 400.f, y });
}

void Game::processEvents() {
    while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) window.close();

        if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
            if (state == GameState::Menu) {
                if (keyEvent->code == sf::Keyboard::Key::Up) menuSelection = std::max(0, menuSelection - 1);
                if (keyEvent->code == sf::Keyboard::Key::Down) menuSelection = std::min(3, menuSelection + 1);
                if (keyEvent->code == sf::Keyboard::Key::Enter) {
                    if (menuSelection == 0) state = GameState::LevelSelect;
                    else if (menuSelection == 1) state = GameState::SkinSelect;
                    else if (menuSelection == 2) state = GameState::Settings;
                    else window.close();
                    menuSelection = 0;
                }
            }
            else if (state == GameState::SkinSelect) {
                if (keyEvent->code == sf::Keyboard::Key::Left) currentSkinIndex = std::max(0, currentSkinIndex - 1);
                if (keyEvent->code == sf::Keyboard::Key::Right) currentSkinIndex = std::min(4, currentSkinIndex + 1);
                if (keyEvent->code == sf::Keyboard::Key::Enter || keyEvent->code == sf::Keyboard::Key::Escape) {
                    state = GameState::Menu;
                    menuSelection = 0;
                }
            }
            else if (state == GameState::Settings) {
                if (keyEvent->code == sf::Keyboard::Key::Left) bgStyle = std::max(0, bgStyle - 1);
                if (keyEvent->code == sf::Keyboard::Key::Right) bgStyle = std::min(3, bgStyle + 1);
                if (keyEvent->code == sf::Keyboard::Key::Enter || keyEvent->code == sf::Keyboard::Key::Escape) {
                    state = GameState::Menu;
                    menuSelection = 0;
                }
            }
            else if (state == GameState::LevelSelect) {
                if (keyEvent->code == sf::Keyboard::Key::Up) menuSelection = std::max(0, menuSelection - 1);
                if (keyEvent->code == sf::Keyboard::Key::Down) menuSelection = std::min(2, menuSelection + 1);
                if (keyEvent->code == sf::Keyboard::Key::Escape) state = GameState::Menu;
                if (keyEvent->code == sf::Keyboard::Key::Enter) {
                    currentLevel = static_cast<Level>(menuSelection);
                    state = GameState::DiffSelect;
                    menuSelection = 0;
                }
            }
            else if (state == GameState::DiffSelect) {
                if (keyEvent->code == sf::Keyboard::Key::Up) menuSelection = std::max(0, menuSelection - 1);
                if (keyEvent->code == sf::Keyboard::Key::Down) menuSelection = std::min(2, menuSelection + 1);
                if (keyEvent->code == sf::Keyboard::Key::Escape) state = GameState::LevelSelect;
                if (keyEvent->code == sf::Keyboard::Key::Enter) {
                    currentDiff = static_cast<Difficulty>(menuSelection);
                    startLevel();
                }
            }
            else if (state == GameState::Playing) {
                if (keyEvent->code == sf::Keyboard::Key::Escape) { state = GameState::Paused; menuSelection = 0; }
                if (keyEvent->code == sf::Keyboard::Key::Q && player.energy >= 35.f) {
                    player.energy -= 35.f;
                    shockwaves.push_back(Shockwave(player.headPos));
                }
                if ((keyEvent->code == sf::Keyboard::Key::LShift || keyEvent->code == sf::Keyboard::Key::F) && player.dashCharges > 0 && !player.isDashing) {
                    player.dashCharges--;
                    player.isDashing = true;
                    player.dashTimer = 0.15f;
                    player.isInvincible = true;
                    player.invTimer = 0.40f;
                }
                if (keyEvent->code == sf::Keyboard::Key::F1) { boss.takeDamage(boss.getMaxHealth()); boss2.takeDamage(boss2.getMaxHealth()); }
            }
            else if (state == GameState::Paused) {
                if (keyEvent->code == sf::Keyboard::Key::Up) menuSelection = std::max(0, menuSelection - 1);
                if (keyEvent->code == sf::Keyboard::Key::Down) menuSelection = std::min(2, menuSelection + 1);
                if (keyEvent->code == sf::Keyboard::Key::Enter) {
                    if (menuSelection == 0) state = GameState::Playing;
                    else if (menuSelection == 1) state = GameState::Menu;
                    else if (menuSelection == 2) { boss.kill(); boss2.kill(); state = GameState::Win; }
                }
            }
            else if (state == GameState::GameOver || state == GameState::Win) {
                if (keyEvent->code == sf::Keyboard::Key::Enter) state = GameState::Menu;
            }
        }
    }
}

void Game::update(float dt) {
    // 【修改点】让时间在 Settings 和 Menu 界面也略微流动，以驱动背景动画
    if (state == GameState::Menu || state == GameState::LevelSelect || state == GameState::Settings) {
        stats.timeElapsed += dt; // 驱动背景滚动
        honestPreviewTimer += dt;
        if (honestPreviewTimer >= 1.0f / 3.0f) {
            honestPreviewTimer -= 1.0f / 3.0f;
            honestPreviewFrame = (honestPreviewFrame + 1) % 4;
        }
        himePreviewTimer += dt;
        if (himePreviewTimer >= 1.0f / 3.0f) {
            himePreviewTimer -= 1.0f / 3.0f;
            himePreviewFrame = (himePreviewFrame + 1) % 4;
        }
    }

    if (state != GameState::Playing) return;

    stats.timeElapsed += dt;
    feelManager.update(dt);

    if (feelManager.isFrozen()) return;

    if (currentLevel == Level::Level3) {
        bool b1Wait = (boss.getState() == BossState::PhaseWait || boss.getState() == BossState::Dead);
        bool b2Wait = (boss2.getState() == BossState::PhaseWait || boss2.getState() == BossState::Dead);

        if (b1Wait && b2Wait) {
            if (boss.getState() == BossState::PhaseWait) boss.advancePhase();
            if (boss2.getState() == BossState::PhaseWait) boss2.advancePhase();
        }
    }
    else {
        if (boss.getState() == BossState::PhaseWait) boss.advancePhase();
    }

    if (!boss.isDead() && !boss.isDying()) {
        if (dataPoints.empty()) {
            spawnTimer += dt;
            if (spawnTimer > 1.2f) {
                sf::Vector2f spawnPos;
                do {
                    spawnPos = { (float)(rand() % 700 + 50), (float)(rand() % 400 + 100) };
                } while (std::hypot(spawnPos.x - boss.getPosition().x, spawnPos.y - boss.getPosition().y) < 120.f);
                dataPoints.push_back(DataPoint(spawnPos, 0));
                spawnTimer = 0.f;
            }
        }

        bool hasHeal = false;
        for (const auto& dp : dataPoints) {
            if (dp.type == 1) { hasHeal = true; break; }
        }

        if (!hasHeal) {
            healSpawnTimer += dt;
            if (healSpawnTimer >= 10.0f) {
                healSpawnTimer = 0.0f;
                sf::Vector2f spawnPos;
                do {
                    spawnPos = { (float)(rand() % 700 + 50), (float)(rand() % 400 + 100) };
                } while (std::hypot(spawnPos.x - boss.getPosition().x, spawnPos.y - boss.getPosition().y) < 120.f);
                dataPoints.push_back(DataPoint(spawnPos, 1));
            }
        }
    }

    if (player.update(dt, sf::Vector2u(800, 600))) {
        float damage = 10.f + player.bodyCount * 8.f;
        snakeProjectiles.push_back(SnakeProjectile(snakeAttackDotTex, player.headPos, damage));
    }

    sf::Vector2f playerBodyPos = player.headPos;
    if (!player.trail.empty()) {
        playerBodyPos = player.trail[player.trail.size() / 2].position;
    }

    if (currentLevel == Level::Level3) {
        int combinedPhase = std::max(boss.getCurrentPhase(), boss2.getCurrentPhase());

        if (combinedPhase == 0) {
            if (!isSwapping && !boss.isDead() && !boss2.isDead()) {
                bossSwapTimer += dt;
                if (bossSwapTimer >= 15.f) {
                    bossSwapTimer -= 15.f;
                    isSwapping = true;
                    swapProgress = 0.f;
                    himeStartPos = { boss.getBasePosX(), boss.getBasePosY() };
                    honestStartPos = { boss2.getBasePosX(), boss2.getBasePosY() };
                    himeTargetPos = honestStartPos;
                    honestTargetPos = himeStartPos;
                }
            }
            else if (isSwapping) {
                swapProgress += dt / 2.0f;
                if (swapProgress >= 1.0f) { swapProgress = 1.0f; isSwapping = false; }
                float t = 0.5f * (1.f - std::cos(3.14159f * swapProgress));
                boss.setBasePosX(himeStartPos.x + (himeTargetPos.x - himeStartPos.x) * t);
                boss.setBasePosY(himeStartPos.y + (himeTargetPos.y - himeStartPos.y) * t);
                boss2.setBasePosX(honestStartPos.x + (honestTargetPos.x - honestStartPos.x) * t);
                boss2.setBasePosY(honestStartPos.y + (honestTargetPos.y - honestStartPos.y) * t);
            }
        }
        else if (combinedPhase == 1) {
            isSwapping = false;
            boss.setBasePosX(200.f);
            boss2.setBasePosX(600.f);

            float t = std::fmod(stats.timeElapsed, 10.f);
            if (t < 4.0f) {
                float moveY = std::sin(t * (3.14159f / 2.f)) * 150.f;
                boss.setBasePosY(300.f + moveY);
                boss2.setBasePosY(300.f - moveY);
            }
            else {
                boss.setBasePosY(300.f);
                boss2.setBasePosY(300.f);
            }
        }
        else if (combinedPhase >= 2) {
            isSwapping = false;

            float cycleTime = 24.0f;
            float t = std::fmod(stats.timeElapsed, cycleTime);

            sf::Vector2f b1_d1(150.f, 100.f);
            sf::Vector2f b1_d2(150.f, 500.f);
            sf::Vector2f b2_d1(650.f, 500.f);
            sf::Vector2f b2_d2(650.f, 100.f);

            if (t < 8.0f) {
                boss.setBasePosX(b1_d1.x); boss.setBasePosY(b1_d1.y);
                boss2.setBasePosX(b2_d1.x); boss2.setBasePosY(b2_d1.y);
            }
            else if (t < 12.0f) {
                float progress = (t - 8.0f) / 4.0f;
                progress = progress * progress * (3.f - 2.f * progress);
                boss.setBasePosX(b1_d1.x + (b1_d2.x - b1_d1.x) * progress);
                boss.setBasePosY(b1_d1.y + (b1_d2.y - b1_d1.y) * progress);
                boss2.setBasePosX(b2_d1.x + (b2_d2.x - b2_d1.x) * progress);
                boss2.setBasePosY(b2_d1.y + (b2_d2.y - b2_d1.y) * progress);
            }
            else if (t < 20.0f) {
                boss.setBasePosX(b1_d2.x); boss.setBasePosY(b1_d2.y);
                boss2.setBasePosX(b2_d2.x); boss2.setBasePosY(b2_d2.y);
            }
            else {
                float progress = (t - 20.0f) / 4.0f;
                progress = progress * progress * (3.f - 2.f * progress);
                boss.setBasePosX(b1_d2.x + (b1_d1.x - b1_d2.x) * progress);
                boss.setBasePosY(b1_d2.y + (b1_d1.y - b1_d2.y) * progress);
                boss2.setBasePosX(b2_d2.x + (b2_d1.x - b2_d2.x) * progress);
                boss2.setBasePosY(b2_d2.y + (b2_d1.y - b2_d2.y) * progress);
            }
        }

        boss.update(dt, currentDiff, currentLevel, player.headPos, player.currentDir, playerBodyPos);
        boss2.update(dt, currentDiff, currentLevel, player.headPos, player.currentDir, playerBodyPos);
    }
    else {
        boss.update(dt, currentDiff, currentLevel, player.headPos, player.currentDir, playerBodyPos);
    }

    stats.maxLength = std::max(stats.maxLength, player.bodyCount);

    for (auto& sw : shockwaves) sw.update(dt);
    (void)std::erase_if(shockwaves, [](const Shockwave& sw) { return !sw.isAlive; });

    for (auto& dp : dataPoints) dp.update(dt);

    (void)std::erase_if(dataPoints, [&](const DataPoint& dp) {
        if (dp.isFading && dp.timer >= dp.maxLifetime) return true;

        if (std::hypot(player.headPos.x - dp.shape.getPosition().x, player.headPos.y - dp.shape.getPosition().y) < 20.f) {
            if (dp.type == 0 || dp.type == 2) {
                if (player.bodyCount < player.maxBody) player.bodyCount++;
            }
            else if (dp.type == 1) {
                player.health = std::min(player.health + 1, 5);
                healSpawnTimer = 0.f;
            }
            return true;
        }
        return false;
        });

    std::vector<Bullet>& bossBullets = boss.getBullets();
    std::vector<Bullet>& bossBullets2 = boss2.getBullets();

    for (const auto& sw : shockwaves) {
        (void)std::erase_if(bossBullets, [&](const Bullet& b) { return std::hypot(sw.shape.getPosition().x - b.sprite.getPosition().x, sw.shape.getPosition().y - b.sprite.getPosition().y) < sw.radius; });
        (void)std::erase_if(bossBullets2, [&](const Bullet& b) { return std::hypot(sw.shape.getPosition().x - b.sprite.getPosition().x, sw.shape.getPosition().y - b.sprite.getPosition().y) < sw.radius; });
    }

    (void)std::erase_if(snakeProjectiles, [&](SnakeProjectile& p) {
        if (boss.getState() != BossState::Dead && boss.getState() != BossState::PhaseWait && boss.getState() != BossState::Dying) p.update(dt, boss.getPosition());
        else if (currentLevel == Level::Level3 && boss2.getState() != BossState::Dead && boss2.getState() != BossState::PhaseWait && boss2.getState() != BossState::Dying) p.update(dt, boss2.getPosition());
        else p.vel = { 0.f, -400.f };

        if (boss.getState() != BossState::Dead && boss.getState() != BossState::PhaseWait && boss.getState() != BossState::Dying && std::hypot(p.pos.x - boss.getPosition().x, p.pos.y - boss.getPosition().y) < boss.getHitboxRadius()) {
            boss.takeDamage(p.damage); feelManager.triggerHeavyHit(); return true;
        }
        if (currentLevel == Level::Level3 && boss2.getState() != BossState::Dead && boss2.getState() != BossState::PhaseWait && boss2.getState() != BossState::Dying && std::hypot(p.pos.x - boss2.getPosition().x, p.pos.y - boss2.getPosition().y) < boss2.getHitboxRadius()) {
            boss2.takeDamage(p.damage); feelManager.triggerHeavyHit(); return true;
        }
        return false;
        });

    auto playerHit = [&](int dmg) {
        int extraData = player.bodyCount - 2;
        player.takeDamage(dmg);
        shockwaves.push_back(Shockwave(player.headPos));
        feelManager.triggerLightHit();

        if (extraData > 0) {
            int dropCount = static_cast<int>(std::ceil(extraData * 0.6f));
            for (int i = 0; i < dropCount; ++i) {
                float angle = (std::rand() % 360) * 3.14159f / 180.f;
                float dist = 40.f + (std::rand() % 60);
                sf::Vector2f spawnPos = player.headPos + sf::Vector2f(std::cos(angle) * dist, std::sin(angle) * dist);
                spawnPos.x = std::clamp(spawnPos.x, 20.f, 780.f);
                spawnPos.y = std::clamp(spawnPos.y, 20.f, 580.f);
                dataPoints.push_back(DataPoint(spawnPos, 2));
            }
        }
        };

    if (!player.isInvincible) {
        for (auto it = bossBullets.begin(); it != bossBullets.end(); ) {
            float hitRadius = (it->type == 10 || it->type == 11) ? 35.f : ((it->type == 12) ? 25.f : 12.f);
            int dmg = 1;

            if (std::hypot(player.headPos.x - it->sprite.getPosition().x, player.headPos.y - it->sprite.getPosition().y) < hitRadius) {
                playerHit(dmg);
                it = bossBullets.erase(it);
                break;
            }
            else ++it;
        }
        if (!player.isInvincible) {
            for (auto it = bossBullets2.begin(); it != bossBullets2.end(); ) {
                float hitRadius = (it->type == 10 || it->type == 11) ? 35.f : ((it->type == 12) ? 25.f : 12.f);
                int dmg = 1;

                if (std::hypot(player.headPos.x - it->sprite.getPosition().x, player.headPos.y - it->sprite.getPosition().y) < hitRadius) {
                    playerHit(dmg);
                    it = bossBullets2.erase(it);
                    break;
                }
                else ++it;
            }
        }
    }

    if (player.health <= 0) state = GameState::GameOver;

    bool b1_dead = boss.isDead();
    bool b2_dead = (currentLevel == Level::Level3) ? boss2.isDead() : true;
    if (b1_dead && b2_dead) state = GameState::Win;
}

void Game::render() {
    window.clear(sf::Color(15, 15, 20));

    // --- 【修改点】在 Settings 界面也渲染背景 ---
    if (state == GameState::Playing || state == GameState::Paused || state == GameState::GameOver || state == GameState::Win || state == GameState::Settings) {
        if (bgStyle == 0) {
            sf::RectangleShape hLine({ 800.f, 2.f });
            hLine.setFillColor(sf::Color(0, 255, 255, 12));
            sf::RectangleShape vLine({ 2.f, 600.f });
            vLine.setFillColor(sf::Color(0, 255, 255, 12));
            float scrollOffset = std::fmod(stats.timeElapsed * 40.f, 40.f);
            for (float y = scrollOffset - 40.f; y < 600.f; y += 40.f) {
                hLine.setPosition({ 0.f, y });
                window.draw(hLine);
            }
            for (float x = scrollOffset - 40.f; x < 800.f; x += 40.f) {
                vLine.setPosition({ x, 0.f });
                window.draw(vLine);
            }
        }
        else if (bgStyle == 1) {
            for (int i = 0; i < 100; ++i) {
                float x = std::fmod(std::sin(i * 12.3f) * 4321.f, 800.f);
                if (x < 0) x += 800.f;
                float speed = 50.f + std::fmod(std::cos(i * 3.2f) * 123.f, 100.f);
                float y = std::fmod(std::sin(i * 4.5f) * 800.f + stats.timeElapsed * speed, 600.f);
                if (y < 0) y += 600.f;
                sf::CircleShape star(std::fmod(i, 3.f) + 1.f);
                star.setPosition({ x, y });
                // 【修改点】去除白色，使用幽蓝色，防止与子弹混淆
                star.setFillColor(sf::Color(100, 150, 255, 150));
                window.draw(star);
            }
        }
        else if (bgStyle == 2) {
            sf::Text rainChar(font, "0", 16);
            for (int i = 0; i < 40; ++i) {
                float x = i * 20.f;
                float speed = 100.f + std::fmod(std::sin(i * 1.1f) * 500.f, 150.f);
                float yOffset = std::fmod(stats.timeElapsed * speed, 600.f);
                for (int j = 0; j < 10; ++j) {
                    float y = std::fmod(std::sin(i * 7.f + j * 3.f) * 600.f + yOffset, 600.f);
                    if (y < 0) y += 600.f;
                    rainChar.setPosition({ x, y });
                    rainChar.setString((i + j + static_cast<int>(stats.timeElapsed * 5.f)) % 2 == 0 ? "1" : "0");
                    rainChar.setFillColor(sf::Color(0, 255, 0, static_cast<std::uint8_t>(std::max(0, 150 - j * 15))));
                    window.draw(rainChar);
                }
            }
        }
        else if (bgStyle == 3) {
            sf::CircleShape ring;
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(2.f);
            float t = stats.timeElapsed * 150.f;
            for (int i = 0; i < 6; ++i) {
                float r = std::fmod(t + i * 200.f, 1200.f);
                ring.setRadius(r);
                ring.setOrigin({ r, r });
                ring.setPosition({ 400.f, 300.f });
                float alpha = std::max(0.f, 255.f * (1.f - r / 1200.f));
                ring.setOutlineColor(sf::Color(0, 150, 255, static_cast<std::uint8_t>(alpha * 0.4f)));
                window.draw(ring);
            }
        }
    }

    sf::View view = window.getView();
    sf::Vector2f shakeOffset = { 0.f, 0.f };
    if (state == GameState::Playing || state == GameState::Paused) {
        shakeOffset = feelManager.getShakeOffset();
    }
    view.setCenter({ 400.f + shakeOffset.x, 300.f + shakeOffset.y });
    window.setView(view);

    sf::Text title(font, "", 40);
    sf::Text opt1(font, "", 24);
    sf::Text opt2(font, "", 24);
    sf::Text opt3(font, "", 24);

    if (state == GameState::Playing || state == GameState::Paused) {
        boss.draw(window);
        if (currentLevel == Level::Level3) boss2.draw(window);

        for (auto& dp : dataPoints) {
            window.draw(dp.glow);
            window.draw(dp.shape);
        }

        for (auto& sw : shockwaves) window.draw(sw.shape);
        for (auto& p : snakeProjectiles) window.draw(p.sprite);
        player.draw(window);

        for (int i = 0; i < player.health; ++i) {
            sf::RectangleShape hb({ 16.f, 16.f }); hb.setPosition({ 20.f + i * 22.f, 20.f }); hb.setFillColor(sf::Color::Green); window.draw(hb);
        }
        for (int i = 0; i < 2; ++i) {
            sf::CircleShape dashDot(6.f); dashDot.setPosition({ 20.f + i * 16.f, 45.f });
            dashDot.setFillColor(i < player.dashCharges ? sf::Color::Yellow : sf::Color(100, 100, 0, 150)); window.draw(dashDot);
        }
        if (player.dashCharges < 2) {
            sf::RectangleShape dashBarBG({ 30.f, 4.f }); dashBarBG.setPosition({ 20.f, 59.f }); dashBarBG.setFillColor(sf::Color(50, 50, 0)); window.draw(dashBarBG);
            sf::RectangleShape dashBar({ 30.f * (player.dashCooldown / 2.5f), 4.f }); dashBar.setPosition({ 20.f, 59.f }); dashBar.setFillColor(sf::Color::Yellow); window.draw(dashBar);
        }
        sf::RectangleShape eb({ 100.f * (player.energy / 100.f), 6.f }); eb.setPosition({ 20.f, 67.f }); eb.setFillColor(sf::Color::Cyan); window.draw(eb);

        sf::Text phaseText(font, "PHASE", 14); phaseText.setPosition({ 620.f, 20.f }); phaseText.setFillColor(sf::Color::White); window.draw(phaseText);
        for (int i = 0; i < 3; ++i) {
            sf::RectangleShape pb({ 30.f, 12.f }); pb.setPosition({ 680.f + i * 35.f, 22.f });
            pb.setFillColor(i <= boss.getCurrentPhase() ? sf::Color::Red : sf::Color(50, 0, 0));
            if (i == boss.getCurrentPhase()) { pb.setOutlineThickness(2.f); pb.setOutlineColor(sf::Color::Yellow); }
            window.draw(pb);
        }

        if (state == GameState::Paused) {
            sf::RectangleShape overlay({ 800.f, 600.f }); overlay.setFillColor(sf::Color(0, 0, 0, 150)); window.draw(overlay);
            title.setString("PAUSED"); centerText(title, 200.f); window.draw(title);
            opt1.setString("Continue"); opt2.setString("Return to Menu"); opt3.setString("Skip Level (Cheat)");
            opt1.setFillColor(menuSelection == 0 ? sf::Color::Yellow : sf::Color::White);
            opt2.setFillColor(menuSelection == 1 ? sf::Color::Yellow : sf::Color::White);
            opt3.setFillColor(menuSelection == 2 ? sf::Color::Yellow : sf::Color::White);
            centerText(opt1, 300.f); centerText(opt2, 350.f); centerText(opt3, 400.f);
            window.draw(opt1); window.draw(opt2); window.draw(opt3);
        }
    }
    else {
        if (state == GameState::Menu) {
            title.setString("LAST COMMAND - MODDED");
            opt1.setString("Start Game");
            opt2.setString("Skin Select");
            opt3.setString("Settings");
            sf::Text opt4(font, "Exit", 24);

            opt1.setFillColor(menuSelection == 0 ? sf::Color::Yellow : sf::Color::White);
            opt2.setFillColor(menuSelection == 1 ? sf::Color::Yellow : sf::Color::White);
            opt3.setFillColor(menuSelection == 2 ? sf::Color::Yellow : sf::Color::White);
            opt4.setFillColor(menuSelection == 3 ? sf::Color::Yellow : sf::Color::White);

            centerText(opt1, 280.f); window.draw(opt1);
            centerText(opt2, 330.f); window.draw(opt2);
            centerText(opt3, 380.f); window.draw(opt3);
            centerText(opt4, 430.f); window.draw(opt4);

            sf::Sprite himeSprite(previewHimeTex);
            int himeFrameW = previewHimeTex.getSize().x / 4; int himeFrameH = previewHimeTex.getSize().y;
            himeSprite.setTextureRect(sf::IntRect({ himePreviewFrame * himeFrameW, 0 }, { himeFrameW, himeFrameH }));
            himeSprite.setOrigin({ himeFrameW / 2.f, himeFrameH / 2.f }); himeSprite.setScale({ 3.0f, 3.0f });
            himeSprite.setPosition({ 650.f, 300.f }); himeSprite.setColor(sf::Color(255, 255, 255, 220)); window.draw(himeSprite);

            sf::Sprite honestSprite(previewHonestTex);
            int honestFrameW = previewHonestTex.getSize().x / 4; int honestFrameH = previewHonestTex.getSize().y;
            honestSprite.setTextureRect(sf::IntRect({ honestPreviewFrame * honestFrameW, 0 }, { honestFrameW, honestFrameH }));
            honestSprite.setOrigin({ honestFrameW / 2.f, honestFrameH / 2.f }); honestSprite.setScale({ 2.0f, 2.0f });
            honestSprite.setPosition({ 150.f, 300.f }); honestSprite.setColor(sf::Color(255, 255, 255, 220)); window.draw(honestSprite);

            if (recommendText) window.draw(*recommendText);
        }
        else if (state == GameState::Settings) {
            title.setString("SETTINGS");
            sf::Text sub(font, "< Use L/R Arrows to Change >", 20); centerText(sub, 220.f); window.draw(sub);

            std::string bgName = "Cyber Grid";
            if (bgStyle == 1) bgName = "Starfield";
            else if (bgStyle == 2) bgName = "Matrix Rain";
            else if (bgStyle == 3) bgName = "Pulse Rings";

            opt1.setString("Background: " + bgName);
            opt1.setFillColor(sf::Color::Yellow);
            centerText(opt1, 300.f); window.draw(opt1);

            sf::Text hint(font, "Press ENTER to Return", 18); centerText(hint, 450.f); window.draw(hint);
        }
        else if (state == GameState::SkinSelect) {
            title.setString("SKIN SELECT");
            sf::Text sub(font, "< Use L/R Arrows >", 20); centerText(sub, 220.f); window.draw(sub);
            sf::Sprite previewHead(snakeHeadTex); previewHead.setPosition({ 370.f, 300.f }); window.draw(previewHead);
            sf::Sprite previewBody(snakeBodyTexs[currentSkinIndex]); previewBody.setPosition({ 410.f, 300.f }); window.draw(previewBody);
            opt1.setString("Skin: " + std::to_string(currentSkinIndex + 1)); opt1.setFillColor(sf::Color::Yellow);
            sf::Text hint(font, "Press ENTER to Return", 18); centerText(hint, 450.f); window.draw(hint);
        }
        else if (state == GameState::LevelSelect) {
            title.setString("SELECT LEVEL");
            opt1.setString("Level 1: Static Core (Honest)");
            opt2.setString("Level 2: Moving Target (Hime)");
            opt3.setString("Level 3: Double Core (Honest x Hime)");
            opt1.setFillColor(menuSelection == 0 ? sf::Color::Yellow : sf::Color::White);
            opt2.setFillColor(menuSelection == 1 ? sf::Color::Yellow : sf::Color::White);
            opt3.setFillColor(menuSelection == 2 ? sf::Color::Yellow : sf::Color::White);
            centerText(opt3, 380.f); window.draw(opt3);

            if (menuSelection == 0 || menuSelection == 2) {
                sf::Sprite honestSprite(previewHonestTex);
                int honestFrameW = previewHonestTex.getSize().x / 4; int honestFrameH = previewHonestTex.getSize().y;
                honestSprite.setTextureRect(sf::IntRect({ honestPreviewFrame * honestFrameW, 0 }, { honestFrameW, honestFrameH }));
                honestSprite.setOrigin({ honestFrameW / 2.f, honestFrameH / 2.f });
                honestSprite.setScale({ menuSelection == 2 ? 1.5f : 2.0f, menuSelection == 2 ? 1.5f : 2.0f });
                honestSprite.setPosition({ 150.f, 300.f }); honestSprite.setColor(sf::Color(255, 255, 255, 200)); window.draw(honestSprite);
            }
            if (menuSelection == 1 || menuSelection == 2) {
                sf::Sprite himeSprite(previewHimeTex);
                int himeFrameW = previewHimeTex.getSize().x / 4; int himeFrameH = previewHimeTex.getSize().y;
                himeSprite.setTextureRect(sf::IntRect({ himePreviewFrame * himeFrameW, 0 }, { himeFrameW, himeFrameH }));
                himeSprite.setOrigin({ himeFrameW / 2.f, himeFrameH / 2.f });
                himeSprite.setScale({ menuSelection == 2 ? 2.2f : 3.0f, menuSelection == 2 ? 2.2f : 3.0f });
                himeSprite.setPosition({ 650.f, 300.f }); himeSprite.setColor(sf::Color(255, 255, 255, 200)); window.draw(himeSprite);
            }
        }
        else if (state == GameState::DiffSelect) {
            title.setString("SELECT DIFFICULTY");
            opt1.setString("Easy"); opt2.setString("Normal"); opt3.setString("Hard");
            opt1.setFillColor(menuSelection == 0 ? sf::Color::Yellow : sf::Color::White);
            opt2.setFillColor(menuSelection == 1 ? sf::Color::Yellow : sf::Color::White);
            opt3.setFillColor(menuSelection == 2 ? sf::Color::Yellow : sf::Color::White);
            centerText(opt3, 380.f); window.draw(opt3);
        }
        else if (state == GameState::GameOver || state == GameState::Win) {
            title.setString(state == GameState::Win ? "MISSION ACCOMPLISHED" : "SYSTEM FAILURE");
            title.setFillColor(state == GameState::Win ? sf::Color::Green : sf::Color::Red);
            opt1.setString("Time: " + std::to_string((int)stats.timeElapsed) + "s");
            opt2.setString("Max Length: " + std::to_string(stats.maxLength));
            opt3.setString("Damage Taken: " + std::to_string(stats.damageTaken));
            sf::Text hint(font, "Press ENTER to return", 18); centerText(hint, 450.f); window.draw(hint);
            centerText(opt3, 380.f); window.draw(opt3);
        }

        if (state != GameState::Playing && state != GameState::Paused) {
            centerText(title, 150.f); window.draw(title);
            if (state != GameState::SkinSelect && state != GameState::Settings) { centerText(opt1, 280.f); centerText(opt2, 330.f); window.draw(opt1); window.draw(opt2); }
        }
    }

    view.setCenter({ 400.f, 300.f });
    window.setView(view);

    window.display();
}

void Game::run() {
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        processEvents();
        update(dt);
        render();
    }
}