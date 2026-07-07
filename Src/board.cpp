#include "board.h"
#include "ui.h"

#include <random>
#include <iostream>
#include <vector>
#include <QDebug>
#include <QFile>
using namespace std;

// ==================== 构造函数 ====================
board::board()
{
    init();
}

// ==================== 初始化 ====================
void board::init()
{
    for(int i = 0; i < ROW; i++)
    {
        for(int j = 0; j < COL; j++)
        {
            grid[i][j] = 0;
        }
    }
    score = 0;
    step = 0;
    gameStart = false;
    gameOver = false;
}

// ==================== 重新开始游戏 ====================
void board::initAll()
{
    init();             

    createNum();        
    createNum();      
    
    gameStart = true;    
    gameOver = false;  
}

// ==================== 生成数字 ====================
bool board::createNum()
{
    vector<pair<int, int>> index;
    for(int i = 0; i < ROW; i++)
    {
        for(int j = 0; j < COL; j++)
        {
            if(grid[i][j] == 0)
            {
                index.push_back(make_pair(i, j));
            }
        }
    }
    
    if(index.empty()) 
        return false;  
    
    int ind = rand() % index.size();
    int row = index[ind].first;
    int col = index[ind].second;
    grid[row][col] = (rand() % 10 < 9) ? 2 : 4;
    
    return true;
}

// ==================== 处理用户操作 ====================
void board::process(int cmd)
{
    if(gameOver) return;  // 游戏已结束，不再处理
    
    // 保存旧棋盘用于判断是否发生了变化
    int oldGrid[110][110];
    for(int i = 0; i < ROW; i++)
        for(int j = 0; j < COL; j++)
            oldGrid[i][j] = grid[i][j];
    
    // 执行移动
    switch(cmd)
    {
    case CMD_UP:
        moveUp();
        break;
    case CMD_DOWN:
        moveDown();
        break;
    case CMD_LEFT:
        moveLeft();
        break;
    case CMD_RIGHT:
        moveRight();
        break;
    default:
        return;
    }
    
    // 判断棋盘是否发生了变化
    bool changed = false;
    for(int i = 0; i < ROW; i++)
        for(int j = 0; j < COL; j++)
            if(oldGrid[i][j] != grid[i][j])
                changed = true;
    
    if(!changed) return;  // 没有变化，不生成新数字
    
    // 生成新数字
    createNum();
    step++;
    
    // 判断游戏状态
    int result = judge();
    if(result == STAT_WIN)
    {
        gameOver = true;
        cout << "WIN" << endl;
    }
    else if(result == STAT_LOSE)
    {
        gameOver = true;
        cout << "LOSE" << endl;
    }
}

// ==================== 判定游戏输赢 ====================
int board::judge()
{
    // 1. 检查是否胜利（出现 2048）
    for(int i = 0; i < ROW; i++)
    {
        for(int j = 0; j < COL; j++)
        {
            if(grid[i][j] >= 2048)  // 出现 2048 或更大
                return STAT_WIN;
        }
    }
    
    // 2. 检查是否还有空格
    for(int i = 0; i < ROW; i++)
    {
        for(int j = 0; j < COL; j++)
        {
            if(grid[i][j] == 0)
                return STAT_PROCESS;  // 还有空格，游戏继续
        }
    }
    
    // 3. 检查是否有相邻的相同数字（横向或纵向）
    for(int i = 0; i < ROW; i++)
    {
        for(int j = 0; j < COL; j++)
        {
            // 检查右边相邻
            if(j + 1 < COL && grid[i][j] == grid[i][j + 1])
                return STAT_PROCESS;
            // 检查下边相邻
            if(i + 1 < ROW && grid[i][j] == grid[i + 1][j])
                return STAT_PROCESS;
        }
    }
    
    // 没有空格，没有相邻相同数字 → 游戏失败
    return STAT_LOSE;
}

// ==================== moveUp（上移） ====================
void board::moveUp()
{
    for (int j = 0; j < COL; j++)
    {
        vector<int> t;
        for (int i = 0; i < ROW; i++)
        {
            if (grid[i][j] != 0)
                t.push_back(grid[i][j]);
        }

        vector<int> merged;
        for (int k = 0; k < (int)t.size(); k++)
        {
            if (k + 1 < (int)t.size() && t[k] == t[k + 1])
            {
                merged.push_back(t[k] * 2);
                score += t[k];  
                k++;
            }
            else
            {
                merged.push_back(t[k]);
            }
        }

        int cnt = 0;
        for (int val : merged)
        {
            grid[cnt++][j] = val;
        }
        while (cnt < ROW)
        {
            grid[cnt++][j] = 0;
        }
    }
}

// ==================== moveDown（下移） ====================
void board::moveDown()
{
    for (int j = 0; j < COL; j++)
    {
        vector<int> t;
        for (int i = ROW - 1; i >= 0; i--)
        {
            if (grid[i][j] != 0)
                t.push_back(grid[i][j]);
        }

        vector<int> merged;
        for (int k = 0; k < (int)t.size(); k++)
        {
            if (k + 1 < (int)t.size() && t[k] == t[k + 1])
            {
                merged.push_back(t[k] * 2);
                score += t[k];  
                k++;
            }
            else
            {
                merged.push_back(t[k]);
            }
        }

        int cnt = ROW - 1;
        for (int val : merged)
        {
            grid[cnt--][j] = val;
        }
        while (cnt >= 0)
        {
            grid[cnt--][j] = 0;
        }
    }
}

// ==================== moveLeft（左移） ====================
void board::moveLeft()
{
    for (int i = 0; i < ROW; i++)
    {
        vector<int> t;
        for (int j = 0; j < COL; j++)
        {
            if (grid[i][j] != 0)
                t.push_back(grid[i][j]);
        }

        vector<int> merged;
        for (int k = 0; k < (int)t.size(); k++)
        {
            if (k + 1 < (int)t.size() && t[k] == t[k + 1])
            {
                merged.push_back(t[k] * 2);
                score += t[k];  
                k++;
            }
            else
            {
                merged.push_back(t[k]);
            }
        }

        int cnt = 0;
        for (int val : merged)
        {
            grid[i][cnt++] = val;
        }
        while (cnt < COL)
        {
            grid[i][cnt++] = 0;
        }
    }
}

// ==================== moveRight（右移） ====================
void board::moveRight()
{
    for (int i = 0; i < ROW; i++)
    {
        vector<int> t;
        for (int j = COL - 1; j >= 0; j--)
        {
            if (grid[i][j] != 0)
                t.push_back(grid[i][j]);
        }

        vector<int> merged;
        for (int k = 0; k < (int)t.size(); k++)
        {
            if (k + 1 < (int)t.size() && t[k] == t[k + 1])
            {
                merged.push_back(t[k] * 2);
                score += t[k];  
                k++;
            }
            else
            {
                merged.push_back(t[k]);
            }
        }

        int cnt = COL - 1;
        for (int val : merged)
        {
            grid[i][cnt--] = val;
        }
        while (cnt >= 0)
        {
            grid[i][cnt--] = 0;
        }
    }
}

// ==================== Getter 函数 ====================
bool board::getGameStart()
{
    return gameStart;
}

void board::setGameStart(bool start)
{
    gameStart = start;
}

int board::getData(int row, int col)
{
    if(row < 0 || row >= ROW || col < 0 || col >= COL)
        return -1;
    return grid[row][col];
}

int board::getGrade()
{
    return score;
}

int board::getStep()
{
    return step;
}