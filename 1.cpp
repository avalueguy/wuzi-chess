#include <iostream>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cstring>
using namespace std;

#define INF 2000000000

// 动态适配 15x15 棋盘
const int GRIDSIZE = 15;
const int EMPTY = 0;
const int BLACK = 1;
const int WHITE = 2;

int board[GRIDSIZE][GRIDSIZE];

// ==========================================
// 【终极算力引擎：三进制滑动哈希表】
// ==========================================
int score_table[729];
int pow3[6] = {1, 3, 9, 27, 81, 243};

clock_t START_TIME;
double TIME_LIMIT = 0.90 * CLOCKS_PER_SEC; 
bool TIMEOUT_FLAG = false;

int my_color_global;
int op_color_global;

void init_score_table(int my_color) {
    int op_color = (my_color == BLACK) ? WHITE : BLACK;
    for (int i = 0; i < 729; i++) {
        int temp = i;
        int counts[3] = {0};
        for (int j = 0; j < 6; j++) {
            counts[temp % 3]++;
            temp /= 3;
        }
        int my_cnt = counts[my_color];
        int op_cnt = counts[op_color];
        int val = 0;

        if (op_cnt == 0) { 
            if (my_cnt == 1) val = 10;
            else if (my_cnt == 2) val = 100;
            else if (my_cnt == 3) val = 1000;
            else if (my_cnt == 4) val = 20000;
            else if (my_cnt == 5) val = 500000;
            else if (my_cnt == 6) val = 20000000; 
        } else if (my_cnt == 0) { 
            if (op_cnt == 1) val = -10;
            else if (op_cnt == 2) val = -120;
            else if (op_cnt == 3) val = -1200;
            else if (op_cnt == 4) val = -25000;    
            else if (op_cnt == 5) val = -600000;   
            else if (op_cnt == 6) val = -30000000; 
        }
        score_table[i] = val;
    }
}

int evaluate_board() {
    int total = 0;
    
    // 横向扫描
    for (int i = 0; i < GRIDSIZE; i++) {
        int hash = 0;
        for (int j = 0; j < 6; j++) hash += board[i][j] * pow3[j];
        total += score_table[hash];
        for (int j = 6; j < GRIDSIZE; j++) {
            hash = (hash / 3) + board[i][j] * 243;
            total += score_table[hash];
        }
    }
    // 纵向扫描
    for (int j = 0; j < GRIDSIZE; j++) {
        int hash = 0;
        for (int i = 0; i < 6; i++) hash += board[i][j] * pow3[i];
        total += score_table[hash];
        for (int i = 6; i < GRIDSIZE; i++) {
            hash = (hash / 3) + board[i][j] * 243;
            total += score_table[hash];
        }
    }
    // 主对角线扫描 \ .
    for (int d = -(GRIDSIZE - 6); d <= (GRIDSIZE - 6); d++) {
        int start_i = max(0, -d);
        int start_j = max(0, d);
        int len = min(GRIDSIZE - start_i, GRIDSIZE - start_j);
        if (len < 6) continue;
        int hash = 0;
        for (int k = 0; k < 6; k++) hash += board[start_i + k][start_j + k] * pow3[k];
        total += score_table[hash];
        for (int k = 6; k < len; k++) {
            hash = (hash / 3) + board[start_i + k][start_j + k] * 243;
            total += score_table[hash];
        }
    }
    // 副对角线扫描 / .
    for (int d = 5; d <= 2 * GRIDSIZE - 7; d++) {
        int start_i = max(0, d - (GRIDSIZE - 1));
        int start_j = min(GRIDSIZE - 1, d);
        int len = min(GRIDSIZE - start_i, start_j + 1);
        if (len < 6) continue;
        int hash = 0;
        for (int k = 0; k < 6; k++) hash += board[start_i + k][start_j - k] * pow3[k];
        total += score_table[hash];
        for (int k = 6; k < len; k++) {
            hash = (hash / 3) + board[start_i + k][start_j - k] * 243;
            total += score_table[hash];
        }
    }
    return total;
}

bool has_neighbor(int r, int c) {
    for (int i = max(0, r - 2); i <= min(GRIDSIZE - 1, r + 2); i++) {
        for (int j = max(0, c - 2); j <= min(GRIDSIZE - 1, c + 2); j++) {
            if (board[i][j] != EMPTY) return true;
        }
    }
    return false;
}

struct Move { int x1, y1, x2, y2, score; };

vector<Move> generate_moves(int max_spots) {
    vector<pair<int, int>> single_scores; 
    
    for(int i = 0; i < GRIDSIZE; i++) {
        for(int j = 0; j < GRIDSIZE; j++) {
            if (board[i][j] != EMPTY) continue;
            if (!has_neighbor(i, j)) continue; 
            
            board[i][j] = my_color_global;
            int s1 = evaluate_board();
            board[i][j] = op_color_global;
            int s2 = evaluate_board();
            board[i][j] = EMPTY;
            
            single_scores.push_back({s1 - s2, i * GRIDSIZE + j});
        }
    }
    
    sort(single_scores.rbegin(), single_scores.rend());
    int limit = min((int)single_scores.size(), max_spots);
    
    vector<Move> moves;
    for (int i = 0; i < limit; i++) {
        for (int j = i + 1; j < limit; j++) {
            int idx1 = single_scores[i].second;
            int idx2 = single_scores[j].second;
            moves.push_back({idx1/GRIDSIZE, idx1%GRIDSIZE, idx2/GRIDSIZE, idx2%GRIDSIZE, single_scores[i].first + single_scores[j].first});
        }
    }
    return moves;
}

int alphaBeta(int depth, int alpha, int beta, bool isMax) {
    if (clock() - START_TIME > TIME_LIMIT) { TIMEOUT_FLAG = true; return 0; }
    
    int eval = evaluate_board();
    if (depth == 0 || abs(eval) > 10000000) return eval;

    vector<Move> moves = generate_moves(8);
    if (moves.empty()) return eval;

    if (isMax) {
        int maxEval = -INF;
        for (auto m : moves) {
            board[m.x1][m.y1] = my_color_global; board[m.x2][m.y2] = my_color_global;
            int e = alphaBeta(depth - 1, alpha, beta, false);
            board[m.x1][m.y1] = EMPTY; board[m.x2][m.y2] = EMPTY;
            
            if (TIMEOUT_FLAG) return 0;
            maxEval = max(maxEval, e);
            alpha = max(alpha, e);
            if (beta <= alpha) break; 
        }
        return maxEval;
    } else {
        int minEval = INF;
        for (auto m : moves) {
            board[m.x1][m.y1] = op_color_global; board[m.x2][m.y2] = op_color_global;
            int e = alphaBeta(depth - 1, alpha, beta, true);
            board[m.x1][m.y1] = EMPTY; board[m.x2][m.y2] = EMPTY;
            
            if (TIMEOUT_FLAG) return 0;
            minEval = min(minEval, e);
            beta = min(beta, e);
            if (beta <= alpha) break; 
        }
        return minEval;
    }
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    char message[256];
    if (!(cin >> message)) return 0; 

    // ========= 【模式A】线上测评模式 =========
    if (isdigit(message[0]) || (message[0] == '-' && isdigit(message[1]))) {
        int turnID = atoi(message);
        my_color_global = WHITE;
        op_color_global = BLACK;
        int x0, y0, x1, y1;

        for (int i = 0; i < GRIDSIZE; ++i)
            for (int j = 0; j < GRIDSIZE; ++j)
                board[i][j] = EMPTY;

        for (int i = 0; i < turnID; i++) {
            cin >> x0 >> y0 >> x1 >> y1;
            if (i == 0 && x0 == -1 && y0 == -1) {
                my_color_global = BLACK;
                op_color_global = WHITE;
            }
            if (x0 >= 0 && x0 < GRIDSIZE && y0 >= 0 && y0 < GRIDSIZE) board[x0][y0] = op_color_global;
            if (x1 >= 0 && x1 < GRIDSIZE && y1 >= 0 && y1 < GRIDSIZE) board[x1][y1] = op_color_global;

            if (i < turnID - 1) {
                cin >> x0 >> y0 >> x1 >> y1;
                if (x0 >= 0 && x0 < GRIDSIZE && y0 >= 0 && y0 < GRIDSIZE) board[x0][y0] = my_color_global;
                if (x1 >= 0 && x1 < GRIDSIZE && y1 >= 0 && y1 < GRIDSIZE) board[x1][y1] = my_color_global;
            }
        }

        init_score_table(my_color_global);

        int startX = -1, startY = -1, resultX = -1, resultY = -1;
        bool selfFirstBlack = (turnID == 1 && my_color_global == BLACK);

        if (selfFirstBlack) {
            startX = GRIDSIZE / 2; startY = GRIDSIZE / 2; resultX = -1; resultY = -1;
        } else {
            START_TIME = clock();
            TIME_LIMIT = 0.90 * CLOCKS_PER_SEC; 
            TIMEOUT_FLAG = false;

            vector<Move> root_moves = generate_moves(12);
            Move best_move = root_moves.empty() ? Move{0, 0, 0, 1, 0} : root_moves[0];

            for (int depth = 1; depth <= 3; depth++) {
                int alpha = -INF;
                int beta = INF;
                Move current_depth_best = root_moves[0];
                
                for (auto m : root_moves) {
                    if (clock() - START_TIME > TIME_LIMIT) {
                        TIMEOUT_FLAG = true; break;
                    }
                    
                    board[m.x1][m.y1] = my_color_global; 
                    board[m.x2][m.y2] = my_color_global;
                    int e = alphaBeta(depth - 1, alpha, beta, false);
                    board[m.x1][m.y1] = EMPTY; 
                    board[m.x2][m.y2] = EMPTY;

                    if (TIMEOUT_FLAG) break;

                    if (e > alpha) {
                        alpha = e;
                        current_depth_best = m;
                    }
                    if (alpha > 10000000) break;
                }
                
                if (!TIMEOUT_FLAG) {
                    best_move = current_depth_best;
                    if (alpha > 10000000) break; 
                } else {
                    break;
                }
            }
            startX = best_move.x1; startY = best_move.y1;
            resultX = best_move.x2; resultY = best_move.y2;
        }

        bool isIllegal = false;
        if (selfFirstBlack) {
            if (startX < 0 || startX >= GRIDSIZE || startY < 0 || startY >= GRIDSIZE || board[startX][startY] != EMPTY) isIllegal = true;
        } else {
            if (startX < 0 || startX >= GRIDSIZE || startY < 0 || startY >= GRIDSIZE || 
                resultX < 0 || resultX >= GRIDSIZE || resultY < 0 || resultY >= GRIDSIZE || 
                board[startX][startY] != EMPTY || board[resultX][resultY] != EMPTY || 
                (startX == resultX && startY == resultY)) isIllegal = true;
        }

        if (isIllegal) {
            int emptyCount = 0;
            for (int i = 0; i < GRIDSIZE; i++) {
                for (int j = 0; j < GRIDSIZE; j++) {
                    if (board[i][j] == EMPTY) {
                        if (selfFirstBlack) { startX = i; startY = j; resultX = -1; resultY = -1; emptyCount = 2; break; }
                        else {
                            if (emptyCount == 0) { startX = i; startY = j; emptyCount++; }
                            else if (emptyCount == 1) { resultX = i; resultY = j; emptyCount++; break; }
                        }
                    }
                }
                if (emptyCount >= 2) break;
            }
        }

        cout << startX << " " << startY << " " << resultX << " " << resultY << endl;
        return 0;
    } 
    // ========= 【模式B】本地工具连续测试模式 =========
    else {
        int computerSide = WHITE;
        do {
            if (strcmp(message, "name?") == 0) {
                cout << "name 发际线在作队" << endl;
            } else if (strcmp(message, "new") == 0) {
                cin >> message;
                computerSide = (strcmp(message, "black") == 0) ? BLACK : WHITE;
                for (int i = 0; i < GRIDSIZE; ++i)
                    for (int j = 0; j < GRIDSIZE; ++j)
                        board[i][j] = EMPTY;
                
                if (computerSide == BLACK) {
                    board[GRIDSIZE/2][GRIDSIZE/2] = computerSide;
                    printf("move %c%c@@\n", (GRIDSIZE/2) + 'A', (GRIDSIZE/2) + 'A');
                    fflush(stdout);
                }
            } else if (strcmp(message, "move") == 0) {
                cin >> message;
                int step_first_x = message[0] - 'A';
                int step_first_y = message[1] - 'A';
                int step_second_x = message[2] - 'A';
                int step_second_y = message[3] - 'A';

                if (step_first_x >= 0 && step_first_x < GRIDSIZE && step_first_y >= 0 && step_first_y < GRIDSIZE)
                    board[step_first_x][step_first_y] = (computerSide == BLACK) ? WHITE : BLACK;
                if (message[2] != '@' && step_second_x >= 0 && step_second_x < GRIDSIZE && step_second_y >= 0 && step_second_y < GRIDSIZE) {
                    board[step_second_x][step_second_y] = (computerSide == BLACK) ? WHITE : BLACK;
                }

                my_color_global = computerSide;
                op_color_global = (computerSide == BLACK) ? WHITE : BLACK;
                init_score_table(my_color_global);

                START_TIME = clock();
                TIME_LIMIT = 0.90 * CLOCKS_PER_SEC;
                TIMEOUT_FLAG = false;

                vector<Move> root_moves = generate_moves(12);
                Move best_move = root_moves.empty() ? Move{0, 0, 0, 1, 0} : root_moves[0];

                for (int depth = 1; depth <= 3; depth++) {
                    int alpha = -INF;
                    int beta = INF;
                    Move current_depth_best = root_moves[0];
                    
                    for (auto m : root_moves) {
                        if (clock() - START_TIME > TIME_LIMIT) { TIMEOUT_FLAG = true; break; }
                        board[m.x1][m.y1] = my_color_global; board[m.x2][m.y2] = my_color_global;
                        int e = alphaBeta(depth - 1, alpha, beta, false);
                        board[m.x1][m.y1] = EMPTY; board[m.x2][m.y2] = EMPTY;

                        if (TIMEOUT_FLAG) break;
                        if (e > alpha) { alpha = e; current_depth_best = m; }
                        if (alpha > 10000000) break;
                    }
                    if (!TIMEOUT_FLAG) {
                        best_move = current_depth_best;
                        if (alpha > 10000000) break; 
                    } else break;
                }

                int startX = best_move.x1, startY = best_move.y1;
                int resultX = best_move.x2, resultY = best_move.y2;

                bool isIllegal = false;
                if (startX < 0 || startX >= GRIDSIZE || startY < 0 || startY >= GRIDSIZE || resultX < 0 || resultX >= GRIDSIZE || resultY < 0 || resultY >= GRIDSIZE || board[startX][startY] != EMPTY || board[resultX][resultY] != EMPTY || (startX == resultX && startY == resultY)) {
                    isIllegal = true;
                }

                if (isIllegal) {
                    int emptyCount = 0;
                    for (int i = 0; i < GRIDSIZE; i++) {
                        for (int j = 0; j < GRIDSIZE; j++) {
                            if (board[i][j] == EMPTY) {
                                if (emptyCount == 0) { startX = i; startY = j; emptyCount++; }
                                else if (emptyCount == 1) { resultX = i; resultY = j; emptyCount++; break; }
                            }
                        }
                        if (emptyCount >= 2) break;
                    }
                }

                board[startX][startY] = computerSide;
                board[resultX][resultY] = computerSide;

                printf("move %c%c%c%c\n", startX + 'A', startY + 'A', resultX + 'A', resultY + 'A');
                fflush(stdout);
            } else if (strcmp(message, "quit") == 0) {
                break;
            }
        } while (cin >> message);
    }
    return 0;
}