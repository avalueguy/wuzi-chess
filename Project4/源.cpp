#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>
#include <cmath>

#define GRIDSIZE 19
#define judge_black 0
#define judge_white 1
#define grid_blank 0
#define grid_black 1
#define grid_white -1

using namespace std;

int currBotColor; // 本方所执子颜色（1为黑，-1为白，棋盘状态亦同）
int gridInfo[GRIDSIZE][GRIDSIZE] = { 0 }; // 先x后y，记录棋盘状态
int dx[] = { -1,-1,-1,0,0,1,1,1 };
int dy[] = { -1,0,1,-1,1,-1,0,1 };

// 判断是否在棋盘内
inline bool inMap(int x, int y)
{
    if (x < 0 || x >= GRIDSIZE || y < 0 || y >= GRIDSIZE)
        return false;
    return true;
}

// 在坐标处落子，检查是否合法或模拟落子
bool ProcStep(int x0, int y0, int x1, int y1, int grid_color, bool check_only)
{
    if (x1 == -1 || y1 == -1) {
        if (!inMap(x0, y0) || gridInfo[x0][y0] != grid_blank)
            return false;
        if (!check_only) {
            gridInfo[x0][y0] = grid_color;
        }
        return true;
    }
    else {
        if ((!inMap(x0, y0)) || (!inMap(x1, y1)))
            return false;
        if (gridInfo[x0][y0] != grid_blank || gridInfo[x1][y1] != grid_blank)
            return false;
        if (!check_only) {
            gridInfo[x0][y0] = grid_color;
            gridInfo[x1][y1] = grid_color;
        }
        return true;
    }
}

int main()
{
    int x0, y0, x1, y1;

    // 分析自己收到的输入和自己过往的输出，并恢复棋盘状态
    int turnID;
    cin >> turnID;
    currBotColor = grid_white; // 先假设自己是白方
    for (int i = 0; i < turnID; i++)
    {
        // 根据这些输入输出逐渐恢复状态到当前回合
        cin >> x0 >> y0 >> x1 >> y1;
        if (x0 == -1)
            currBotColor = grid_black; // 第一回合收到坐标是-1, -1，说明我是黑方
        if (x0 >= 0)
            ProcStep(x0, y0, x1, y1, -currBotColor, false); // 模拟对方落子
        if (i < turnID - 1) {
            cin >> x0 >> y0 >> x1 >> y1;
            if (x0 >= 0)
                ProcStep(x0, y0, x1, y1, currBotColor, false); // 模拟己方落子
        }
    }

    /************************************************************************************/
    /***在下面填充你的代码，决策结果（本方将落子的位置）存入startX、startY、resultX、resultY中*****/

    int startX = -1, startY = -1, resultX = -1, resultY = -1;
    bool selfFirstBlack = (turnID == 1 && currBotColor == grid_black);

    // 严格遵照官方文档的防超时设计，设定 0.95 秒的阈值
    clock_t startTime = clock();
    double timeThreshold = 0.95 * (double)CLOCKS_PER_SEC;

    if (selfFirstBlack) {
        // 黑方先手，只下一子，直接下在棋盘正中央
        startX = 9; startY = 9; resultX = -1; resultY = -1;
    }
    else {
        // 定义一个局部的搜索器结构体，封装启发式评估和Alpha-Beta剪枝
        struct Searcher {
            int dirX[4] = { 1, 0, 1, 1 };
            int dirY[4] = { 0, 1, 1, -1 };
            int grid[GRIDSIZE][GRIDSIZE];
            int myC, opC;

            // 线型评分函数
            int calcLine(int c, int opp) {
                if (opp > 0) return 0; // 死线
                switch (c) {
                case 0: return 0;
                case 1: return 2;
                case 2: return 15;
                case 3: return 400;
                case 4: return 8000;
                case 5: return 200000;
                case 6: return 10000000;
                }
                return 0;
            }

            // 整个棋盘的静态评估
            int evaluateBoard() {
                int myScore = 0, opScore = 0;
                for (int x = 0; x < GRIDSIZE; x++) {
                    for (int y = 0; y < GRIDSIZE; y++) {
                        for (int d = 0; d < 4; d++) {
                            int nx = x + 5 * dirX[d];
                            int ny = y + 5 * dirY[d];
                            if (nx >= 0 && nx < GRIDSIZE && ny >= 0 && ny < GRIDSIZE) {
                                int c = 0, op = 0;
                                for (int k = 0; k < 6; k++) {
                                    int p = grid[x + k * dirX[d]][y + k * dirY[d]];
                                    if (p == myC) c++;
                                    else if (p == opC) op++;
                                }
                                myScore += calcLine(c, op);
                                opScore += calcLine(op, c);
                            }
                        }
                    }
                }
                return myScore - opScore * 2;
            }

            // 评估在(x,y)落子的潜力
            int evalPointAdd(int x, int y, int color) {
                int score = 0;
                int oppColor = -color;
                for (int d = 0; d < 4; d++) {
                    for (int offset = 0; offset < 6; offset++) {
                        int sx = x - offset * dirX[d];
                        int sy = y - offset * dirY[d];
                        int ex = sx + 5 * dirX[d];
                        int ey = sy + 5 * dirY[d];
                        if (sx >= 0 && sx < GRIDSIZE && sy >= 0 && sy < GRIDSIZE &&
                            ex >= 0 && ex < GRIDSIZE && ey >= 0 && ey < GRIDSIZE) {
                            int c = 0, op = 0;
                            for (int k = 0; k < 6; k++) {
                                int p = grid[sx + k * dirX[d]][sy + k * dirY[d]];
                                if (p == color) c++;
                                else if (p == oppColor) op++;
                            }
                            if (op == 0) score += calcLine(c + 1, 0) - calcLine(c, 0);
                        }
                    }
                }
                return score;
            }

            struct PointInfo {
                int x, y, score;
                bool operator<(const PointInfo& o) const { return score > o.score; }
            };

            struct Move { int x1, y1, x2, y2; };

            // Alpha-Beta 剪枝递归
            int alphaBeta(int depth, int alpha, int beta, bool isMax, clock_t startT, double limitT) {
                if ((double)(clock() - startT) > limitT) {
                    return isMax ? -100000000 : 100000000;
                }

                int score = evaluateBoard();
                if (depth == 0 || std::abs(score) > 5000000) return score;

                std::vector<PointInfo> pts;
                for (int x = 0; x < GRIDSIZE; x++) {
                    for (int y = 0; y < GRIDSIZE; y++) {
                        if (grid[x][y] == grid_blank) {
                            bool near = false;
                            for (int dx = -2; dx <= 2; dx++) {
                                for (int dy = -2; dy <= 2; dy++) {
                                    if (x + dx >= 0 && x + dx < GRIDSIZE && y + dy >= 0 && y + dy < GRIDSIZE && grid[x + dx][y + dy] != grid_blank) {
                                        near = true; break;
                                    }
                                }
                                if (near) break;
                            }
                            if (near) {
                                pts.push_back({ x, y, evalPointAdd(x,y,myC) + evalPointAdd(x,y,opC) });
                            }
                        }
                    }
                }
                std::sort(pts.begin(), pts.end());
                size_t limit = 10; // 已修复警告
                if (pts.size() > limit) pts.resize(limit);

                std::vector<Move> moves;
                for (size_t i = 0; i < pts.size(); i++) {
                    for (size_t j = i + 1; j < pts.size(); j++) {
                        moves.push_back({ pts[i].x, pts[i].y, pts[j].x, pts[j].y });
                    }
                }

                if (isMax) {
                    int maxEval = -1000000000;
                    for (Move m : moves) {
                        grid[m.x1][m.y1] = myC; grid[m.x2][m.y2] = myC;
                        int eval = alphaBeta(depth - 1, alpha, beta, false, startT, limitT);
                        grid[m.x1][m.y1] = 0; grid[m.x2][m.y2] = 0;
                        maxEval = std::max(maxEval, eval);
                        alpha = std::max(alpha, eval);
                        if (beta <= alpha) break;
                    }
                    return maxEval;
                }
                else {
                    int minEval = 1000000000;
                    for (Move m : moves) {
                        grid[m.x1][m.y1] = opC; grid[m.x2][m.y2] = opC;
                        int eval = alphaBeta(depth - 1, alpha, beta, true, startT, limitT);
                        grid[m.x1][m.y1] = 0; grid[m.x2][m.y2] = 0;
                        minEval = std::min(minEval, eval);
                        beta = std::min(beta, eval);
                        if (beta <= alpha) break;
                    }
                    return minEval;
                }
            }
        };

        Searcher searcher;
        searcher.myC = currBotColor;
        searcher.opC = -currBotColor;
        for (int i = 0; i < GRIDSIZE; i++)
            for (int j = 0; j < GRIDSIZE; j++)
                searcher.grid[i][j] = gridInfo[i][j];

        std::vector<Searcher::PointInfo> pts;
        for (int x = 0; x < GRIDSIZE; x++) {
            for (int y = 0; y < GRIDSIZE; y++) {
                if (searcher.grid[x][y] == grid_blank) {
                    bool near = false;
                    for (int dX = -2; dX <= 2; dX++) {
                        for (int dY = -2; dY <= 2; dY++) {
                            if (x + dX >= 0 && x + dX < GRIDSIZE && y + dY >= 0 && y + dY < GRIDSIZE && searcher.grid[x + dX][y + dY] != grid_blank) {
                                near = true; break;
                            }
                        }
                        if (near) break;
                    }
                    if (near) {
                        pts.push_back({ x, y, searcher.evalPointAdd(x,y,searcher.myC) + searcher.evalPointAdd(x,y,searcher.opC) });
                    }
                }
            }
        }
        std::sort(pts.begin(), pts.end());
        size_t limit = 12; // 已修复警告
        if (pts.size() > limit) pts.resize(limit);

        if (pts.size() >= 2) {
            std::vector<Searcher::Move> rootMoves;
            for (size_t i = 0; i < pts.size(); i++) {
                for (size_t j = i + 1; j < pts.size(); j++) {
                    rootMoves.push_back({ pts[i].x, pts[i].y, pts[j].x, pts[j].y });
                }
            }

            int bestVal = -1000000000;
            Searcher::Move bestMove = rootMoves[0];

            for (Searcher::Move m : rootMoves) {
                searcher.grid[m.x1][m.y1] = searcher.myC;
                searcher.grid[m.x2][m.y2] = searcher.myC;

                int eval = searcher.alphaBeta(1, -1000000000, 1000000000, false, startTime, timeThreshold);

                searcher.grid[m.x1][m.y1] = 0;
                searcher.grid[m.x2][m.y2] = 0;

                if (eval > bestVal) {
                    bestVal = eval;
                    bestMove = m;
                }

                if ((double)(clock() - startTime) > timeThreshold) break;
            }
            startX = bestMove.x1; startY = bestMove.y1;
            resultX = bestMove.x2; resultY = bestMove.y2;
        }
    }

    // =========================================================================
    // 【终极拦截器】如果经过上面搜索后，依然存在重合、越界或下在已有子上的情况，强制纠正
    // =========================================================================
    bool isIllegal = false;
    if (selfFirstBlack) {
        if (!inMap(startX, startY) || gridInfo[startX][startY] != grid_blank) {
            isIllegal = true;
        }
    }
    else {
        if (!inMap(startX, startY) || !inMap(resultX, resultY) ||
            gridInfo[startX][startY] != grid_blank ||
            gridInfo[resultX][resultY] != grid_blank ||
            (startX == resultX && startY == resultY)) {
            isIllegal = true;
        }
    }

    if (isIllegal) {
        int emptyCount = 0;
        for (int i = 0; i < GRIDSIZE; i++) {
            for (int j = 0; j < GRIDSIZE; j++) {
                if (gridInfo[i][j] == grid_blank) {
                    if (selfFirstBlack) {
                        startX = i; startY = j; resultX = -1; resultY = -1;
                        emptyCount = 2; // 直接满足跳出条件
                        break;
                    }
                    else {
                        if (emptyCount == 0) {
                            startX = i; startY = j; emptyCount++;
                        }
                        else if (emptyCount == 1) {
                            resultX = i; resultY = j; emptyCount++;
                            break;
                        }
                    }
                }
            }
            if (emptyCount >= 2) break;
        }
    }
    // =========================================================================

    /****在上方填充你的代码，决策结果（本方将落子的位置）存入startX、startY、resultX、resultY中****/
    /************************************************************************************/

    // 决策结束，向平台输出决策结果
    cout << startX << ' ' << startY << ' ' << resultX << ' ' << resultY << endl;
    return 0;
}