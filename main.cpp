#include "Game.h"
#include <ctime>
#include <cstdlib>

int main() {
    // 【系统初始化】
    // 使用当前时间作为随机数种子，确保每次游戏启动时，
    // Boss的弹幕随机轨迹、环境数据的散落位置、背景特效等都是完全随机的。
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // 实例化主游戏类。这会在内存中分配窗口、加载所有资源、初始化管理器。
    Game game;

    // 启动游戏主循环。在这个函数返回之前，程序会一直运行，直到玩家关闭窗口。
    game.run();

    return 0;
}