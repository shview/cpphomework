#include "Game.h"
#include <iostream>
#include <cmath>
#include <algorithm>

// 构造函数：初始化 1600x1200 窗口，但内部通过 View 将视野锁定在经典的 800x600，实现伪抗锯齿与缩放。
Game::Game() : window(sf::VideoMode({ 1600, 1200 }), "Last Command - Modded"), state(GameState::Menu), menuSelection(0), unlockedLevels(3), currentSkinIndex(0), bgStyle(0), spawnTimer(0.f), healSpawnTimer(0.f) {
    window.setFramerateLimit(60); // 锁 60 帧，确保物理判定统一
    sf::View view({ 400.f, 300.f }, { 800.f, 600.f });
    window.setView(view);
    loadResources();
    player.initSprites(snakeHeadTex, snakeBodyTexs[currentSkinIndex]);
    soundManager.playMenuBGM();
}

// 【i18n 取词核心】根据当前的语言设置，从字典中获取宽字符文本
std::wstring Game::T(const std::string& key) {
    if (i18n.find(key) != i18n.end()) {
        return currentLang == Language::EN ? i18n[key].first : i18n[key].second;
    }
    return L""; // 防止键不存在导致的崩溃
}

void Game::loadResources() {
    // 【硬核加载机制】：为了防止文件没找到抛出警告，使用 (void) 强行抑制返回值
    (void)font.openFromFile("Cubic.ttf");

    // --- 【构建庞大的双语字典 (English, 中文)】 ---
    i18n["m_title"] = { L"LAST COMMAND - MODDED", L"最后指令 - 改版" };
    i18n["m_start"] = { L"Start Game", L"开始游戏" };
    i18n["m_skin"] = { L"Skin Select", L"皮肤选择" };
    i18n["m_set"] = { L"Settings", L"游戏设置" };
    i18n["m_exit"] = { L"Exit", L"退出游戏" };
    i18n["m_rec"] = { L"Highly recommend playing the original game!", L"推荐游玩原作" };

    i18n["set_title"] = { L"SETTINGS", L"游 戏 设 置" };
    i18n["set_sub"] = { L"< Use L/R Arrows to Change >", L"< 使用左右方向键修改 >" };
    i18n["set_lang"] = { L"Language: ", L"游戏语言: " };
    i18n["set_bg"] = { L"Background: ", L"游戏背景: " };
    i18n["set_bgm"] = { L"BGM: ", L"战斗音乐: " };
    i18n["set_aud"] = { L"Audio Settings >>", L"音量设置 >>" };
    i18n["set_ret"] = { L"Press ENTER to Return", L"按 ENTER 键确认并返回" };

    i18n["bg_0"] = { L"Cyber Grid", L"幽暗网格" };
    i18n["bg_1"] = { L"Starfield", L"星空跃迁" };
    i18n["bg_2"] = { L"Matrix Rain", L"代码黑客雨" };
    i18n["bg_3"] = { L"Pulse Rings", L"超声波波纹" };
    i18n["bgm_rand"] = { L"Random", L"随机播放" };

    i18n["aud_title"] = { L"AUDIO SETTINGS", L"音 量 设 置" };
    i18n["aud_b"] = { L"BGM Vol: ", L"音乐音量: " };
    i18n["aud_s"] = { L"SFX Vol: ", L"音效音量: " };
    i18n["aud_back"] = { L"Back", L"返回上一级" };

    i18n["sk_title"] = { L"SKIN SELECT", L"皮 肤 选 择" };
    i18n["sk_sub"] = { L"< Use L/R Arrows >", L"< 使用左右方向键 >" };
    i18n["sk_cur"] = { L"Skin: ", L"皮肤外观: " };

    i18n["lv_title"] = { L"SELECT LEVEL", L"选 择 关 卡" };
    i18n["lv_1"] = { L"Level 1: Static Core (Honest)", L"第一关: 静态核心 (Honest)" };
    i18n["lv_2"] = { L"Level 2: Moving Target (Hime)", L"第二关: 移动标靶 (Hime)" };
    i18n["lv_3"] = { L"Level 3: Double Core (Honest x Hime)", L"第三关: 双生核心 (Honest x Hime)" };

    i18n["df_title"] = { L"SELECT DIFFICULTY", L"选 择 难 度" };
    i18n["df_e"] = { L"Easy", L"简单 (Easy)" };
    i18n["df_n"] = { L"Normal", L"正常 (Normal)" };
    i18n["df_h"] = { L"Hard", L"困难 (Hard)" };

    i18n["hud_phase"] = { L"PHASE", L"当前阶段" };

    i18n["pa_title"] = { L"PAUSED", L"游 戏 暂 停" };
    i18n["pa_cont"] = { L"Continue", L"继续游戏" };
    i18n["pa_ret"] = { L"Return to Menu", L"返回主菜单" };
    i18n["pa_skip"] = { L"Skip Level (Cheat)", L"跳过关卡 (作弊)" };

    i18n["go_title"] = { L"SYSTEM FAILURE", L"系 统 崩 溃" };
    i18n["go_t"] = { L"Time: ", L"通关耗时: " };
    i18n["go_l"] = { L"Max Length: ", L"最大长度: " };
    i18n["go_d"] = { L"Damage Taken: ", L"受到伤害: " };
    i18n["go_loss"] = { L"Data Lost: ", L"数据丢失: " };
    i18n["go_r1"] = { L"Revive: Current State (Clear Bullets)", L"当前状态复活 (原地满血复活，清空弹幕)" };
    i18n["go_r2"] = { L"Revive: Restart Phase", L"当前阶段重置复活" };
    i18n["go_ret"] = { L"Give up & Return to Menu", L"放弃并返回主菜单" };

    i18n["wi_title"] = { L"MISSION ACCOMPLISHED", L"任 务 完 成" };
    // ------------------------------------

    // 读取所有视觉贴图
    (void)bulletTex01.loadFromFile("assets/images/bulletBoss01.png");
    (void)bulletTex02.loadFromFile("assets/images/bulletBoss02.png");
    (void)snakeHeadTex.loadFromFile("assets/images/snakeSkinHead.png");

    snakeBodyTexs.resize(5);
    for (int i = 0; i < 5; ++i) {
        (void)snakeBodyTexs[i].loadFromFile("assets/images/snakeSkinBody0" + std::to_string(i + 1) + ".png");
    }
    (void)snakeAttackDotTex.loadFromFile("assets/images/snakeAttackDot.png");
    (void)heartOutTex.loadFromFile("assets/images/levelHeart20x22OutlineForHonest.png");
    (void)heartFillTex.loadFromFile("assets/images/levelHeart20x22.png");

    (void)boss1Stay.loadFromFile("assets/images/BossHonest_Stay.png");
    (void)boss1Cast.loadFromFile("assets/images/BossHonest_Cast01.png");
    (void)boss1Suffer.loadFromFile("assets/images/BossHonest_Suffer.png");

    (void)boss2Stay.loadFromFile("assets/images/BossHime_Stay.png");
    (void)boss2Cast.loadFromFile("assets/images/BossHime_Start.png");
    (void)boss2Suffer.loadFromFile("assets/images/BossHime_Suffer.png");
    (void)boss2End.loadFromFile("assets/images/BossHime_End.png");
    (void)boss2BG.loadFromFile("assets/images/BossHime_BG_OBJECT.png");

    (void)honestSpecialTex.loadFromFile("assets/images/BossHonest_Bullet.png");
    (void)himeSpecialTex.loadFromFile("assets/images/BossHime_Bullet.png");
    (void)bubbleTex.loadFromFile("assets/images/Boss_bubbleSmall.png");

    (void)previewHimeTex.loadFromFile("assets/images/npcSpineCalculationHime.png");
    (void)previewHonestTex.loadFromFile("assets/images/Honest_idle.png");

    // 读取所有音频 (长音乐做流播放，短音效放 Buffer)
    (void)soundManager.bgmMenu.openFromFile("assets/sound/Algromith.wav");
    soundManager.bgmMenu.setVolume(30.f);

    (void)soundManager.bgmBattle[0].openFromFile("assets/sound/Algromith.wav");
    soundManager.bgmBattle[0].setVolume(30.f);

    (void)soundManager.bgmBattle[1].openFromFile("assets/sound/Honest_Battle_MasterRevert.wav");
    soundManager.bgmBattle[1].setVolume(30.f);

    (void)soundManager.bgmBattle[2].openFromFile("assets/sound/GerneralEventTypeA.wav");
    soundManager.bgmBattle[2].setVolume(30.f);

    (void)soundManager.bgmVictory.openFromFile("assets/sound/Credit.wav");
    soundManager.bgmVictory.setVolume(40.f);

    (void)soundManager.bgmDefeat.openFromFile("assets/sound/DramaDanger.wav");
    soundManager.bgmDefeat.setVolume(40.f);

    // 氛围白噪音持续播放
    (void)soundManager.bgmAmbient.openFromFile("assets/sound/Massive_City0530Mastering_LOOP.wav");
    soundManager.bgmAmbient.setVolume(15.f);
    soundManager.bgmAmbient.setLooping(true);
    soundManager.bgmAmbient.play();

    soundManager.loadBuffer("parse_shoot", "assets/sound/attack_3_loud.wav");
    soundManager.loadBuffer("boss_hurt", "assets/sound/battle_bossBlockHurt_2.wav");
    soundManager.loadBuffer("dash", "assets/sound/battle_dash_3.wav");
    soundManager.loadBuffer("player_hurt", "assets/sound/battle_playerDeadBody_2.wav");
    soundManager.loadBuffer("player_dead", "assets/sound/battle_playerDeadMain_2.wav");
    soundManager.loadBuffer("bullet", "assets/sound/bullet.wav");
    soundManager.loadBuffer("click", "assets/sound/Element Click.wav");
    soundManager.loadBuffer("nav", "assets/sound/Navigation_Slide_05.wav");
    soundManager.loadBuffer("bubble", "assets/sound/battle_dashRestore_3.wav");
    soundManager.loadBuffer("special", "assets/sound/battle_feverTrigger_1.wav");
    soundManager.loadBuffer("player_shoot", "assets/sound/bullet_anserLaser_action_1.wav");
    soundManager.loadBuffer("shockwave", "assets/sound/ui_limitWarning_serious.wav");
    soundManager.loadBuffer("pick_data", "assets/sound/battle_dottedline_1.wav");
    soundManager.loadBuffer("error", "assets/sound/DM-CGS-04.wav");
    soundManager.loadBuffer("warning", "assets/sound/scene_computerBi.wav");
    soundManager.loadBuffer("heal", "assets/sound/UI Click 8.wav");
    soundManager.loadBuffer("boss_dead", "assets/sound/battle_bossDead00.wav");
    soundManager.loadBuffer("aim", "assets/sound/bullet_selfPhotoTip_1.wav");
    soundManager.loadBuffer("parsing_loop", "assets/sound/Computation_Periodic_03.wav");
    soundManager.loadBuffer("dash_restore", "assets/sound/UI Click 13.wav");

    soundManager.updateVolumes();

    recommendText.emplace(font);
    recommendText->setCharacterSize(18);
    recommendText->setFillColor(sf::Color(255, 255, 255, 180));
    recommendText->setString(T("m_rec"));
    centerText(*recommendText, 550.f);
}

// 用于音频设置界面的可视进度条生成
void Game::getBarString(int vol, std::wstring& outStr) {
    outStr = L"[";
    for (int i = 0; i < 10; ++i) outStr += (i < vol) ? L"|" : L" ";
    outStr += L"]";
}

// 供 "当前阶段重置复活" 选项调用。抹除玩家所有加成，抹除全图状态，让Boss满血回到该阶段起始位置。
void Game::restartPhase() {
    player.reset();
    player.initSprites(snakeHeadTex, snakeBodyTexs[currentSkinIndex]);
    player.health = 3;
    player.isInvincible = true;
    player.invTimer = 3.0f; // 复活给予超长无敌帧

    dataPoints.clear();
    shockwaves.clear();
    snakeProjectiles.clear();
    spawnTimer = 0.f;
    healSpawnTimer = 0.f;
    bossSwapTimer = 0.f;
    isSwapping = false;

    boss.resetToCurrentPhase();
    if (currentLevel == Level::Level3) {
        boss2.resetToCurrentPhase();
        int combinedPhase = std::max(boss.getCurrentPhase(), boss2.getCurrentPhase());
        if (combinedPhase == 0) {
            boss.setBasePosX(200.f); boss.setBasePosY(300.f);
            boss2.setBasePosX(600.f); boss2.setBasePosY(300.f);
        }
        else {
            boss.setBasePosX(150.f); boss.setBasePosY(100.f);
            boss2.setBasePosX(650.f); boss2.setBasePosY(500.f);
        }
    }

    soundManager.playBattleBGM(bgmStyle);
    state = GameState::Playing;
    timeScale = 1.0f; // 清除时空减速
}

// 创建一个全新干净的游戏关卡
void Game::startLevel() {
    player.reset();
    player.initSprites(snakeHeadTex, snakeBodyTexs[currentSkinIndex]);
    dataPoints.clear();
    shockwaves.clear();
    snakeProjectiles.clear();
    stats = GameStats(); // 结算数据彻底清零
    lastParsingState = false;
    borderFlashTimer = 0.f;
    timeScale = 1.0f;

    bossSwapTimer = 0.f;
    isSwapping = false;
    spawnTimer = 0.f;
    healSpawnTimer = 0.f;

    soundManager.playBattleBGM(bgmStyle);

    // 使用不同参数实例化具体的 Boss 对象
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
        cfg.bossHitboxRadius = 35.f;
        boss.init(currentDiff, currentLevel, font, &heartOutTex, &heartFillTex, &bulletTex01, &bulletTex02, cfg, &soundManager);
        boss2.kill(); // 隐藏不需要的二号位 Boss
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
        cfg.bossHitboxRadius = 45.f;
        boss.init(currentDiff, currentLevel, font, &heartOutTex, &heartFillTex, &bulletTex01, &bulletTex02, cfg, &soundManager);
        boss2.kill();
    }
    else if (currentLevel == Level::Level3) {
        // 双 Boss 联合关卡配置
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
        cfgHime.bossHitboxRadius = 45.f;
        boss.init(currentDiff, Level::Level3, font, &heartOutTex, &heartFillTex, &bulletTex01, &bulletTex02, cfgHime, &soundManager);

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
        cfgHonest.bossHitboxRadius = 35.f;
        boss2.init(currentDiff, Level::Level3, font, &heartOutTex, &heartFillTex, &bulletTex01, &bulletTex02, cfgHonest, &soundManager);
    }
    state = GameState::Playing;
}

// 将文本坐标对齐至横向中心
void Game::centerText(sf::Text& text, float y) {
    sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin({ bounds.size.x / 2.0f, bounds.size.y / 2.0f });
    text.setPosition({ 400.f, y });
}

// 大管家轮询事件 (按键与系统)
void Game::processEvents() {
    while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) window.close();

        if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {

            // --- UI 导航控制区 (加入了取模运算实现无限循环) ---
            if (state == GameState::Menu) {
                if (keyEvent->code == sf::Keyboard::Key::Up) { menuSelection = (menuSelection - 1 + 4) % 4; soundManager.playSound("nav"); }
                if (keyEvent->code == sf::Keyboard::Key::Down) { menuSelection = (menuSelection + 1) % 4; soundManager.playSound("nav"); }
                if (keyEvent->code == sf::Keyboard::Key::Enter) {
                    soundManager.playSound("click");
                    if (menuSelection == 0) state = GameState::LevelSelect;
                    else if (menuSelection == 1) state = GameState::SkinSelect;
                    else if (menuSelection == 2) { state = GameState::Settings; settingsSelection = 0; }
                    else window.close();
                    menuSelection = 0;
                }
            }
            else if (state == GameState::SkinSelect) {
                if (keyEvent->code == sf::Keyboard::Key::Left) { currentSkinIndex = (currentSkinIndex - 1 + 5) % 5; soundManager.playSound("nav"); }
                if (keyEvent->code == sf::Keyboard::Key::Right) { currentSkinIndex = (currentSkinIndex + 1) % 5; soundManager.playSound("nav"); }
                if (keyEvent->code == sf::Keyboard::Key::Enter || keyEvent->code == sf::Keyboard::Key::Escape) {
                    soundManager.playSound("click");
                    state = GameState::Menu;
                    menuSelection = 0;
                }
            }
            else if (state == GameState::Settings) {
                if (keyEvent->code == sf::Keyboard::Key::Up) { settingsSelection = (settingsSelection - 1 + 4) % 4; soundManager.playSound("nav"); }
                if (keyEvent->code == sf::Keyboard::Key::Down) { settingsSelection = (settingsSelection + 1) % 4; soundManager.playSound("nav"); }
                if (keyEvent->code == sf::Keyboard::Key::Left) {
                    if (settingsSelection == 0) currentLang = (currentLang == Language::EN) ? Language::ZH : Language::EN;
                    else if (settingsSelection == 1) bgStyle = (bgStyle - 1 + 4) % 4;
                    else if (settingsSelection == 2) { bgmStyle = (bgmStyle - 1 + 4) % 4; soundManager.previewBGM(bgmStyle); }
                    soundManager.playSound("nav");
                    recommendText->setString(T("m_rec"));
                }
                if (keyEvent->code == sf::Keyboard::Key::Right) {
                    if (settingsSelection == 0) currentLang = (currentLang == Language::EN) ? Language::ZH : Language::EN;
                    else if (settingsSelection == 1) bgStyle = (bgStyle + 1) % 4;
                    else if (settingsSelection == 2) { bgmStyle = (bgmStyle + 1) % 4; soundManager.previewBGM(bgmStyle); }
                    soundManager.playSound("nav");
                    recommendText->setString(T("m_rec"));
                }
                if (keyEvent->code == sf::Keyboard::Key::Enter) {
                    soundManager.playSound("click");
                    if (settingsSelection == 3) { state = GameState::SettingsAudio; audioSelection = 0; }
                    else { state = GameState::Menu; menuSelection = 0; soundManager.playMenuBGM(); }
                }
                else if (keyEvent->code == sf::Keyboard::Key::Escape) {
                    soundManager.playSound("click");
                    state = GameState::Menu; menuSelection = 0; soundManager.playMenuBGM();
                }
            }
            else if (state == GameState::SettingsAudio || state == GameState::PausedAudio) {
                if (keyEvent->code == sf::Keyboard::Key::Up) { audioSelection = (audioSelection - 1 + 3) % 3; soundManager.playSound("nav"); }
                if (keyEvent->code == sf::Keyboard::Key::Down) { audioSelection = (audioSelection + 1) % 3; soundManager.playSound("nav"); }
                if (keyEvent->code == sf::Keyboard::Key::Left) {
                    if (audioSelection == 0) bgmVolInt = std::max(0, bgmVolInt - 1);
                    else if (audioSelection == 1) sfxVolInt = std::max(0, sfxVolInt - 1);
                    // 同步到管理器的真实音量因子
                    soundManager.bgmVolumeMulti = bgmVolInt / 10.f;
                    soundManager.sfxVolumeMulti = sfxVolInt / 10.f;
                    soundManager.updateVolumes();
                    if (audioSelection == 1) soundManager.playSound("player_shoot");
                    soundManager.playSound("nav");
                }
                if (keyEvent->code == sf::Keyboard::Key::Right) {
                    if (audioSelection == 0) bgmVolInt = std::min(10, bgmVolInt + 1);
                    else if (audioSelection == 1) sfxVolInt = std::min(10, sfxVolInt + 1);
                    soundManager.bgmVolumeMulti = bgmVolInt / 10.f;
                    soundManager.sfxVolumeMulti = sfxVolInt / 10.f;
                    soundManager.updateVolumes();
                    if (audioSelection == 1) soundManager.playSound("player_shoot");
                    soundManager.playSound("nav");
                }
                if (keyEvent->code == sf::Keyboard::Key::Enter || keyEvent->code == sf::Keyboard::Key::Escape) {
                    soundManager.playSound("click");
                    if (audioSelection == 2 || keyEvent->code == sf::Keyboard::Key::Escape) {
                        state = (state == GameState::SettingsAudio) ? GameState::Settings : GameState::Paused;
                    }
                }
            }
            else if (state == GameState::LevelSelect) {
                if (keyEvent->code == sf::Keyboard::Key::Up) { menuSelection = (menuSelection - 1 + 3) % 3; soundManager.playSound("nav"); }
                if (keyEvent->code == sf::Keyboard::Key::Down) { menuSelection = (menuSelection + 1) % 3; soundManager.playSound("nav"); }
                if (keyEvent->code == sf::Keyboard::Key::Escape) { state = GameState::Menu; soundManager.playSound("click"); }
                if (keyEvent->code == sf::Keyboard::Key::Enter) {
                    soundManager.playSound("click");
                    currentLevel = static_cast<Level>(menuSelection);
                    state = GameState::DiffSelect;
                    menuSelection = 0;
                }
            }
            else if (state == GameState::DiffSelect) {
                if (keyEvent->code == sf::Keyboard::Key::Up) { menuSelection = (menuSelection - 1 + 3) % 3; soundManager.playSound("nav"); }
                if (keyEvent->code == sf::Keyboard::Key::Down) { menuSelection = (menuSelection + 1) % 3; soundManager.playSound("nav"); }
                if (keyEvent->code == sf::Keyboard::Key::Escape) { state = GameState::LevelSelect; soundManager.playSound("click"); }
                if (keyEvent->code == sf::Keyboard::Key::Enter) {
                    soundManager.playSound("click");
                    currentDiff = static_cast<Difficulty>(menuSelection);
                    startLevel(); // 正式载入并开始关卡！
                }
            }
            // --- 核心游玩操作检测区 ---
            else if (state == GameState::Playing) {
                if (keyEvent->code == sf::Keyboard::Key::Escape) {
                    soundManager.playSound("click");
                    state = GameState::Paused; menuSelection = 0;
                }
                // 玩家按下 Q：震碎弹幕并释放 Shockwave 实体
                if (keyEvent->code == sf::Keyboard::Key::Q) {
                    if (player.energy >= 35.f) {
                        player.energy -= 35.f;
                        shockwaves.push_back(Shockwave(player.headPos));
                        soundManager.playSound("shockwave");
                    }
                    else {
                        soundManager.playSound("error");
                    }
                }
                // 玩家按下 LShift/F：触发极其短促但带无敌的瞬移冲刺
                if ((keyEvent->code == sf::Keyboard::Key::LShift || keyEvent->code == sf::Keyboard::Key::F) && !player.isDashing) {
                    if (player.dashCharges > 0) {
                        player.dashCharges--;
                        player.isDashing = true;
                        player.dashTimer = 0.15f;
                        player.isInvincible = true;
                        player.invTimer = 0.40f;
                        soundManager.playSound("dash");
                    }
                    else {
                        soundManager.playSound("error");
                    }
                }

                // 【测试工具热键 / 开发者大礼包】
                // F1: 直接将场上全部存活的 Boss 扣除阶段血量
                if (keyEvent->code == sf::Keyboard::Key::F1) { boss.takeDamage(boss.getMaxHealth()); boss2.takeDamage(boss2.getMaxHealth()); }
                // F2: 切换“子弹时间”，时空缩放降至 1/4
                if (keyEvent->code == sf::Keyboard::Key::F2) { timeScale = (timeScale == 1.0f) ? 0.25f : 1.0f; }
                // F3: 回满血，非常适合练身法
                if (keyEvent->code == sf::Keyboard::Key::F3) { player.health = 5; soundManager.playSound("heal"); }
                // F4: 强制塞入一节数据至尾巴
                if (keyEvent->code == sf::Keyboard::Key::F4) {
                    if (player.bodyCount < player.maxBody) {
                        player.bodyCount++;
                        soundManager.playSound("pick_data");
                    }
                }
                // F5: 弹幕擦除器。强行清除场上所有致命物，同时免费附赠一个清屏冲击波
                if (keyEvent->code == sf::Keyboard::Key::F5) {
                    boss.clearDanmaku();
                    if (currentLevel == Level::Level3) boss2.clearDanmaku();
                    shockwaves.push_back(Shockwave(player.headPos));
                    soundManager.playSound("shockwave");
                }
            }
            else if (state == GameState::Paused) {
                if (keyEvent->code == sf::Keyboard::Key::Up) { menuSelection = (menuSelection - 1 + 5) % 5; soundManager.playSound("nav"); }
                if (keyEvent->code == sf::Keyboard::Key::Down) { menuSelection = (menuSelection + 1) % 5; soundManager.playSound("nav"); }
                if (keyEvent->code == sf::Keyboard::Key::Enter) {
                    soundManager.playSound("click");
                    if (menuSelection == 0) state = GameState::Playing;
                    else if (menuSelection == 1) { state = GameState::PausedAudio; audioSelection = 0; }
                    else if (menuSelection == 2) { state = GameState::Menu; soundManager.playMenuBGM(); }
                    else if (menuSelection == 3) { boss.kill(); boss2.kill(); state = GameState::Win; soundManager.stopAllBGM(); }
                    else if (menuSelection == 4) { currentLang = (currentLang == Language::EN) ? Language::ZH : Language::EN; }
                }
            }
            else if (state == GameState::GameOver) {
                if (keyEvent->code == sf::Keyboard::Key::Up) { menuSelection = (menuSelection - 1 + 3) % 3; soundManager.playSound("nav"); }
                if (keyEvent->code == sf::Keyboard::Key::Down) { menuSelection = (menuSelection + 1) % 3; soundManager.playSound("nav"); }
                if (keyEvent->code == sf::Keyboard::Key::Enter) {
                    soundManager.playSound("click");
                    if (menuSelection == 0) {
                        // 原地复活：重置血量并赠送 3 秒超级无敌帧与大招清场
                        state = GameState::Playing;
                        player.health = 3;
                        player.isInvincible = true;
                        player.invTimer = 3.0f;
                        shockwaves.push_back(Shockwave(player.headPos));
                        soundManager.playBattleBGM(bgmStyle);
                        timeScale = 1.0f;
                    }
                    else if (menuSelection == 1) {
                        // 阶段重置
                        restartPhase();
                    }
                    else if (menuSelection == 2) {
                        // 放弃
                        soundManager.playMenuBGM();
                        state = GameState::Menu;
                        timeScale = 1.0f;
                        menuSelection = 0;
                    }
                }
            }
            else if (state == GameState::Win) {
                if (keyEvent->code == sf::Keyboard::Key::Enter) {
                    soundManager.playSound("click");
                    soundManager.playMenuBGM();
                    state = GameState::Menu;
                }
            }
        }
    }
}

// 核心大循环物理/动画推进
void Game::update(float dt) {
    soundManager.update();

    // UI 界面的背景动画和预览动画在此推进
    if (state == GameState::Menu || state == GameState::LevelSelect || state == GameState::Settings || state == GameState::SettingsAudio) {
        stats.timeElapsed += dt;
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

    // 非战斗期直接返回，不推进物理计算
    if (state != GameState::Playing) return;

    // 【核心时间系统控制】所有的物理、冷却、动画推进，全部乘以 timeScale。
    // 这意味着如果 F2 开启了 0.25 的缩放，连带着所有的速度都会变慢。
    dt *= timeScale;

    stats.timeElapsed += dt;
    feelManager.update(dt);

    // 如果画面被“受击重创”冻结，则强行吞掉这一帧，所有物体原样定格
    if (feelManager.isFrozen()) return;

    // 控制解析大招的“嗡嗡”蓄力声频
    if (player.isParsing && player.bodyCount >= 3) {
        soundManager.setLoopingSound("parsing_loop", true);
    }
    else {
        soundManager.setLoopingSound("parsing_loop", false);
    }

    // 一些工具函数：强制给双 Boss 摆位
    auto preSetBossPos = [&](int phase) {
        if (phase == 0) {
            boss.setBasePosX(200.f); boss.setBasePosY(300.f);
            boss2.setBasePosX(600.f); boss2.setBasePosY(300.f);
        }
        else if (phase == 1) {
            boss.setBasePosX(150.f); boss.setBasePosY(100.f);
            boss2.setBasePosX(650.f); boss2.setBasePosY(500.f);
        }
        };

    // 如果两个 Boss 都死完了，则共同进入下一个阶段
    if (currentLevel == Level::Level3) {
        bool b1Wait = (boss.getState() == BossState::PhaseWait || boss.getState() == BossState::Dead);
        bool b2Wait = (boss2.getState() == BossState::PhaseWait || boss2.getState() == BossState::Dead);

        if (b1Wait && b2Wait) {
            int combinedPhase = std::max(boss.getCurrentPhase(), boss2.getCurrentPhase());
            preSetBossPos(combinedPhase);

            if (boss.getState() == BossState::PhaseWait) boss.advancePhase();
            if (boss2.getState() == BossState::PhaseWait) boss2.advancePhase();
        }
    }
    else {
        // 单体 Boss 直接转段
        if (boss.getState() == BossState::PhaseWait) boss.advancePhase();
    }

    // --- 数据块生成控制 ---
    if (!boss.isDead() && !boss.isDying()) {
        // 如果场上没有任何数据包，快速倒计时 1.2s 刷一个
        if (dataPoints.empty()) {
            spawnTimer += dt;
            if (spawnTimer > 1.2f) {
                sf::Vector2f spawnPos;
                do {
                    spawnPos = { (float)(rand() % 700 + 50), (float)(rand() % 400 + 100) };
                    // 生成点不能贴脸刷在 Boss 脚下
                } while (std::hypot(spawnPos.x - boss.getPosition().x, spawnPos.y - boss.getPosition().y) < 120.f);
                dataPoints.push_back(DataPoint(spawnPos, 0));
                spawnTimer = 0.f;
            }
        }

        // 保底回血机制：如果场上没有绿包，则每 10s 生成一个
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

    int oldDashCharges = player.dashCharges;

    // --- 核心玩家判定点：调用 player.update 并接盘抛出的发射请求 ---
    int consumedData = player.update(dt, sf::Vector2u(800, 600));

    // 触发撞墙警告闪红蓝光边框
    if (player.triggerBorderFlash) {
        borderFlashTimer = 0.6f;
        player.triggerBorderFlash = false;
        soundManager.playSound("error");
    }
    if (borderFlashTimer > 0.f) borderFlashTimer -= dt;

    // 解析成功，获得子弹投射权
    if (consumedData > 0) {
        // 高额回报机制！如果超过 3 节数据，基础威力将从 *6 飙升到 *10！
        float damage = (consumedData <= 3) ? (consumedData * 6.f) : (consumedData * 10.f);

        int targetId = 1;
        // 小辅助函数，只有活着的 Boss 才会被锁定
        auto isValid = [](Boss& b) {
            return b.getState() != BossState::Dead && b.getState() != BossState::PhaseWait && b.getState() != BossState::Dying;
            };

        // 如果场上有两个活人，谁近就锁定谁发射
        if (currentLevel == Level::Level3) {
            float dist1 = isValid(boss) ? std::hypot(player.headPos.x - boss.getPosition().x, player.headPos.y - boss.getPosition().y) : 999999.f;
            float dist2 = isValid(boss2) ? std::hypot(player.headPos.x - boss2.getPosition().x, player.headPos.y - boss2.getPosition().y) : 999999.f;

            if (dist2 < dist1) {
                targetId = 2;
            }
        }

        // 把目标ID传进去，死锁追踪
        snakeProjectiles.push_back(SnakeProjectile(snakeAttackDotTex, player.headPos, damage, targetId));
        soundManager.playSound("parse_shoot");
        soundManager.playSound("player_shoot");
    }

    if (player.dashCharges > oldDashCharges) {
        soundManager.playSound("dash_restore");
    }

    sf::Vector2f playerBodyPos = player.headPos;
    if (!player.trail.empty()) {
        playerBodyPos = player.trail[player.trail.size() / 2].position;
    }

    // ===========================================
    // 🎭 Boss 宏观阵型调度系统 (Stage Director)
    // ===========================================
    // 这个庞大的 IF 分支控制着两位 Boss 在不同阶段如何互相绕圈、漂浮、或者在四角跳位。
    // 具体移动都通过简单的数学公式(Sin/Cos/多项式平滑)叠加到 basePosX/Y 上。
    if (currentLevel == Level::Level3) {
        int combinedPhase = std::max(boss.getCurrentPhase(), boss2.getCurrentPhase());

        if (combinedPhase == 0) {
            // 第一阶段：每 15 秒双 Boss 左右对调
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
                float t = 0.5f * (1.f - std::cos(3.14159f * swapProgress)); // Cosine 平滑缓动
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

            // 第二阶段：Y轴上下反相悬浮移动
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

            // 第三阶段：24 秒长周期的四角矩形换位
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
                progress = progress * progress * (3.f - 2.f * progress); // 三次方 Smoothstep
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

        // 把玩家位置透传给 Boss 实体，用作狙击预判计算
        boss.update(dt, currentDiff, currentLevel, player.headPos, player.currentDir, playerBodyPos);
        boss2.update(dt, currentDiff, currentLevel, player.headPos, player.currentDir, playerBodyPos);
    }
    else {
        boss.update(dt, currentDiff, currentLevel, player.headPos, player.currentDir, playerBodyPos);
    }

    // 记录最大身长供结算显示
    stats.maxLength = std::max(stats.maxLength, player.bodyCount);

    // 推进冲击波圈扩大
    for (auto& sw : shockwaves) sw.update(dt);
    (void)std::erase_if(shockwaves, [](const Shockwave& sw) { return !sw.isAlive; });

    // 【普通的更新数据点】
    for (auto& dp : dataPoints) dp.update(dt);

    // 玩家物理接触数据掉落物的拾取逻辑
    (void)std::erase_if(dataPoints, [&](const DataPoint& dp) {
        if (dp.isFading && dp.timer >= dp.maxLifetime) return true;

        // 磁铁吸过来之后碰到嘴巴就算吃掉
        if (std::hypot(player.headPos.x - dp.shape.getPosition().x, player.headPos.y - dp.shape.getPosition().y) < 20.f) {
            if (dp.type == 0 || dp.type == 2) {
                if (player.bodyCount < player.maxBody) player.bodyCount++;
                soundManager.playSound("pick_data");
            }
            else if (dp.type == 1) {
                player.health = std::min(player.health + 1, 5);
                healSpawnTimer = 0.f;
                soundManager.playSound("heal");
            }
            return true;
        }
        return false;
        });

    std::vector<Bullet>& bossBullets = boss.getBullets();
    std::vector<Bullet>& bossBullets2 = boss2.getBullets();

    // 巨大的子弹需要更早被消除，所以查表获取不同类型子弹的精确半径
    auto getBulletRadius = [](int type) -> float {
        if (type == 10) return 15.f;
        if (type == 11) return 65.f;
        if (type == 12) return 25.f;
        if (type == 1) return 18.f;
        return 12.f;
        };

    // 冲击波消除子弹逻辑 (距离 < 冲击波半径 + 子弹本体半径 就会被波及震碎)
    auto clearBullets = [&](std::vector<Bullet>& blist) {
        std::erase_if(blist, [&](const Bullet& b) {
            float br = getBulletRadius(b.type);
            for (const auto& sw : shockwaves) {
                if (std::hypot(sw.shape.getPosition().x - b.sprite.getPosition().x, sw.shape.getPosition().y - b.sprite.getPosition().y) < sw.radius + br) {
                    return true;
                }
            }
            return false;
            });
        };
    clearBullets(bossBullets);
    clearBullets(bossBullets2);

    // 玩家自己发射的子弹逻辑
    (void)std::erase_if(snakeProjectiles, [&](SnakeProjectile& p) {
        bool hasTarget = false;
        sf::Vector2f targetPos;
        // 获取这枚子弹发射时死锁的目标
        Boss* targetBoss = (p.targetBossId == 1) ? &boss : &boss2;

        if (targetBoss->getState() != BossState::Dead && targetBoss->getState() != BossState::PhaseWait && targetBoss->getState() != BossState::Dying) {
            targetPos = targetBoss->getPosition();
            hasTarget = true;
        }

        if (hasTarget) {
            // 进行自动追踪
            p.update(dt, targetPos);
        }
        else {
            // 如果目标在此期间死了或处于无敌段，子弹失去追踪，只能直勾勾向上飞
            p.vel = { 0.f, -400.f };
            p.pos += p.vel * dt;
            p.sprite.setPosition(p.pos);
            p.sprite.setRotation(sf::degrees(std::atan2(p.vel.y, p.vel.x) * 180.f / 3.14159f));
        }

        bool hit = false;
        auto isValid = [](Boss& b) {
            return b.getState() != BossState::Dead && b.getState() != BossState::PhaseWait && b.getState() != BossState::Dying;
            };

        // 命中 Boss 逻辑，命中半径包含 Boss的肉身大小 + 10px(子弹自身宽容度)
        if (isValid(boss) && std::hypot(p.pos.x - boss.getPosition().x, p.pos.y - boss.getPosition().y) < boss.getHitboxRadius() + 10.f) {
            boss.takeDamage(p.damage);
            hit = true;
        }
        else if (currentLevel == Level::Level3 && isValid(boss2) && std::hypot(p.pos.x - boss2.getPosition().x, p.pos.y - boss2.getPosition().y) < boss2.getHitboxRadius() + 10.f) {
            boss2.takeDamage(p.damage);
            hit = true;
        }

        if (hit) {
            feelManager.triggerHeavyHit(); // 每次命中引发极大的画面卡肉震动感
            soundManager.playSound("boss_hurt");
            return true;
        }
        return false;
        });

    // --- 玩家被击中惩罚回调 ---
    auto playerHit = [&](int dmg) {
        int extraData = player.bodyCount - 2;
        player.takeDamage(dmg);

        // 精确记录本体承受伤害与掉落数据
        stats.damageTaken += dmg;
        stats.dataLost += extraData; // 掉血时也会掉光所有额外数据

        shockwaves.push_back(Shockwave(player.headPos)); // 受伤时自动触发免费反击波保命
        feelManager.triggerLightHit();
        soundManager.playSound("player_hurt");

        // 60% 真实战损留底散落
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

    // --- 【超核心碰撞大集成：受击+断尾 系统 (已移除擦弹)】 ---
    auto checkPlayerCollision = [&](std::vector<Bullet>& bullets) {
        for (auto it = bullets.begin(); it != bullets.end(); ) {
            float hitRadius = getBulletRadius(it->type);
            int dmg = 1;

            bool mainBodyHit = false;

            // 提取公共的：检测一个坐标点是否在受击范围内
            auto checkPoint = [&](sf::Vector2f p) {
                float dist = std::hypot(p.x - it->sprite.getPosition().x, p.y - it->sprite.getPosition().y);
                if (dist < hitRadius) {
                    mainBodyHit = true;
                }
                };

            // 1. 首要检测蛇头 (核心区)
            checkPoint(player.headPos);

            // 2. 如果蛇头没死，检测前两节致命身躯
            if (!mainBodyHit) {
                for (int i = 1; i <= 2 && i <= player.bodyCount; ++i) {
                    size_t idx = i * player.gap;
                    if (idx < player.trail.size()) {
                        checkPoint(player.trail[idx].position);
                        if (mainBodyHit) break;
                    }
                }
            }

            // 【情况 A】如果爆头了，触发真实受击并直接结束本次检测！
            if (mainBodyHit) {
                playerHit(dmg);
                it = bullets.erase(it); // 子弹结结实实打在肉上被吸收
                continue;
            }

            // 【情况 B】没爆头，继续向后看：是否切断了尾巴(非致命区)
            bool tailHit = false;
            int cutIndex = -1;
            sf::Vector2f hitPos;
            for (int i = 3; i <= player.bodyCount; ++i) {
                size_t idx = i * player.gap;
                if (idx < player.trail.size()) {
                    sf::Vector2f segmentPos = player.trail[idx].position;
                    // 尾巴只做粗暴的碰撞判定
                    if (std::hypot(segmentPos.x - it->sprite.getPosition().x, segmentPos.y - it->sprite.getPosition().y) < hitRadius) {
                        tailHit = true;
                        cutIndex = i;
                        hitPos = segmentPos;
                        break;
                    }
                }
            }

            if (tailHit) {
                // 算出因为断尾丢了多少节数据
                int extraDataLost = player.bodyCount - cutIndex + 1;
                player.bodyCount = cutIndex - 1;

                // 记录尾巴被切断产生的数据丢失
                stats.dataLost += extraDataLost;

                feelManager.triggerLightHit();
                soundManager.playSound("player_hurt");

                int dropCount = static_cast<int>(std::ceil(extraDataLost * 0.6f));
                for (int i = 0; i < dropCount; ++i) {
                    float angle = (std::rand() % 360) * 3.14159f / 180.f;
                    float dist = 30.f + (std::rand() % 50);
                    sf::Vector2f spawnPos = hitPos + sf::Vector2f(std::cos(angle) * dist, std::sin(angle) * dist);
                    spawnPos.x = std::clamp(spawnPos.x, 20.f, 780.f);
                    spawnPos.y = std::clamp(spawnPos.y, 20.f, 580.f);
                    dataPoints.push_back(DataPoint(spawnPos, 2));
                }

                // 核心变化：不再 erase(it) 删除子弹，让其穿透身体继续沿直线当电锯飞行！
                ++it;
                continue;
            }

            ++it; // 这颗子弹啥也没撞到，继续飞
        }
        };

    // 无敌帧内不进行受击检测
    if (!player.isInvincible) {
        checkPlayerCollision(bossBullets);
        if (!player.isInvincible) { // 避免同时被多发子弹秒杀结算多次
            checkPlayerCollision(bossBullets2);
        }
    }

    if (player.health <= 0 && state != GameState::GameOver) {
        soundManager.stopAllBGM();
        soundManager.setLoopingSound("parsing_loop", false);
        soundManager.playSound("player_dead");
        soundManager.bgmDefeat.play(); // 切换惨烈的失败音乐
        state = GameState::GameOver;
        menuSelection = 0;
    }

    bool b1_dead = boss.isDead();
    bool b2_dead = (currentLevel == Level::Level3) ? boss2.isDead() : true;
    if (b1_dead && b2_dead && state != GameState::Win) {
        soundManager.stopAllBGM();
        soundManager.setLoopingSound("parsing_loop", false);
        soundManager.bgmVictory.play();
        state = GameState::Win;
    }
}

// 统一的画面投射渲染层
void Game::render() {
    // 经典暗色背景
    window.clear(sf::Color(15, 15, 20));

    // 根据背景样式，利用数学函数直接在显存渲染动态特效
    if (state == GameState::Playing || state == GameState::Paused || state == GameState::PausedAudio || state == GameState::GameOver || state == GameState::Win || state == GameState::Settings || state == GameState::SettingsAudio) {
        if (bgStyle == 0) {
            // Cyber Grid - 赛博透视网格
            sf::RectangleShape hLine({ 800.f, 2.f });
            hLine.setFillColor(sf::Color(0, 255, 255, 12));
            sf::RectangleShape vLine({ 2.f, 600.f });
            vLine.setFillColor(sf::Color(0, 255, 255, 12));
            float scrollOffset = static_cast<float>(std::fmod(stats.timeElapsed * 40.f, 40.f));
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
            // Starfield - 向下飞速流动的伪 3D 星空
            for (int i = 0; i < 100; ++i) {
                float x = static_cast<float>(std::fmod(std::sin(i * 12.3f) * 4321.f, 800.f));
                if (x < 0) x += 800.f;
                float speed = 50.f + static_cast<float>(std::fmod(std::cos(i * 3.2f) * 123.f, 100.f));
                float y = static_cast<float>(std::fmod(std::sin(i * 4.5f) * 800.f + stats.timeElapsed * speed, 600.f));
                if (y < 0) y += 600.f;
                sf::CircleShape star(std::fmod(static_cast<float>(i), 3.f) + 1.f);
                star.setPosition({ x, y });
                star.setFillColor(sf::Color(100, 150, 255, 150));
                window.draw(star);
            }
        }
        else if (bgStyle == 2) {
            // Matrix Rain - 黑客帝国绿色数字雨
            sf::Text rainChar(font, L"0", 16);
            for (int i = 0; i < 40; ++i) {
                float x = i * 20.f;
                float speed = 100.f + static_cast<float>(std::fmod(std::sin(i * 1.1f) * 500.f, 150.f));
                float yOffset = static_cast<float>(std::fmod(stats.timeElapsed * speed, 600.f));
                for (int j = 0; j < 10; ++j) {
                    float y = static_cast<float>(std::fmod(std::sin(i * 7.f + j * 3.f) * 600.f + yOffset, 600.f));
                    if (y < 0) y += 600.f;
                    rainChar.setPosition({ x, y });
                    rainChar.setString((i + j + static_cast<int>(stats.timeElapsed * 5.f)) % 2 == 0 ? L"1" : L"0");
                    // 越往上的字符透明度越低，产生拖尾
                    rainChar.setFillColor(sf::Color(0, 255, 0, static_cast<std::uint8_t>(std::max(0, 150 - j * 15))));
                    window.draw(rainChar);
                }
            }
        }
        else if (bgStyle == 3) {
            // Pulse Rings - 中心不断扩散的高科电磁波纹
            sf::CircleShape ring;
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(2.f);
            float t = stats.timeElapsed * 150.f;
            for (int i = 0; i < 6; ++i) {
                float r = static_cast<float>(std::fmod(t + i * 200.f, 1200.f));
                ring.setRadius(r);
                ring.setOrigin({ r, r });
                ring.setPosition({ 400.f, 300.f });
                // 越往外散透明度越低
                float alpha = std::max(0.f, 255.f * (1.f - r / 1200.f));
                ring.setOutlineColor(sf::Color(0, 150, 255, static_cast<std::uint8_t>(alpha * 0.4f)));
                window.draw(ring);
            }
        }
    }

    // 摄像机镜头渲染层：只在游玩时处理打击反馈带来的偏移震动
    sf::View view = window.getView();
    sf::Vector2f shakeOffset = { 0.f, 0.f };
    if (state == GameState::Playing) {
        shakeOffset = feelManager.getShakeOffset();
    }
    // 把屏幕焦点加上随机震动
    view.setCenter({ 400.f + shakeOffset.x, 300.f + shakeOffset.y });
    window.setView(view);

    sf::Text title(font, L"", 40);
    sf::Text opt1(font, L"", 24);
    sf::Text opt2(font, L"", 24);
    sf::Text opt3(font, L"", 24);
    sf::Text opt4(font, L"", 24);

    if (state == GameState::Playing || state == GameState::Paused || state == GameState::PausedAudio) {
        boss.draw(window);
        if (currentLevel == Level::Level3) boss2.draw(window);

        for (auto& dp : dataPoints) {
            window.draw(dp.glow);
            window.draw(dp.shape);
        }

        for (auto& sw : shockwaves) window.draw(sw.shape);
        for (auto& p : snakeProjectiles) window.draw(p.sprite);
        player.draw(window); // 玩家层放在最前面

        // 【屏幕穿梭警告边框UI渲染】(在UI图层下，但在主场景之上)
        if (borderFlashTimer > 0.f) {
            int flashCycle = static_cast<int>(borderFlashTimer * 10) % 2;
            if (flashCycle == 0) {
                sf::RectangleShape border({ 800.f, 600.f });
                border.setFillColor(sf::Color::Transparent);
                border.setOutlineColor(sf::Color(0, 100, 255, 150));
                border.setOutlineThickness(-8.f); // 负数代表向内渲染厚度，避免出界
                window.draw(border);
            }
        }

        // --- 静态 UI 抬头显示层 (HUD) ---
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

        sf::Text phaseText(font, T("hud_phase"), 14); phaseText.setPosition({ 620.f, 20.f }); phaseText.setFillColor(sf::Color::White); window.draw(phaseText);
        for (int i = 0; i < 3; ++i) {
            sf::RectangleShape pb({ 30.f, 12.f }); pb.setPosition({ 680.f + i * 35.f, 22.f });
            pb.setFillColor(i <= boss.getCurrentPhase() ? sf::Color::Red : sf::Color(50, 0, 0));
            if (i == boss.getCurrentPhase()) { pb.setOutlineThickness(2.f); pb.setOutlineColor(sf::Color::Yellow); }
            window.draw(pb);
        }

        // --- 暂停界面弹出渲染层 ---
        if (state == GameState::Paused || state == GameState::PausedAudio) {
            // 黑底半透明遮罩幕布
            sf::RectangleShape overlay({ 800.f, 600.f }); overlay.setFillColor(sf::Color(0, 0, 0, 150)); window.draw(overlay);

            if (state == GameState::Paused) {
                title.setString(T("pa_title")); centerText(title, 200.f); window.draw(title);

                // 借用翻译引擎 T() 生成所有的外显文本
                opt1.setString(T("pa_cont"));
                opt2.setString(T("set_aud"));
                opt3.setString(T("pa_ret"));
                opt4.setString(T("pa_skip"));
                sf::Text opt5(font, T("set_lang") + (currentLang == Language::EN ? L"English" : L"中文"), 24);

                // 根据游标高亮当前选中的条目
                opt1.setFillColor(menuSelection == 0 ? sf::Color::Yellow : sf::Color::White);
                opt2.setFillColor(menuSelection == 1 ? sf::Color::Yellow : sf::Color::White);
                opt3.setFillColor(menuSelection == 2 ? sf::Color::Yellow : sf::Color::White);
                opt4.setFillColor(menuSelection == 3 ? sf::Color::Yellow : sf::Color::White);
                opt5.setFillColor(menuSelection == 4 ? sf::Color::Yellow : sf::Color::White);

                centerText(opt1, 300.f); centerText(opt2, 340.f); centerText(opt3, 380.f); centerText(opt4, 420.f); centerText(opt5, 460.f);
                window.draw(opt1); window.draw(opt2); window.draw(opt3); window.draw(opt4); window.draw(opt5);
            }
            else if (state == GameState::PausedAudio) {
                title.setString(T("aud_title")); centerText(title, 200.f); window.draw(title);
                std::wstring bgmStr, sfxStr;
                getBarString(bgmVolInt, bgmStr);
                getBarString(sfxVolInt, sfxStr);

                opt1.setString(T("aud_b") + bgmStr);
                opt2.setString(T("aud_s") + sfxStr);
                opt3.setString(T("aud_back"));
                opt1.setFillColor(audioSelection == 0 ? sf::Color::Yellow : sf::Color::White);
                opt2.setFillColor(audioSelection == 1 ? sf::Color::Yellow : sf::Color::White);
                opt3.setFillColor(audioSelection == 2 ? sf::Color::Yellow : sf::Color::White);
                centerText(opt1, 300.f); centerText(opt2, 350.f); centerText(opt3, 400.f);
                window.draw(opt1); window.draw(opt2); window.draw(opt3);
            }
        }
    }
    // --- 【死亡结算展示层 (展示战损)】 ---
    else if (state == GameState::GameOver) {
        title.setString(T("go_title"));
        title.setFillColor(sf::Color::Red);
        centerText(title, 150.f); window.draw(title);

        sf::Text stat1(font, T("go_t") + std::to_wstring((int)stats.timeElapsed) + L"s", 18);
        sf::Text stat2(font, T("go_l") + std::to_wstring(stats.maxLength), 18);
        sf::Text stat3(font, T("go_d") + std::to_wstring(stats.damageTaken), 18);
        sf::Text stat4(font, T("go_loss") + std::to_wstring(stats.dataLost), 18);

        centerText(stat1, 200.f); window.draw(stat1);
        centerText(stat2, 230.f); window.draw(stat2);
        centerText(stat3, 260.f); window.draw(stat3);
        centerText(stat4, 290.f); window.draw(stat4);

        opt1.setString(T("go_r1"));
        opt2.setString(T("go_r2"));
        opt3.setString(T("go_ret"));

        opt1.setFillColor(menuSelection == 0 ? sf::Color::Yellow : sf::Color::White);
        opt2.setFillColor(menuSelection == 1 ? sf::Color::Yellow : sf::Color::White);
        opt3.setFillColor(menuSelection == 2 ? sf::Color::Yellow : sf::Color::White);

        centerText(opt1, 350.f); window.draw(opt1);
        centerText(opt2, 400.f); window.draw(opt2);
        centerText(opt3, 450.f); window.draw(opt3);
    }
    // --- 【胜利结算展示层】 ---
    else if (state == GameState::Win) {
        title.setString(T("wi_title"));
        title.setFillColor(sf::Color::Green);
        centerText(title, 150.f); window.draw(title);

        sf::Text stat1(font, T("go_t") + std::to_wstring((int)stats.timeElapsed) + L"s", 18);
        sf::Text stat2(font, T("go_l") + std::to_wstring(stats.maxLength), 18);
        sf::Text stat3(font, T("go_d") + std::to_wstring(stats.damageTaken), 18);
        sf::Text stat4(font, T("go_loss") + std::to_wstring(stats.dataLost), 18);

        centerText(stat1, 220.f); window.draw(stat1);
        centerText(stat2, 260.f); window.draw(stat2);
        centerText(stat3, 300.f); window.draw(stat3);
        centerText(stat4, 340.f); window.draw(stat4);

        sf::Text hint(font, T("set_ret"), 18);
        centerText(hint, 450.f); window.draw(hint);
    }
    else {
        // --- 静态纯 UI 菜单层轮播 ---
        if (state == GameState::Menu) {
            title.setString(T("m_title"));
            centerText(title, 150.f); window.draw(title);

            opt1.setString(T("m_start"));
            opt2.setString(T("m_skin"));
            opt3.setString(T("m_set"));
            opt4.setString(T("m_exit"));

            opt1.setFillColor(menuSelection == 0 ? sf::Color::Yellow : sf::Color::White);
            opt2.setFillColor(menuSelection == 1 ? sf::Color::Yellow : sf::Color::White);
            opt3.setFillColor(menuSelection == 2 ? sf::Color::Yellow : sf::Color::White);
            opt4.setFillColor(menuSelection == 3 ? sf::Color::Yellow : sf::Color::White);

            centerText(opt1, 280.f); window.draw(opt1);
            centerText(opt2, 330.f); window.draw(opt2);
            centerText(opt3, 380.f); window.draw(opt3);
            centerText(opt4, 430.f); window.draw(opt4);

            // 绘制主页两侧的装饰性 Boss 立绘图 (调用了切片函数实现呼吸动画)
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
            title.setString(T("set_title"));
            centerText(title, 150.f); window.draw(title);

            sf::Text sub(font, T("set_sub"), 20); centerText(sub, 220.f); window.draw(sub);

            opt1.setString(T("set_lang") + (currentLang == Language::EN ? L"English" : L"中文"));

            std::wstring bgName = T("bg_0");
            if (bgStyle == 1) bgName = T("bg_1");
            else if (bgStyle == 2) bgName = T("bg_2");
            else if (bgStyle == 3) bgName = T("bg_3");
            opt2.setString(T("set_bg") + bgName);

            std::wstring bgmName = T("bgm_rand");
            if (bgmStyle == 0) bgmName = L"Algromith";
            else if (bgmStyle == 1) bgmName = L"BattleRevert";
            else if (bgmStyle == 2) bgmName = L"EventTypeA";
            opt3.setString(T("set_bgm") + bgmName);

            opt4.setString(T("set_aud"));

            opt1.setFillColor(settingsSelection == 0 ? sf::Color::Yellow : sf::Color::White);
            opt2.setFillColor(settingsSelection == 1 ? sf::Color::Yellow : sf::Color::White);
            opt3.setFillColor(settingsSelection == 2 ? sf::Color::Yellow : sf::Color::White);
            opt4.setFillColor(settingsSelection == 3 ? sf::Color::Yellow : sf::Color::White);

            centerText(opt1, 300.f); window.draw(opt1);
            centerText(opt2, 340.f); window.draw(opt2);
            centerText(opt3, 380.f); window.draw(opt3);
            centerText(opt4, 420.f); window.draw(opt4);

            sf::Text hint(font, T("set_ret"), 18); centerText(hint, 480.f); window.draw(hint);
        }
        else if (state == GameState::SettingsAudio) {
            title.setString(T("aud_title"));
            centerText(title, 150.f); window.draw(title);

            std::wstring bgmStr, sfxStr;
            getBarString(bgmVolInt, bgmStr);
            getBarString(sfxVolInt, sfxStr);

            opt1.setString(T("aud_b") + bgmStr);
            opt2.setString(T("aud_s") + sfxStr);
            opt3.setString(T("aud_back"));

            opt1.setFillColor(audioSelection == 0 ? sf::Color::Yellow : sf::Color::White);
            opt2.setFillColor(audioSelection == 1 ? sf::Color::Yellow : sf::Color::White);
            opt3.setFillColor(audioSelection == 2 ? sf::Color::Yellow : sf::Color::White);
            centerText(opt1, 300.f); centerText(opt2, 350.f); centerText(opt3, 400.f);
            window.draw(opt1); window.draw(opt2); window.draw(opt3);
        }
        else if (state == GameState::SkinSelect) {
            title.setString(T("sk_title"));
            centerText(title, 150.f); window.draw(title);

            sf::Text sub(font, T("sk_sub"), 20); centerText(sub, 220.f); window.draw(sub);
            sf::Sprite previewHead(snakeHeadTex); previewHead.setPosition({ 370.f, 300.f }); window.draw(previewHead);
            sf::Sprite previewBody(snakeBodyTexs[currentSkinIndex]); previewBody.setPosition({ 410.f, 300.f }); window.draw(previewBody);

            opt1.setString(T("sk_cur") + std::to_wstring(currentSkinIndex + 1));
            opt1.setFillColor(sf::Color::Yellow);
            centerText(opt1, 380.f); window.draw(opt1);

            sf::Text hint(font, T("set_ret"), 18); centerText(hint, 450.f); window.draw(hint);
        }
        else if (state == GameState::LevelSelect) {
            title.setString(T("lv_title"));
            centerText(title, 150.f); window.draw(title);

            opt1.setString(T("lv_1"));
            opt2.setString(T("lv_2"));
            opt3.setString(T("lv_3"));
            opt1.setFillColor(menuSelection == 0 ? sf::Color::Yellow : sf::Color::White);
            opt2.setFillColor(menuSelection == 1 ? sf::Color::Yellow : sf::Color::White);
            opt3.setFillColor(menuSelection == 2 ? sf::Color::Yellow : sf::Color::White);

            centerText(opt1, 280.f); window.draw(opt1);
            centerText(opt2, 330.f); window.draw(opt2);
            centerText(opt3, 380.f); window.draw(opt3);

            // 让当前选中的关卡所对应的立绘在屏幕两侧浮现显示
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
            title.setString(T("df_title"));
            centerText(title, 150.f); window.draw(title);

            opt1.setString(T("df_e"));
            opt2.setString(T("df_n"));
            opt3.setString(T("df_h"));
            opt1.setFillColor(menuSelection == 0 ? sf::Color::Yellow : sf::Color::White);
            opt2.setFillColor(menuSelection == 1 ? sf::Color::Yellow : sf::Color::White);
            opt3.setFillColor(menuSelection == 2 ? sf::Color::Yellow : sf::Color::White);

            centerText(opt1, 280.f); window.draw(opt1);
            centerText(opt2, 330.f); window.draw(opt2);
            centerText(opt3, 380.f); window.draw(opt3);
        }
    }

    view.setCenter({ 400.f, 300.f });
    window.setView(view);

    window.display(); // 呈递给 GPU 进行显示器渲染
}

// 主干引擎心跳，包含帧率统一测算
void Game::run() {
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        processEvents();
        update(dt);
        render();
    }
}