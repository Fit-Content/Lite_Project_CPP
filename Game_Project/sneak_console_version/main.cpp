#include <iostream>
#include <Windows.h>
#include <list>
#include <thread>
#include <chrono>
using namespace std;
using namespace std::chrono_literals;

const int nScreenWidth = 80;  // 屏幕宽度
const int nScreenHeight = 30; // 屏幕高度

// 表示蛇的一个部分，其实就是位于游戏区域中的某个坐标
struct sSnakeSegment
{
    int x;
    int y;
};

int main()
{
#pragma region 屏幕初始化
    // 保存命令行缓存数据的内存咯，将它抽象成一个二维数组即可
    // 保险起见，可以使用 TCHAR* 定义，然后开启 UNICODE 宏从而支持中文
    wchar_t *screen = new wchar_t[nScreenWidth * nScreenHeight];
    // 此处通常还会对内存进行初始化啦
    for (int i = 0; i < nScreenWidth * nScreenHeight; i++)
    {
        screen[i] = L' '; // 初始化为空格
    }
    // 如它的名字，Console Buffer
    // 创建一个指向 console 缓冲区的句柄，具备 "READ、WRITE" 权限
    HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    // 将这个 buffer 设置成控制台的缓冲区
    SetConsoleActiveScreenBuffer(hConsole);
    DWORD dwBytesWritten = 0;
#pragma endregion

    // 包裹住内部的游戏循环，当 game over 时可以按空格键继续游戏
    while (true)
    {
#pragma region 游戏数据
        // 蛇的各个部分,也就是表示一条蛇咯，并指定初始大小、位置。
        list<sSnakeSegment> snake{
            {10, 10}, {10, 11}, {10, 12}, {10, 13}, {10, 14}}; // 初始情况下 snake 只有 2 部分

        // 食物的位置，以下是初始情况
        int nFoodX = 30;
        int nFoodY = 10;
        int nScore = 0;          // 记录分数
        int nSnakeDirection = 0; // 蛇的方向，0 代表向上，1 代表向右，2 代表向下，3 代表向左
        bool bDead = false;      // 是否死亡，也表示游戏是否结束

        // 按键控制，只有左右两个按键，其实就是按顺时针或逆时针修改蛇的前进方向
        // 蛇肯定不会直接 180 度转向
        bool bKeyLeft = false;
        bool bKeyLeftOld = false; // 记录上一次按键状态，只有在改变状态时才会更新蛇的方向

        bool bKeyRight = false;
        bool bKeyRighttOld = false; // 记录上一次按键状态

#pragma endregion

        // 游戏循环
        while (!bDead)
        {

#pragma region 游戏帧率与键盘输入
            // 每隔一段时间 snake 会自动前进，所以设置帧率
            // 使用帧率并不好，因为如果玩家快速按键也不会及时得到响应
            // this_thread::sleep_for(200ms);

            // 现在使用计时器，在 200ms 内捕获多次按键，这样远远比暂停整个游戏 200ms 再捕获按键好一些
            // 因为现在是整个程序就在此处停了 200ms 等待输入。
            // 而此前的 this_thread::sleep_for(200ms) 是直接暂停 200ms 什么都不做呢
            auto t1 = chrono::system_clock::now();
            // 也正是因为这个循环 “阻塞” 了游戏 200ms，所以 200ms 也是帧率哟
            // 现在又需要改进，因为在控制台中向下移动时会显得更块（字符没 有挤在一起的原因）
            // 所以根据蛇的移动方向来确定不同的停止时间
            const auto nFrameTime = (nSnakeDirection % 2 == 1) ? 120ms : 200ms;
            while ((chrono::system_clock::now() - t1) < nFrameTime)
            {
                bKeyRight = (GetAsyncKeyState((unsigned char)'A') & 0x8000) != 0;
                bKeyLeft = (GetAsyncKeyState((unsigned char)'D') & 0x8000) != 0;
                if (bKeyRight && !bKeyRighttOld)
                {
                    nSnakeDirection++;        // 顺时针转向，即右转
                    if (nSnakeDirection == 4) // 表示转向了 360 度，需要重新归零
                        nSnakeDirection = 0;
                }
                if (bKeyLeft && !bKeyLeftOld)
                {
                    nSnakeDirection--; // 左转
                    if (nSnakeDirection == -1)
                        nSnakeDirection = 3;
                }
                bKeyLeftOld = bKeyLeft;
                bKeyRighttOld = bKeyRight;
            }

#pragma endregion

#pragma region 游戏逻辑
            // 蛇的移动，其实就是修改 list 中的坐标
            // 根据蛇的方向，在蛇的头部（即链表头部）添加新的坐标，从而表示一次移动
            switch (nSnakeDirection)
            {
            case 0: // 向上
                snake.push_front({snake.front().x, snake.front().y - 1});
                break;

            case 1: // 向右
                snake.push_front({snake.front().x + 1, snake.front().y});
                break;

            case 2: // 向下
                snake.push_front({snake.front().x, snake.front().y + 1});
                break;

            case 3: // 向左
                snake.push_front({snake.front().x - 1, snake.front().y});
                break;
            }
            // 然后还需要删除尾部，因为已经向前移动了一步，所以尾部的坐标就不再需要了
            snake.pop_back();

            // =============== 碰撞检测
            // 1. 游戏的边界检测
            // 蛇头的 x 坐标需要在游戏区域内，同理还有 y 坐标
            if (snake.front().x < 0 || snake.front().x >= nScreenWidth)
                bDead = true; // 碰到边界，死亡
            // 为什么 y < 3 呢？？因为游戏区域的前面 3 行用于输出得分信息了
            if (snake.front().y < 3 || snake.front().y >= nScreenHeight)
                bDead = true; // 碰到边界，死亡

            // 2. 蛇的自身检测，也就是当蛇的头部碰到自身时 game over
            // 这种情况下，检测蛇身的所有坐标，如果和蛇头的坐标重复，就说明发生了碰撞
            for (auto it = snake.begin(); it != snake.end(); it++)
            {
                // 既然是拿蛇头的坐标和其它的蛇身坐标进行比较
                // 所以需要先排除蛇头的自身的坐标嘛
                if (it != snake.begin() && it->x == snake.front().x && it->y == snake.front().y)
                {
                    bDead = true; // 碰到自己，死亡
                }
            }

            // 3. 食物的检测
            // 首先，是蛇的头部碰到了食物
            if (snake.front().x == nFoodX && snake.front().y == nFoodY)
            {
                nScore++; // 吃到食物，得分加 1

                // 随机生成食物的位置！为了避免食物生成的位置和蛇身重合，所以需要循环判断（因为生成时是随机的）
                // 此时，进入循环中时，食物所在的位置就是 '*' 咯
                while (screen[nFoodX + nFoodY * nScreenWidth] != L' ')
                {
                    // 生成食物新的 x、y 坐标
                    nFoodX = rand() % nScreenWidth;
                    // 随机生成食物的位置，但不能在游戏区域的前面 3 行
                    // rand() % (nScreenHeight - 3) 就是生成一个 0 到 nScreenHeight - 3 之间的随机数
                    // 然后加 3 是为了保证生成的坐标不在游戏区域的前面 3 行，因为那里是展示的得分区域
                    nFoodY = (rand() % (nScreenHeight - 3)) + 3;
                }
                // 然后，在蛇的尾部添加 5 个新的坐标，表示吃到食物后，蛇的长度增加 5
                // 注意咯，现在这添加的 5 个坐标是重叠的！当蛇前进时，尾部的坐标会被删除掉
                // 具体看游戏效果演示，更像是凭空长出来的
                for (int i = 0; i < 5; i++)
                    snake.push_back({snake.back().x, snake.back().y});
            }
#pragma endregion

#pragma region 游戏绘制
            // 清空屏幕
            for (int i = 0; i < nScreenWidth * nScreenHeight; i++)
            {
                screen[i] = L' ';
            }

            // 绘制游戏区域的边界
            for (int i = 0; i < nScreenWidth; i++)
            {
                // 第 i 列的第一个元素（也就是第一行都是边框)
                screen[i] = L'=';
                // 第 3 行也是边框
                screen[i + nScreenWidth * 2] = L'=';
            }
            // 绘制分输，它比游戏区域宽一些，它输出在第 2 行
            wsprintfW(&screen[nScreenWidth + 5], L"Score: %d", nScore);

            // 绘制蛇，其实就是把 list 中的坐标都画出来
            for (auto &seg : snake)
            {
                // 游戏结束时将蛇画成 x
                if (bDead)
                {
                    screen[seg.x + seg.y * nScreenWidth] = L'+';
                }
                else
                {
                    screen[seg.x + seg.y * nScreenWidth] = L'O';
                }
            }
            // 但是呢，蛇的头部需要更具有标识性，换一个字符显示，同时简化了方面代码
            screen[snake.front().x + snake.front().y * nScreenWidth] = (bDead ? L'X' : L'@');

            // 绘制食物
            screen[nFoodX + nFoodY * nScreenWidth] = L'*';

            // 需要保证字符串的末尾是空字符
            screen[nScreenWidth * nScreenHeight - 1] = '\0';

            // 游戏结束
            if (bDead)
            {
                // 在 screen[15][40] 的位置输出信息
                wsprintfW(&screen[nScreenWidth * 15 + 40], L"Game Over!");
            }

            // 输出到控制台中，注意使用 W 版本，因为 screen 是 wchar_t
            // 当然也可以使用 windows 类型 TCHAR 来定义 screen 啦
            WriteConsoleOutputCharacterW(hConsole,
                                         screen,
                                         nScreenWidth * nScreenHeight,
                                         // 指定要写入到控制台的位置（也就是偏移量）
                                         // 通常就是在控制台的左上角
                                         {0, 0},
                                         // 记录输出的字符串
                                         &dwBytesWritten);

#pragma endregion
        }

        // 等待空格键按下，然后重新开始游戏
        while ((GetAsyncKeyState((unsigned char)' ') & 0x8000) == 0)
        {
            bDead = false;
        }
    }
    return 0;
}