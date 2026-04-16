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
    clock_t startTime = clock();

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
                // 遍历所有可能的连续6子窗口
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
                // 防守权重略高，避免被偷鸡
                return myScore - opScore * 2;
            }

            // 评估在(x,y)落子的潜力（用于预排序筛选候选点）
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
            int alphaBeta(int depth, int alpha, int beta, bool isMax, clock_t startT) {
                // 卡时机制：运行时间接近0.9秒立即返回极大/极小值强制截断 [cite: 153, 155]
                if (clock() - startT > 0.90 * CLOCKS_PER_SEC) {
                    return isMax ? -100000000 : 100000000;
                }

                int score = evaluateBoard();
                // 深度达到或者已经出现胜负局面则不再搜索
                if (depth == 0 || std::abs(score) > 5000000) return score;

                std::vector<PointInfo> pts;
                for (int x = 0; x < GRIDSIZE; x++) {
                    for (int y = 0; y < GRIDSIZE; y++) {
                        if (grid[x][y] == 0) {
                            bool near = false;
                            // 只考虑周围距离为2以内的点，修剪无效分支
                            for (int dx = -2; dx <= 2; dx++) {
                                for (int dy = -2; dy <= 2; dy++) {
                                    if (x + dx >= 0 && x + dx < GRIDSIZE && y + dy >= 0 && y + dy < GRIDSIZE && grid[x + dx][y + dy] != 0) {
                                        near = true; break;
                                    }
                                }
                                if (near) break;
                            }
                            if (near) {
                                // 综合考虑该点的进攻与防守价值
                                pts.push_back({ x, y, evalPointAdd(x,y,myC) + evalPointAdd(x,y,opC) });
                            }
                        }
                    }
                }
                std::sort(pts.begin(), pts.end());
                int limit = 10; // 控制每一层的分支数量以保证性能
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
                        int eval = alphaBeta(depth - 1, alpha, beta, false, startT);
                        grid[m.x1][m.y1] = 0; grid[m.x2][m.y2] = 0;
                        maxEval = std::max(maxEval, eval);
                        alpha = std::max(alpha, eval);
                        if (beta <= alpha) break; // 剪枝
                    }
                    return maxEval;
                }
                else {
                    int minEval = 1000000000;
                    for (Move m : moves) {
                        grid[m.x1][m.y1] = opC; grid[m.x2][m.y2] = opC;
                        int eval = alphaBeta(depth - 1, alpha, beta, true, startT);
                        grid[m.x1][m.y1] = 0; grid[m.x2][m.y2] = 0;
                        minEval = std::min(minEval, eval);
                        beta = std::min(beta, eval);
                        if (beta <= alpha) break; // 剪枝
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

        // 1. 获取最顶层的候选点
        std::vector<Searcher::PointInfo> pts;
        for (int x = 0; x < GRIDSIZE; x++) {
            for (int y = 0; y < GRIDSIZE; y++) {
                if (searcher.grid[x][y] == 0) {
                    bool near = false;
                    for (int dX = -2; dX <= 2; dX++) {
                        for (int dY = -2; dY <= 2; dY++) {
                            if (x + dX >= 0 && x + dX < GRIDSIZE && y + dY >= 0 && y + dY < GRIDSIZE && searcher.grid[x + dX][y + dY] != 0) {
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
        int limit = 12; // 根节点的分支可以稍微宽广一点
        if (pts.size() > limit) pts.resize(limit);

        // 2. 生成成对的落子组合并进行搜索
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

                // 搜索深度设为1代表预测对方进行1回合（2颗子）的回应
                int eval = searcher.alphaBeta(1, -1000000000, 1000000000, false, startTime);

                searcher.grid[m.x1][m.y1] = 0;
                searcher.grid[m.x2][m.y2] = 0;

                if (eval > bestVal) {
                    bestVal = eval;
                    bestMove = m;
                }

                // 强制卡时：即将超过0.9秒直接跳出循环 [cite: 153, 154, 155]
                if (clock() - startTime > 0.90 * CLOCKS_PER_SEC) break;
            }
            startX = bestMove.x1; startY = bestMove.y1;
            resultX = bestMove.x2; resultY = bestMove.y2;

        }
        else if (pts.size() == 1) {
            // 极个别情况（如最后无处落子）的兜底
            startX = pts[0].x; startY = pts[0].y;
            resultX = pts[0].x; resultY = pts[0].y;
        }
    }

    /****在上方填充你的代码，决策结果（本方将落子的位置）存入startX、startY、resultX、resultY中****/
    /************************************************************************************/

    // 决策结束，向平台输出决策结果

    cout << startX << ' ' << startY << ' ' << resultX << ' ' << resultY << endl;
    return 0;
}