#include "Game.h"
#include <iostream>

Game::Game() : window(sf::VideoMode({ 1600, 1200 }), "Last Command - Modded"), state(GameState::Menu), menuSelection(0), unlockedLevels(1), currentSkinIndex(0), spawnTimer(0.f) {
    window.setFramerateLimit(60);
    sf::View view({ 400.f, 300.f }, { 800.f, 600.f });
    window.setView(view);

    loadResources();
    player.initSprites(snakeHeadTex, snakeBodyTexs[currentSkinIndex]);
}

void Game::loadResources() {
    (void)font.openFromFile("arial.ttf");
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

    // 加载特殊攻击的三个贴图
    (void)honestSpecialTex.loadFromFile("assets/BossHonest_Bullet.png");
    (void)himeSpecialTex.loadFromFile("assets/BossHime_Bullet.png");
    (void)bubbleTex.loadFromFile("assets/Boss_bubbleSmall.png");
}

void Game::startLevel() {
    player.reset();
    player.initSprites(snakeHeadTex, snakeBodyTexs[currentSkinIndex]);
    dataPoints.clear();
    shockwaves.clear();
    snakeProjectiles.clear();
    stats = GameStats();

    BossConfig cfg;
    // 注入特殊攻击贴图
    cfg.texHonestSpecial = &honestSpecialTex;
    cfg.texHimeSpecial = &himeSpecialTex;
    cfg.texBubble = &bubbleTex;

    if (currentLevel == Level::Level1) {
        cfg.bossId = 1; // 标记为 Honest
        cfg.stayTex = &boss1Stay; cfg.castTex = &boss1Cast; cfg.sufferTex = &boss1Suffer; cfg.endTex = &boss1Suffer; cfg.bgObjectTex = nullptr;
        cfg.animStay = { &boss1Stay, 8, 1, 8, 10.f };
        cfg.animCast = { &boss1Cast, 8, 3, 18, 10.f };
        cfg.animSuffer = { &boss1Suffer, 8, 4, 28, 10.f };
        cfg.animEnd = cfg.animSuffer;
    }
    else {
        cfg.bossId = 2; // 标记为 Hime
        cfg.stayTex = &boss2Stay; cfg.castTex = &boss2Cast; cfg.sufferTex = &boss2Suffer; cfg.endTex = &boss2End; cfg.bgObjectTex = &boss2BG;
        cfg.animStay = { &boss2Stay, 8, 1, 8, 10.f };
        cfg.animCast = { &boss2Cast, 6, 2, 11, 10.f };
        cfg.animSuffer = { &boss2Suffer, 6, 2, 11, 10.f };
        cfg.animEnd = { &boss2End, 6, 2, 12, 10.f };
    }

    boss.init(currentDiff, currentLevel, font, &heartOutTex, &heartFillTex, &bulletTex01, &bulletTex02, cfg);
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
                if (keyEvent->code == sf::Keyboard::Key::Down) menuSelection = std::min(2, menuSelection + 1);
                if (keyEvent->code == sf::Keyboard::Key::Enter) {
                    if (menuSelection == 0) state = GameState::LevelSelect;
                    else if (menuSelection == 1) state = GameState::SkinSelect;
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
            else if (state == GameState::LevelSelect) {
                if (keyEvent->code == sf::Keyboard::Key::Up) menuSelection = std::max(0, menuSelection - 1);
                if (keyEvent->code == sf::Keyboard::Key::Down) menuSelection = std::min(2, menuSelection + 1);
                if (keyEvent->code == sf::Keyboard::Key::U) unlockedLevels = 3;
                if (keyEvent->code == sf::Keyboard::Key::Escape) state = GameState::Menu;
                if (keyEvent->code == sf::Keyboard::Key::Enter) {
                    if (menuSelection < unlockedLevels) {
                        currentLevel = static_cast<Level>(menuSelection);
                        state = GameState::DiffSelect;
                        menuSelection = 0;
                    }
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
                if ((keyEvent->code == sf::Keyboard::Key::LShift || keyEvent->code == sf::Keyboard::Key::F)
                    && player.dashCharges > 0 && !player.isDashing) {
                    player.dashCharges--;
                    player.isDashing = true;
                    player.dashTimer = 0.15f;
                    player.isInvincible = true;
                    player.invTimer = 0.40f;
                }
                if (keyEvent->code == sf::Keyboard::Key::F1) boss.takeDamage(boss.getMaxHealth());
            }
            else if (state == GameState::Paused) {
                if (keyEvent->code == sf::Keyboard::Key::Up) menuSelection = std::max(0, menuSelection - 1);
                if (keyEvent->code == sf::Keyboard::Key::Down) menuSelection = std::min(2, menuSelection + 1);
                if (keyEvent->code == sf::Keyboard::Key::Enter) {
                    if (menuSelection == 0) state = GameState::Playing;
                    else if (menuSelection == 1) state = GameState::Menu;
                    else if (menuSelection == 2) {
                        boss.takeDamage(boss.getMaxHealth() * 3);
                        state = GameState::Win;
                    }
                }
            }
            else if (state == GameState::GameOver || state == GameState::Win) {
                if (keyEvent->code == sf::Keyboard::Key::Enter) {
                    if (state == GameState::Win) unlockedLevels = std::max(unlockedLevels, static_cast<int>(currentLevel) + 2);
                    state = GameState::Menu;
                }
            }
        }
    }
}

void Game::update(float dt) {
    if (state != GameState::Playing) return;

    stats.timeElapsed += dt;

    if (!boss.isDead() && !boss.isDying()) {
        if (dataPoints.empty()) {
            spawnTimer += dt;
            if (spawnTimer > 1.2f) {
                sf::Vector2f spawnPos;
                do {
                    spawnPos = { (float)(rand() % 700 + 50), (float)(rand() % 400 + 100) };
                } while (std::hypot(spawnPos.x - boss.getPosition().x, spawnPos.y - boss.getPosition().y) < 120.f);
                dataPoints.push_back(DataPoint(spawnPos));
                spawnTimer = 0.f;
            }
        }
    }

    if (player.update(dt, sf::Vector2u(800, 600))) {
        float damage = 10.f + player.bodyCount * 8.f;
        snakeProjectiles.push_back(SnakeProjectile(snakeAttackDotTex, player.headPos, damage));
    }

    boss.update(dt, currentDiff, currentLevel);
    stats.maxLength = std::max(stats.maxLength, player.bodyCount);

    for (auto& sw : shockwaves) sw.update(dt);
    (void)std::erase_if(shockwaves, [](const Shockwave& sw) { return !sw.isAlive; });

    (void)std::erase_if(dataPoints, [&](const DataPoint& dp) {
        if (std::hypot(player.headPos.x - dp.shape.getPosition().x, player.headPos.y - dp.shape.getPosition().y) < 20.f) {
            if (player.bodyCount < player.maxBody) player.bodyCount++;
            return true;
        }
        return false;
        });

    std::vector<Bullet>& bossBullets = boss.getBullets();
    for (const auto& sw : shockwaves) {
        (void)std::erase_if(bossBullets, [&](const Bullet& b) {
            return std::hypot(sw.shape.getPosition().x - b.sprite.getPosition().x, sw.shape.getPosition().y - b.sprite.getPosition().y) < sw.radius;
            });
    }

    (void)std::erase_if(snakeProjectiles, [&](SnakeProjectile& p) {
        p.update(dt, boss.getPosition());
        if (std::hypot(p.pos.x - boss.getPosition().x, p.pos.y - boss.getPosition().y) < boss.getHitboxRadius()) {
            boss.takeDamage(p.damage);
            return true;
        }
        return false;
        });

    if (!player.isInvincible) {
        for (auto it = bossBullets.begin(); it != bossBullets.end(); ) {
            // 所有子弹无论种类，碰撞体积统一定为 12.f
            if (std::hypot(player.headPos.x - it->sprite.getPosition().x, player.headPos.y - it->sprite.getPosition().y) < 12.f) {
                player.takeDamage();
                shockwaves.push_back(Shockwave(player.headPos));
                it = bossBullets.erase(it);
                break;
            }
            else ++it;
        }
    }

    if (player.health <= 0) state = GameState::GameOver;
    if (boss.isDead()) state = GameState::Win;
}

void Game::render() {
    window.clear(sf::Color(15, 15, 20));

    sf::Text title(font, "", 40);
    sf::Text opt1(font, "", 24);
    sf::Text opt2(font, "", 24);
    sf::Text opt3(font, "", 24);

    if (state == GameState::Playing || state == GameState::Paused) {
        boss.draw(window);
        for (auto& dp : dataPoints) window.draw(dp.shape);
        for (auto& sw : shockwaves) window.draw(sw.shape);
        for (auto& p : snakeProjectiles) window.draw(p.sprite);
        player.draw(window);

        for (int i = 0; i < player.health; ++i) {
            sf::RectangleShape hb({ 16.f, 16.f });
            hb.setPosition({ 20.f + i * 22.f, 20.f }); hb.setFillColor(sf::Color::Green); window.draw(hb);
        }
        for (int i = 0; i < 2; ++i) {
            sf::CircleShape dashDot(6.f);
            dashDot.setPosition({ 20.f + i * 16.f, 45.f });
            dashDot.setFillColor(i < player.dashCharges ? sf::Color::Yellow : sf::Color(100, 100, 0, 150));
            window.draw(dashDot);
        }
        if (player.dashCharges < 2) {
            sf::RectangleShape dashBarBG({ 30.f, 4.f }); dashBarBG.setPosition({ 20.f, 59.f }); dashBarBG.setFillColor(sf::Color(50, 50, 0)); window.draw(dashBarBG);
            sf::RectangleShape dashBar({ 30.f * (player.dashCooldown / 2.5f), 4.f }); dashBar.setPosition({ 20.f, 59.f }); dashBar.setFillColor(sf::Color::Yellow); window.draw(dashBar);
        }
        sf::RectangleShape eb({ 100.f * (player.energy / 100.f), 6.f });
        eb.setPosition({ 20.f, 67.f }); eb.setFillColor(sf::Color::Cyan); window.draw(eb);

        sf::Text phaseText(font, "PHASE", 14); phaseText.setPosition({ 620.f, 20.f }); phaseText.setFillColor(sf::Color::White); window.draw(phaseText);
        for (int i = 0; i < 3; ++i) {
            sf::RectangleShape pb({ 30.f, 12.f }); pb.setPosition({ 680.f + i * 35.f, 22.f });
            pb.setFillColor(i <= boss.getCurrentPhase() ? sf::Color::Red : sf::Color(50, 0, 0));
            if (i == boss.getCurrentPhase()) { pb.setOutlineThickness(2.f); pb.setOutlineColor(sf::Color::Yellow); }
            window.draw(pb);
        }

        if (state == GameState::Paused) {
            sf::RectangleShape overlay({ 800.f, 600.f });
            overlay.setFillColor(sf::Color(0, 0, 0, 150)); window.draw(overlay);
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
            opt1.setString("Start Game"); opt2.setString("Skin Select"); opt3.setString("Exit");
            opt1.setFillColor(menuSelection == 0 ? sf::Color::Yellow : sf::Color::White);
            opt2.setFillColor(menuSelection == 1 ? sf::Color::Yellow : sf::Color::White);
            opt3.setFillColor(menuSelection == 2 ? sf::Color::Yellow : sf::Color::White);
            centerText(opt3, 380.f); window.draw(opt3);
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
            title.setString("SELECT LEVEL (Press U to Unlock All)");
            opt1.setString(unlockedLevels > 0 ? "Level 1: Boss Honest" : "[LOCKED] Level 1");
            opt2.setString(unlockedLevels > 1 ? "Level 2: Boss Hime" : "[LOCKED] Level 2");
            opt3.setString(unlockedLevels > 2 ? "Level 3: Under Dev" : "[LOCKED] Level 3");
            opt1.setFillColor(menuSelection == 0 ? sf::Color::Yellow : (unlockedLevels > 0 ? sf::Color::White : sf::Color(100, 100, 100)));
            opt2.setFillColor(menuSelection == 1 ? sf::Color::Yellow : (unlockedLevels > 1 ? sf::Color::White : sf::Color(100, 100, 100)));
            opt3.setFillColor(menuSelection == 2 ? sf::Color::Yellow : (unlockedLevels > 2 ? sf::Color::White : sf::Color(100, 100, 100)));
            centerText(opt3, 380.f); window.draw(opt3);
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
            if (state != GameState::SkinSelect) { centerText(opt1, 280.f); centerText(opt2, 330.f); window.draw(opt1); window.draw(opt2); }
            else { centerText(opt1, 380.f); window.draw(opt1); }
        }
    }
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