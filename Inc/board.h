#ifndef GAMELOGIC_H
#define GAMELOGIC_H

#include <iostream>
#include <QSoundEffect>

//控制上下左右移动的标志枚举类型
enum CMD
{
    CMD_UP,
    CMD_DOWN,
    CMD_LEFT,
    CMD_RIGHT,
};

//状态提示枚举类型
enum STAT
{
    STAT_WAIT,
    STAT_PROCESS,
    STAT_WIN,
    STAT_LOSE,
};


class board
{
public:
    board();
    void init();
    bool createNum();
    void process(int cmd);
    int judge();                    // 判定游戏输赢
    void initAll();                 // 重新开始游戏

    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();

    bool getGameStart();
    void setGameStart(bool);
    int getData(int row, int col);
    int getGrade();                 // 获取总分
    int getStep();                  // 获取步数                      

private:
    bool gameStart = false;
    bool gameOver = false;
    int ROW = 4;
    int COL = 4;
    int grid[110][110];
    int score = 0;
    int step = 0;                    
};

#endif // GAMELOGIC_H