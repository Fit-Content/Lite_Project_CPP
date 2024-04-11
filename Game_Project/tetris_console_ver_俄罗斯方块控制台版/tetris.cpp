#include <iostream>
#include <Windows.h>
#include <chrono>
#include <thread>
#include <vector>
using namespace std::chrono_literals;

#pragma region 游戏数据设置
const int nScreenWidth = 80;  // 屏幕宽度
const int nScreenHeight = 30; // 屏幕高度
// 保存方块，共有 7 种，提前规定：每个方块是一个 4 x 4 的矩阵
std::wstring tetromino[7];

// 定义游戏区域的大小
const int nFieldWidth = 12;
const int nFieldHeight = 18;
// 在堆上开辟内存，保存游戏区域中的字符咯
// 所以需要一个指针
unsigned char *pField = nullptr;

#pragma endregion

#pragma region 辅助函数
/* 对数组的 px、py 位置的元素进行旋转，其中 r 表示旋转的度数。
    r = 0：不旋转
    r = 1：顺时针旋转 90 度
    r = 2：逆时针旋转 180 度
    r = 3：逆时针旋转 270 度
    具体无法用语言说明，需要看文档画图。
 */
int Rotate(int px, int py, int r)
{
    switch (r % 4)
    {
    case 0:
        return py * 4 + px; // 原始位置
    case 1:
        return 12 + py - (px * 4); // 顺时针旋转 90 度
    case 2:
        return 15 - (py * 4) - px; // 逆时针旋转 180 度
    case 3:
        return 3 - py + (px * 4); // 逆时针旋转 270 度
    }
    return 0;
}

// 碰撞检测：判断方块是否可以放置在 px 和 py 位置
// nTetromino 是索引，表示哪种方块，用于在 tetromino 索引啦
// nRotation 表示方块将要旋转的角度，0 表示不旋转，1 表示 90 度，2 表示 180 度咯
// nPosX 和 nPosY 表示方块将要放置的位置，以方块左上角为基准
bool DoesPieceFit(int nTetromino, int nRotation, int nPosX, int nPosY)
{
    // 首先遍历整个方块区域，它是一个 4x4 的字符串哟
    for (int px = 0; px < 4; px++)
    {
        for (int py = 0; py < 4; py++)
        {
            // 计算区域中某个小格旋转后的坐标
            int pi = Rotate(px, py, nRotation);

            // 计算旋转后，这个小格子在游戏区域中的坐标
            // 该小格子在游戏区域中的第 nPosY + py 行、第 nPosX + px 列咯
            int fi = (nPosY + py) * nFieldWidth + (nPosX + px);

            // 越界检查，小格子旋转后的位置应该在游戏区域内
            if (nPosX + px >= 0 && nPosX + px < nFieldWidth)
            {
                if (nPosY + py >= 0 && nPosY + py < nFieldHeight)
                {
                    // 如果旋转后，方块（被标记为 X 的格子）的位置不是空字符，
                    // 说明该区域已经被填充了，即该区域已经有方块占据了
                    // 则禁止旋转！
                    if (tetromino[nTetromino][pi] != L'.' && pField[fi] != 0)
                    {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}
#pragma endregion

int main()
{
#pragma region 创建 7 种方块，看代码就能看出形状，并没有简化成一行
    // 第一种方块：竖直方块
    // 第一个元素本质上是长度 16 的字符串，为了便于观察，所以按批次添加了
    tetromino[0].append(L"..X.");
    tetromino[0].append(L"..X.");
    tetromino[0].append(L"..X.");
    tetromino[0].append(L"..X.");

    // 第二种方块：田字方块
    tetromino[1].append(L"....");
    tetromino[1].append(L".XX.");
    tetromino[1].append(L".XX.");
    tetromino[1].append(L"....");

    // 第三种方块：L 形方块
    tetromino[2].append(L"..X.");
    tetromino[2].append(L"..X.");
    tetromino[2].append(L"..X.");
    tetromino[2].append(L"..XX");

    // 第四种方块：反 L 形方块
    tetromino[3].append(L".X..");
    tetromino[3].append(L".X..");
    tetromino[3].append(L".X..");
    tetromino[3].append(L"XX..");

    // 第五种方块
    tetromino[4].append(L"..X.");
    tetromino[4].append(L".XX.");
    tetromino[4].append(L".X..");
    tetromino[4].append(L"....");

    // 第六种方块
    tetromino[5].append(L".X..");
    tetromino[5].append(L".XX.");
    tetromino[5].append(L"..X.");
    tetromino[5].append(L"....");

    // 第七种方块：T 字形
    tetromino[6].append(L"..X.");
    tetromino[6].append(L".XX.");
    tetromino[6].append(L"..X.");
    tetromino[6].append(L"....");
#pragma endregion

#pragma region 初始化游戏区域
    // 开辟内存，保存游戏中方块的状态咯
    pField = new unsigned char[nFieldWidth * nFieldHeight];
    // 初始化游戏的边框
    for (int x = 0; x < nFieldWidth; x++)
    {
        for (int y = 0; y < nFieldHeight; y++)
        {
            // 游戏区域最外层是边框，所以需要初始化
            // 原本用三元表达式写最好，但我想写得清楚一点
            // 判断 x 和 y 是否是游戏区域最外层的点，如果是，则设置为 #
            if (x == 0 || x == nFieldWidth - 1 || y == nFieldHeight - 1)
            {
                pField[y * nFieldWidth + x] = 9;
            }
            else
            {
                pField[y * nFieldWidth + x] = 0;
            }
        }
    }

    // 使用命令行缓存，指定宽高
    wchar_t *screen = new wchar_t[nScreenWidth * nScreenHeight];
    // 初始化为空
    for (int i = 0; i < nScreenWidth * nScreenHeight; i++)
    {
        screen[i] = L' ';
    }
    // 如它的名字，Console Buffer
    HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(hConsole);
    DWORD dwBytesWritten = 0;

#pragma endregion

#pragma region 游戏逻辑所需要的数据
    int nCurrentPiece = 0;           // 记录当前下落的是何种方块，其实就是一个索引啦
    int nCurrentRotation = 0;        // 记录当前方块的旋转角度
    int nCurrentX = nFieldWidth / 2; // 记录当前方块在游戏区域中的位置 x 坐标。默认情况下方块都是从游戏区域中间开始掉落
    int nCurrentY = 0;               // 同上，记录 y 坐标
    bool bGameOver = false;
    bool bKey[4]; // 记录键盘状态
    // 锁死旋转状态。因为游戏 50ms 刷新一次，期间按下 z 键时可能触发多次旋转
    // 所以在按下旋转后需要锁住状态
    bool bRotateHold = false;

    int nSpeed = 20;         // 方块自动下落的速度，每 20 帧就下落一次。所以数字越小，下落越快
    int nSpeedCounter = 0;   // 记录帧数的变化，当到达 nSpeed 时，方块自动下落一次
    bool bForceDown = false; // 记录方块是否可以自动下落

    std::vector<int> vLines; // 记录消除的行号，用于在游戏结束时显示

    // 记录得分，其实就是消除了多少行咯
    int nPieceScore = 0;
    // 记录已经下落的方块数量，下落越多，则游戏难度增加
    // 难度体现为方块的自动下落速度增加咯
    int nPieceCount = 0;
#pragma endregion

    // 游戏循环
    while (!bGameOver)
    {
#pragma region 游戏帧率，50ms 刷新一次
        /*
            这行语句需要用到
            #include <chrono>
            #include <thread>
            using namespace std::chrono_literals;  // 用于书写 50ms 的字面量
         */
        std::this_thread::sleep_for(50ms); // 暂停 50ms 咯
        nSpeedCounter++;
        // 经过指定帧数后，方块自动下落一次
        bForceDown = (nSpeedCounter >= nSpeed);
#pragma endregion

#pragma region 游戏输入，用户的键盘交互
        for (int k = 0; k < 4; k++)
        {
            // 处理按键 R、L、D、Z，捕获按键啦就是
            // R 键：左移
            // L 键：右移
            // D 键：下移
            // Z 键：旋转
            // 如果按下了这些键则返回 true
            bKey[k] = (0x8000 & GetAsyncKeyState((unsigned char)("RLDZ"[k]))) != 0;
        }
#pragma endregion

#pragma region 游戏逻辑，处理用户的键盘输入
        // 我尽量将代码写得清楚，而没有做语法上的精简哟
        // 当按下 R、L、D、Z 键时进行处理咯
        if (bKey[1]) // L 键，左移
        {
            // 计算左移一次时是否发生了碰撞，检测的位置是 nCurrentX - 1, nCurrenttY
            // 返回 true 表示可以左移
            if (DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX - 1, nCurrentY))
            {
                --nCurrentX; // 左移
            }
        }
        if (bKey[0]) // R 键，右移
        {
            if (DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX + 1, nCurrentY))
            {
                ++nCurrentX;
            }
        }
        if (bKey[2]) // D 键，下移
        {
            if (DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY + 1))
            {
                ++nCurrentY;
            }
        }
        if (bKey[3]) // Z 键，旋转
        {
            // 如果没有按下 Z 键，则触发旋转
            if (!bRotateHold && DoesPieceFit(nCurrentPiece, nCurrentRotation + 1, nCurrentX, nCurrentY + 1))
            {
                ++nCurrentRotation;
            }
            bRotateHold = true; // 锁住旋转状态
        }
        else
        {
            bRotateHold = false; // 解锁旋转状态
        }

        // 如果方块自动下落，则进行下落
        if (bForceDown)
        {
            // 自动下落时，如果下落后仍然可以放置，则继续下落
            if (DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY + 1))
            {
                ++nCurrentY;
            }
            else
            {
                // 如果不能继续下落，说明到底了，则将当前方块固定到游戏区域中
                // 其实就是把当前方块的 “内容” 固定到游戏区域中
                for (int px = 0; px < 4; px++)
                {
                    for (int py = 0; py < 4; py++)
                    {
                        if (tetromino[nCurrentPiece][Rotate(px, py, nCurrentRotation)] != L'.')
                        {
                            // 计算出方块内容在游戏区域中的坐标
                            int fi = (nCurrentY + py) * nFieldWidth + nCurrentX + px;
                            // 注意了，因为在把 pFiled 绘制到屏幕（也就是写入到 screen 缓冲区）时，
                            // 用的是 L" ABCDEFG=#"[index] 的形式，所以这里需要 +1 才是正确的字符
                            pField[fi] = nCurrentPiece + 1;
                        }
                    }
                }

                nPieceCount++;
                if (nPieceCount % 10 == 0)
                    if (nSpeed >= 10)
                        nSpeed--; // 难度增加，方块下落速度增加，但最多不超过 10 帧下落一次

                // 现在方块已经固定了，需要判断 “消行”，即凑成一行就消除咯
                // 注意，不需要检查整个游戏区域，因为 “消除行” 的操作是在当前方块固定时才可能触发
                // 所以需要检查当前方块所在的行（注意它占 4 行）是否可以进行消除哟
                for (int py = 0; py < 4; py++)
                    // nCurrentY 表示当前所在的行，它最多占据 4 行，为了保险起见这里做一个判断避免越界
                    if (nCurrentY + py < nFieldHeight - 1)
                    {
                        bool bLine = true; // 记录当前行是否可以消除

                        // 下面就是检测出不消除时的条件
                        // 该游戏区域中 nCurrentY + py 行的所有区域进行检查，看看它们是否都被占据
                        // 如果都被占据，那么 bLine 为 true！
                        for (int px = 1; px < nFieldWidth - 1; px++)
                        {
                            bLine &= (pField[(nCurrentY + py) * nFieldWidth + px] != 0);
                        }

                        if (bLine)
                        {
                            // 消除行咯
                            for (int px = 1; px < nFieldWidth - 1; px++)
                            {
                                // 注意了，因为在把 pFiled 绘制到屏幕（也就是写入到 screen 缓冲区）时，
                                // 用的是 L" ABCDEFG=#"[index] 的形式，所以写入索引
                                pField[(nCurrentY + py) * nFieldWidth + px] = 8;
                            }
                            vLines.push_back(nCurrentY + py); // 记录消除的行号
                        }
                    }

                // 记录得分，默认情况下，只要方块落地，就得分增加 25
                nPieceScore += 25;
                // 如果消除了行，则得分呈指数增加。消除 1 行得 100 分。
                // 消除 4 行得 1600 分
                if (!vLines.empty())
                    // 计算 2 ^ 行数哟
                    nPieceScore += (1 << vLines.size()) * 100;

                // 当前方块放置结束，开始选择下一个方块咯
                nCurrentPiece = rand() % 7; // rand() 生成一个随机数，范围是 0 - RAND_MAX
                nCurrentRotation = 0;
                nCurrentX = nFieldWidth / 2;
                nCurrentY = 0;

                // 如果当前方块放不下（也就新的方块放不下了），说明游戏结束了
                bGameOver = !DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY);
            }
            nSpeedCounter = 0; // 重置帧数的计数咯
        }
#pragma endregion

#pragma region 游戏绘制
        // 绘制游戏区域，注意它绘制在 screen 缓冲区中，将来再把 screen 输出到屏幕上
        for (int y = 0; y < nFieldHeight; y++)
        {
            for (int x = 0; x < nFieldWidth; x++)
            {
                // ABCDEFG 表示 7 种方块咯
                const wchar_t c = L" ABCDEFG=#"[pField[y * nFieldWidth + x]];
                // 前两行空出来，避免靠近窗口标题栏
                // X+2 则是为了忽略边界，因为字符是 wchar_t，所以加的数字 2
                screen[(y + 2) * nScreenWidth + x + 2] = c;
            }
        }
        // 绘制当前掉落的方块（虽然还没有移动），注意它也是写入 screen 缓冲区中
        for (int px = 0; px < 4; px++)
            for (int py = 0; py < 4; py++)
            {
                // 将旋转后的小格子绘制到游戏区域中，注意，只绘制 X 部分，也就是实际方块的部分
                if (tetromino[nCurrentPiece][Rotate(px, py, nCurrentRotation)] != L'.')
                {
                    // 这里是计算位置啦，y * 宽度 + x
                    // nCurrentPiece 记录了当前方块的种类，取值为 0 - 7，原本是当作索引的
                    // 但是绘制方块到游戏区域时，为了区分不同种类的方块，所以将绘制的内容设置成 A、B、C 等等
                    // 而 65 就是 'A'，那么 nCurrentPiece + 65 恰好能利用 A、B 等表示各种不同的方块罗
                    screen[(nCurrentY + py + 2) * nScreenWidth + nCurrentX + px + 2] = nCurrentPiece + 65;
                }
            }

        // 如果有行需要消除，则需要处理
        if (!vLines.empty())
        {
            // 先将要消除的行（已经被取值为 ====）绘制到屏幕上
            WriteConsoleOutputCharacterW(hConsole, screen, nScreenWidth * nScreenHeight, {0, 0}, &dwBytesWritten);
            // 然后过 400ms 后，将消除的行（已经被标记为 ====）清除掉，从而实现一个小动画
            std::this_thread::sleep_for(400ms);
            for (auto &v : vLines) // v 是消除的行号，即游戏区域中的某个 y 坐标
            {
                for (int px = 1; px < nFieldWidth - 1; px++)
                {
                    // 需要把 v 行上面的内容向下移动。其实就是把 v 行往上移动啦
                    // 注意了，此时的 v 行还是 ==== 构成的行哟
                    for (int py = v; py > 0; py--)
                    {
                        pField[py * nFieldWidth + px] = pField[(py - 1) * nFieldWidth + px];
                    }
                    // 然后将其置空
                    pField[px] = 0;
                }
            }
            vLines.clear(); // 清空消除的行号
        }
        // 显示得分
        // 它从 screen 的第 2 行开始，宽度是比游戏区域多 6 空格的位置开始输出
        swprintf_s(&screen[nScreenWidth * 2 + nFieldWidth + 6], 16, L"Score: %8d", nPieceScore);
        // 开始绘制，注意使用 W 版本
        WriteConsoleOutputCharacterW(hConsole, screen, nScreenWidth * nScreenHeight, {0, 0}, &dwBytesWritten);
#pragma endregion
    }
    CloseHandle(hConsole);
    delete[] screen;
    delete[] pField;
    std::cout << "Game Over! Score: " << nPieceScore << std::endl;
    return 0;
}