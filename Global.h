#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <cmath>
#include <cstdlib>
#include <list>
#include <string>
#include <unordered_map>
#include <iostream>

// ==========================================
// 【全局枚举定义】
// ==========================================

// 游戏支持的语言列表：EN(英文), ZH(中文)
enum class Language { EN, ZH };

// 游戏的状态机枚举。决定了当前屏幕应该渲染什么，以及如何响应玩家的按键。
enum class GameState {
    Menu,           // 主菜单
    SkinSelect,     // 皮肤选择界面
    LevelSelect,    // 关卡选择界面
    DiffSelect,     // 难度选择界面
    Settings,       // 综合设置界面 (背景、语言等)
    SettingsAudio,  // 音频设置子界面
    Playing,        // 核心游玩中状态
    Paused,         // 游戏暂停
    PausedAudio,    // 暂停时的音频设置子界面
    GameOver,       // 游戏失败 (玩家死亡)结算界面
    Win             // 游戏胜利结算界面
};

// 游戏难度，直接影响Boss的血量以及发射弹幕的频率
enum class Difficulty { Easy, Normal, Hard };

// 关卡选择。Level1=单Honest, Level2=单Hime, Level3=双Boss联动
enum class Level { Level1, Level2, Level3 };

// ==========================================
// 【全局数据结构】
// ==========================================

// 用于记录玩家单局游戏表现的统计数据，展示在结算画面
struct GameStats {
    float timeElapsed = 0.f; // 存活/通关时间(秒)
    int damageTaken = 0;     // 玩家本体(蛇头)受到伤害的次数
    int maxLength = 2;       // 本局达到的最大长度(体现玩家的贪心程度)
    int dataLost = 0;        // 累计丢失的数据节数(断尾+扣血掉落)，体现战损
};

// 简单的坐标结构体，用于记录蛇身历史轨迹点
struct Pose { sf::Vector2f position; };

// 帧动画信息配置体，用于管理Boss的各种切片动画
struct AnimInfo {
    const sf::Texture* tex = nullptr; // 绑定的精灵图集
    int cols = 1;                     // 图集有多少列
    int rows = 1;                     // 图集有多少行
    int totalFrames = 1;              // 该动作总共包含多少个有效帧
    float fps = 10.f;                 // 播放速度 (Frames Per Second)
};

// ==========================================
// 【打击感管理器 (Game Feel Manager)】
// ==========================================
// 负责处理现代动作游戏必备的“屏幕震动 (Screen Shake)”与“顿帧/卡肉 (Hit Stop)”效果。
class GameFeelManager {
public:
    float shakeIntensity = 0.f; // 当前震动强度 (决定偏移像素的大小)
    float freezeTime = 0.f;     // 顿帧剩余时间 (大于0时画面定格)

    // 触发重击效果 (用于Boss受伤、高伤武器命中等)
    // 产生强烈的屏幕震动，并让游戏逻辑“卡住”0.06秒，增加打击的反馈重量感。
    void triggerHeavyHit() {
        shakeIntensity = 12.0f;
        freezeTime = 0.06f;
    }

    // 触发轻击效果 (用于玩家擦伤、普通受击、断尾等)
    // 只有中等震动，不产生顿帧，以免打断玩家的连续躲避节奏。
    void triggerLightHit() {
        shakeIntensity = 4.0f;
    }

    // 每帧更新打击感状态
    void update(float dt) {
        // 如果处于顿帧状态，则消耗顿帧时间
        if (freezeTime > 0.f) freezeTime -= dt;
        // 如果屏幕正在震动，则以每秒30像素的衰减率逐渐平息震动
        if (shakeIntensity > 0.f) {
            shakeIntensity -= 30.f * dt;
            if (shakeIntensity < 0.f) shakeIntensity = 0.f; // 防止减到负数
        }
    }

    // 外部查询接口：当前是否处于“顿帧/卡肉”状态。
    // 如果返回 true，外部的实体(玩家、Boss、子弹)就不应该更新位置，从而实现画面定格。
    bool isFrozen() const { return freezeTime > 0.f; }

    // 获取当前帧的画面震动偏移量 (用于直接加在 sf::View 摄像机上)
    sf::Vector2f getShakeOffset() {
        if (shakeIntensity <= 0.f) return { 0.f, 0.f };
        // 生成 -shakeIntensity 到 +shakeIntensity 之间的随机 XY 偏移
        float offsetX = ((std::rand() % 100) / 100.f - 0.5f) * 2.f * shakeIntensity;
        float offsetY = ((std::rand() % 100) / 100.f - 0.5f) * 2.f * shakeIntensity;
        return { offsetX, offsetY };
    }
};

// ==========================================
// 【音频管理器 (Sound Manager)】
// ==========================================
// 集中管理游戏中所有的背景音乐(BGM)、环境音(Ambient)和短促音效(SFX)。
class SoundManager {
public:
    // 存储所有短促音效的音频数据 (加载到内存中)
    std::unordered_map<std::string, sf::SoundBuffer> buffers;
    // 防重复播放的冷却计时器 (防止同一帧触发多次相同音效爆音)
    std::unordered_map<std::string, sf::Clock> cooldowns;
    // 正在播放中的一次性音效列表 (用 list 方便在迭代时安全删除)
    std::list<sf::Sound> activeSounds;
    // 需要持续循环播放的特殊音效 (如玩家蓄力解析时的嗡嗡声)
    std::unordered_map<std::string, sf::Sound> loopingSounds;

    // 各种独立的 BGM 播放器 (音乐不加载到内存，而是流式读取，节省RAM)
    sf::Music bgmMenu;          // 主菜单音乐
    sf::Music bgmBattle[3];     // 三首可选的战斗音乐
    sf::Music bgmVictory;       // 胜利结算音乐
    sf::Music bgmDefeat;        // 失败结算音乐
    sf::Music bgmAmbient;       // 底层环境白噪音 (增强赛博空间氛围)

    int currentBattleBgm = 0;   // 当前正在播放的战斗音乐索引
    int bgmSelection = 3;       // 玩家设置的音乐偏好 (3 = 随机播放)
    bool inBattle = false;      // 当前是否处于战斗状态，用于管理切歌逻辑

    // 全局音量倍率 (由 Settings 菜单调整，范围 0.0 ~ 1.0)
    float bgmVolumeMulti = 1.0f;
    float sfxVolumeMulti = 1.0f;

    // 加载音效文件到内存 Buffer 中
    void loadBuffer(const std::string& name, const std::string& path) {
        sf::SoundBuffer buffer;
        if (buffer.loadFromFile(path)) {
            buffers[name] = buffer;
        }
    }

    // 播放指定名称的一次性音效
    void playSound(const std::string& name, float baseVol = 100.f) {
        if (buffers.count(name)) {
            // 防爆音：如果同一个音效距离上次播放还不到 0.05 秒，则忽略本次请求
            if (cooldowns[name].getElapsedTime().asSeconds() < 0.05f) return;
            cooldowns[name].restart();

            // SFML 3: sf::Sound 必须使用带有 sf::SoundBuffer 的构造函数初始化
            activeSounds.emplace_back(buffers.at(name));
            // 应用基础音量和全局 SFX 音量倍率
            activeSounds.back().setVolume(baseVol * sfxVolumeMulti);
            activeSounds.back().play();
        }
    }

    // 控制持续/循环音效的播放与停止 (例如：按住 D 键时的蓄力音)
    void setLoopingSound(const std::string& name, bool play) {
        if (!buffers.count(name)) return;
        if (play) {
            auto it = loopingSounds.find(name);
            if (it == loopingSounds.end()) {
                it = loopingSounds.emplace(name, sf::Sound(buffers.at(name))).first;
            }
            // 如果还未播放，则开启循环并播放
            if (it->second.getStatus() != sf::SoundSource::Status::Playing) {
                it->second.setLooping(true);
                it->second.setVolume(100.f * sfxVolumeMulti);
                it->second.play();
            }
        }
        else {
            // 停止播放并重置状态
            auto it = loopingSounds.find(name);
            if (it != loopingSounds.end()) {
                it->second.stop();
            }
        }
    }

    // 刷新所有的正在播放的音量大小 (用于玩家在暂停菜单调整音量后立刻生效)
    void updateVolumes() {
        bgmMenu.setVolume(30.f * bgmVolumeMulti);
        for (int i = 0; i < 3; ++i) bgmBattle[i].setVolume(30.f * bgmVolumeMulti);
        bgmVictory.setVolume(40.f * bgmVolumeMulti);
        bgmDefeat.setVolume(40.f * bgmVolumeMulti);
        bgmAmbient.setVolume(15.f * bgmVolumeMulti); // 环境音极低，仅作氛围铺垫

        // 更新所有循环音效的音量
        for (auto& pair : loopingSounds) {
            pair.second.setVolume(100.f * sfxVolumeMulti);
        }
        // activeSounds(一次性音效) 的音量由 playSound 生成时决定，无需在此刷新
    }

    // 每帧更新音频管理器
    void update() {
        // 清理已经播放完毕的一次性音效，释放 list 节点内存
        activeSounds.remove_if([](const sf::Sound& s) {
            return s.getStatus() == sf::SoundSource::Status::Stopped;
            });

        // 战斗状态下，处理音乐列表的自动连播/单曲循环机制
        if (inBattle) {
            // 当当前音乐播放自然停止时
            if (bgmBattle[currentBattleBgm].getStatus() == sf::SoundSource::Status::Stopped) {
                // 如果是随机模式，则切到下一首不重复的歌
                if (bgmSelection == 3) {
                    int next = std::rand() % 3;
                    if (next == currentBattleBgm) next = (next + 1) % 3;
                    currentBattleBgm = next;
                }
                // 如果是指定模式，则直接重新 play() 实现单曲循环
                bgmBattle[currentBattleBgm].play();
            }
        }
    }

    // 回到主菜单时调用的 BGM 控制逻辑
    void playMenuBGM() {
        inBattle = false;
        for (int i = 0; i < 3; ++i) bgmBattle[i].stop();
        bgmVictory.stop();
        bgmDefeat.stop();

        // 保证主菜单音乐不会被重复 trigger 导致重头播放
        if (bgmMenu.getStatus() != sf::SoundSource::Status::Playing) {
            bgmMenu.play();
            bgmMenu.setLooping(true); // 主菜单无限循环
        }
    }

    // 进入战斗关卡时调用的 BGM 控制逻辑
    void playBattleBGM(int style) {
        inBattle = true;
        bgmMenu.stop();
        for (int i = 0; i < 3; ++i) bgmBattle[i].stop();
        bgmVictory.stop();
        bgmDefeat.stop();

        bgmSelection = style;
        // 如果是模式 3 (随机)，则掷骰子决定首发歌曲
        if (style == 3) currentBattleBgm = std::rand() % 3;
        else currentBattleBgm = style;

        bgmBattle[currentBattleBgm].play();
    }

    // 在设置菜单里快速切换 BGM 时用于实时试听的函数
    void previewBGM(int style) {
        inBattle = false; // 试听不视为真正进入战斗
        bgmMenu.stop();
        for (int i = 0; i < 3; ++i) bgmBattle[i].stop();
        bgmVictory.stop();
        bgmDefeat.stop();

        if (style >= 0 && style <= 2) {
            bgmBattle[style].setLooping(true); // 试听期间强行将其设为循环
            bgmBattle[style].play();
        }
        else {
            // 切到 Random 选项时，播放主菜单 BGM 作为保底
            bgmMenu.setLooping(true);
            bgmMenu.play();
        }
    }

    // 强制掐断所有前台背景音乐 (常用于准备播放结算音乐前)
    void stopAllBGM() {
        inBattle = false;
        bgmMenu.stop();
        for (int i = 0; i < 3; ++i) bgmBattle[i].stop();
    }
};