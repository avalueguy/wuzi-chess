#ifdef _BOTZONE_ONLINE
#include "jsoncpp/json.h"
#else
#include <json/json.h>
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <set>
#include <sstream>
#include <vector>

using ScoreType = long long;
constexpr int kGridSize = 15;
constexpr ScoreType kWinScore = 1e18;
constexpr int kMaxDepth = 12;
constexpr double kExploreFactor = 1.5;

constexpr int kSearchBreadth[kMaxDepth + 1] = {2, 3, 3, 4, 4, 5, 5, 6, 6, 8, 8, 12, 12};

constexpr ScoreType kWeightAtkStraight[6][3] = {
    {1, 1, 1}, {1, 1, 1}, {1, 1, 2}, {1, 3, 8}, {1, 100, 10000}, {1, 1000, 10050}};
constexpr ScoreType kWeightAtkBroken[6][3] = {
    {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 2, 5}, {1, 110, 120}, {900, 950, 1050}};

constexpr ScoreType kWeightDefStraight[6][3] = {
    {1, 1, 1}, {1, 2, 3}, {1, 5, 20}, {1, 100, 800}, {1, 20000, 50000}, {1, 50000, 100000}};
constexpr ScoreType kWeightDefBroken[6][3] = {
    {1, 1, 1}, {1, 1, 1}, {1, 2, 5}, {1, 20, 80}, {1, 15000, 20000}, {9000, 10000, 20000}};

enum class Stone { kSelf = 0, kEnemy = 1, kEmpty = 2, kBoundary = 3 };

struct GridPos {
    int r, c;
    GridPos(int row = -1, int col = -1) : r(row), c(col) {}
    
    GridPos operator+(const GridPos& o) const { return GridPos(r + o.r, c + o.c); }
    GridPos operator-(const GridPos& o) const { return GridPos(r - o.r, c - o.c); }
    GridPos operator*(int v) const { return GridPos(r * v, c * v); }
    friend GridPos operator*(int v, const GridPos& p) { return GridPos(v * p.r, v * p.c); }
    
    bool operator==(const GridPos& o) const { return r == o.r && c == o.c; }
    bool operator!=(const GridPos& o) const { return r != o.r || c != o.c; }
    bool operator<(const GridPos& o) const { return r == o.r ? c < o.c : r < o.r; }
};

const GridPos kDirs[4] = { GridPos(1, 1), GridPos(1, -1), GridPos(0, 1), GridPos(1, 0) };

struct LineState {
    int seq_len[2] = {0, 0};
    bool is_open[2] = {false, false};
    int gap_len[2] = {0, 0};
    bool gap_open[2] = {false, false};

    inline void merge_from(const LineState& prev, Stone piece, Stone target, int dir_idx) {
        if (piece == Stone::kEmpty) {
            seq_len[dir_idx] = 0;
            is_open[dir_idx] = true;
            gap_len[dir_idx] = prev.seq_len[dir_idx] + 1;
            gap_open[dir_idx] = prev.is_open[dir_idx];
        } else if (piece == target) {
            seq_len[dir_idx] = prev.seq_len[dir_idx] + 1;
            is_open[dir_idx] = prev.is_open[dir_idx];
            gap_len[dir_idx] = prev.gap_len[dir_idx] + 1;
            gap_open[dir_idx] = prev.gap_open[dir_idx];
        } else {
            seq_len[dir_idx] = 0;
            gap_len[dir_idx] = 0;
            is_open[dir_idx] = false;
            gap_open[dir_idx] = false;
        }
    }
};

inline ScoreType evaluate_state(const LineState& st, bool is_defending) {
    int total = st.seq_len[0] + st.seq_len[1] + 1;
    if (total >= 6) return kWinScore;
    
    int hop_l = std::min(5, st.gap_len[0] + st.seq_len[1]);
    int hop_r = std::min(5, st.seq_len[0] + st.gap_len[1]);
    
    int state_s = st.is_open[0] + st.is_open[1];
    int state_hl = st.gap_open[0] + st.is_open[1];
    int state_hr = st.is_open[0] + st.gap_open[1];
    
    if (is_defending) {
        return std::max({
            kWeightDefStraight[total][state_s],
            kWeightDefBroken[hop_l][state_hl],
            kWeightDefBroken[hop_r][state_hr]
        });
    } else {
        return std::max({
            kWeightAtkStraight[total][state_s],
            kWeightAtkBroken[hop_l][state_hl],
            kWeightAtkBroken[hop_r][state_hr]
        });
    }
}

inline bool is_time_exhausted(const std::chrono::time_point<std::chrono::steady_clock>& start_pt) {
    auto current = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(current - start_pt).count() > 960;
}

class GameBoard;

struct ActionRecord {
    GridPos pos;
    ScoreType priority;
    int center_dist;

    ActionRecord(GridPos p, ScoreType prio, int dist) : pos(p), priority(prio), center_dist(dist) {}
    ActionRecord(GameBoard* context, GridPos p);

    bool operator<(const ActionRecord& o) const {
        if (priority != o.priority) return priority > o.priority;
        if (center_dist != o.center_dist) return center_dist < o.center_dist;
        return pos < o.pos;
    }
};


class GameBoard {
private:
    std::vector<Stone> _grid;
    std::vector<LineState> _flat_caches;
    std::vector<ScoreType> _flat_scores;
    std::vector<int> _update_stamps;
    int _current_stamp;

    inline int cache_idx(int r, int c, int d, int side) const {
        return (((r * kGridSize + c) * 4) + d) * 2 + side;
    }
    inline int score_idx(int r, int c, int side) const {
        return (r * kGridSize + c) * 2 + side;
    }
    inline int grid_idx(int r, int c) const {
        return r * kGridSize + c;
    }

public:
    std::set<ActionRecord> available_moves;
    bool is_black;

    explicit GameBoard(bool play_as_black) : _current_stamp(0), is_black(play_as_black) {
        int total_cells = kGridSize * kGridSize;
        _grid.assign(total_cells, Stone::kEmpty);
        _flat_caches.assign(total_cells * 8, LineState());
        _flat_scores.assign(total_cells * 2, 1);
        _update_stamps.assign(total_cells, 0);

        for (int row = 0; row < kGridSize; ++row) {
            for (int col = 0; col < kGridSize; ++col) {
                GridPos p(row, col);
                for (int d = 0; d < 4; ++d) {
                    for (int s = 0; s < 2; ++s) {
                        LineState& st = _flat_caches[cache_idx(row, col, d, s)];
                        GridPos back = p - kDirs[d];
                        if (is_valid(back)) {
                            st.is_open[0] = true;
                            st.gap_len[0] = 1;
                            if (is_valid(p - kDirs[d] * 2)) st.gap_open[0] = true;
                        }
                        GridPos front = p + kDirs[d];
                        if (is_valid(front)) {
                            st.is_open[1] = true;
                            st.gap_len[1] = 1;
                            if (is_valid(p + kDirs[d] * 2)) st.gap_open[1] = true;
                        }
                    }
                }
                refresh_score(p, Stone::kSelf);
                refresh_score(p, Stone::kEnemy);
                available_moves.emplace(this, p);
            }
        }
    }

    inline bool is_valid(GridPos p) const {
        return p.r >= 0 && p.r < kGridSize && p.c >= 0 && p.c < kGridSize;
    }

    inline Stone get_piece(GridPos p) const {
        if (!is_valid(p)) return Stone::kBoundary;
        return _grid[grid_idx(p.r, p.c)];
    }

    inline ScoreType get_score(GridPos p, Stone role) const {
        int side = (role == Stone::kSelf) ? 0 : 1;
        return _flat_scores[score_idx(p.r, p.c, side)];
    }

    void refresh_score(GridPos p, Stone role) {
        ScoreType final_s = 1;
        int side = (role == Stone::kSelf) ? 0 : 1;
        
        // 判断是否需要开启防守权重加成（仅自己是黑方，且算对手分时）
        bool apply_def_boost = (is_black && role == Stone::kEnemy);
        
        for (int d = 3; d >= 0; --d) {
            ScoreType w = evaluate_state(_flat_caches[cache_idx(p.r, p.c, d, side)], apply_def_boost);
            if (w == kWinScore) {
                final_s = kWinScore;
                break;
            }
            final_s *= w;
        }
        _flat_scores[score_idx(p.r, p.c, side)] = final_s;
    }

    void apply_move(GridPos target, Stone role) {
        if (target.r == -1 || get_piece(target) == role) return;

        if (get_piece(target) == Stone::kEmpty) {
            available_moves.erase({this, target});
        }
        _grid[grid_idx(target.r, target.c)] = role;
        
        _current_stamp++;
        std::vector<ActionRecord> impacted;

        for (int d = 0; d < 4; ++d) {
            for (int s = 0; s <= 1; ++s) {
                Stone target_side = (s == 0) ? Stone::kSelf : Stone::kEnemy;
                const LineState origin_st = _flat_caches[cache_idx(target.r, target.c, d, s)];
                
                int flow_dir = 1;
                while (flow_dir >= 0) {
                    GridPos vector_d = (flow_dir == 1) ? kDirs[d] : GridPos(-kDirs[d].r, -kDirs[d].c);
                    int span = origin_st.gap_len[flow_dir] + origin_st.gap_open[flow_dir] + 1;
                    GridPos end_bound = target + vector_d * span;
                    
                    for (GridPos cursor = target + vector_d; cursor != end_bound; cursor = cursor + vector_d) {
                        if (get_piece(cursor) == Stone::kEmpty) {
                            int idx = grid_idx(cursor.r, cursor.c);
                            if (_update_stamps[idx] != _current_stamp) {
                                _update_stamps[idx] = _current_stamp;
                                impacted.emplace_back(this, cursor);
                            }
                        }
                        GridPos prev_cur = cursor - vector_d;
                        int inv_flow = flow_dir ^ 1;
                        
                        _flat_caches[cache_idx(cursor.r, cursor.c, d, s)].merge_from(
                            _flat_caches[cache_idx(prev_cur.r, prev_cur.c, d, s)],
                            get_piece(prev_cur), target_side, inv_flow
                        );
                    }
                    flow_dir--;
                }
            }
        }

        for (const auto& act : impacted) available_moves.erase(act);
        for (const auto& act : impacted) {
            refresh_score(act.pos, Stone::kSelf);
            refresh_score(act.pos, Stone::kEnemy);
            available_moves.emplace(this, act.pos);
        }
        if (role == Stone::kEmpty) {
            available_moves.emplace(this, target);
        }
    }
};

ActionRecord::ActionRecord(GameBoard* context, GridPos p) {
    pos = p;
    center_dist = (std::abs(p.r - kGridSize / 2) + 1) * (std::abs(p.c - kGridSize / 2) + 1);
    
    ScoreType my_val = context->get_score(p, Stone::kSelf);
    ScoreType en_val = context->get_score(p, Stone::kEnemy);
    

    if (context->is_black) {
        if (my_val >= kWinScore / 100) priority = my_val * 2;
        else if (en_val >= kWinScore / 100) priority = en_val * 2;
        else priority = std::max(my_val, en_val * 2);
    } else {
        priority = std::max(my_val, en_val);
    }
}

// --- 模块 2：树搜索调度器 (MCTS逻辑隔离) ---
struct TreeNode {
    int visits = 0;
    int wins = 0;
    Stone current_role;
    Stone terminal_state;
    ActionRecord act_a;
    ActionRecord act_b;
    TreeNode* parent_link;
    std::vector<TreeNode*> branch;

    TreeNode(Stone r, ActionRecord a1, ActionRecord a2, TreeNode* pa, Stone term = Stone::kEmpty)
        : current_role(r), terminal_state(term), act_a(a1), act_b(a2), parent_link(pa) {}

    inline void backpropagate(Stone victor) {
        visits += 2;
        Stone opp_role = (current_role == Stone::kSelf) ? Stone::kEnemy : Stone::kSelf;
        if (victor == opp_role) wins += 2;
        else if (victor == Stone::kEmpty) wins += 1;
    }

    inline double compute_uct() const {
        return static_cast<double>(wins) / visits + 
               kExploreFactor * std::sqrt(std::log(parent_link->visits) / visits);
    }
};

class MctsAgent {
private:
    GameBoard& _board;
    std::chrono::time_point<std::chrono::steady_clock> _start_time;

    Stone execute_simulation(TreeNode* nd, int depth_remain) {
        if (nd->visits == 0 && nd->terminal_state == Stone::kEmpty && depth_remain > 0) {
            ScoreType threshold = std::min(1000.0, std::sqrt(_board.available_moves.begin()->priority));
            std::vector<ActionRecord> elite_pool;
            
            for (auto item : _board.available_moves) {
                int limit = kSearchBreadth[depth_remain];
                if (elite_pool.size() >= (size_t)std::max(2, limit / 2)) {
                    if (item.priority < threshold || elite_pool.size() >= (size_t)limit) break;
                }
                elite_pool.push_back(item);
            }

            std::set<std::pair<GridPos, GridPos>> filter_set;
            for (size_t idx = 0; idx < elite_pool.size(); ++idx) {
                ActionRecord a1 = elite_pool[idx];
                Stone next_r = (nd->current_role == Stone::kSelf) ? Stone::kEnemy : Stone::kSelf;
                
                if (_board.get_score(a1.pos, nd->current_role) == kWinScore) {
                    nd->branch.push_back(new TreeNode(next_r, a1, elite_pool[idx == 0 ? 1 : idx - 1], nd, nd->current_role));
                    break;
                }
                
                int search_limit = (kSearchBreadth[depth_remain] - idx) / 2 + 1;
                int tested = 0;
                bool instant_kill = false;
                
                _board.apply_move(a1.pos, nd->current_role);
                
                for (const auto& a2 : _board.available_moves) {
                    GridPos p_min = (a1.pos < a2.pos) ? a1.pos : a2.pos;
                    GridPos p_max = (a1.pos < a2.pos) ? a2.pos : a1.pos;
                    
                    if (!filter_set.insert({p_min, p_max}).second) continue;
                    if (++tested > search_limit) break;
                    
                    if (_board.get_score(a2.pos, nd->current_role) == kWinScore) {
                        instant_kill = true;
                        nd->branch = { new TreeNode(next_r, a1, a2, nd, nd->current_role) };
                        break;
                    }
                    nd->branch.push_back(new TreeNode(next_r, a1, a2, nd));
                }
                _board.apply_move(a1.pos, Stone::kEmpty);
                if (instant_kill) break;
            }
        }

        if (nd->branch.empty()) {
            nd->backpropagate(nd->terminal_state);
            return nd->terminal_state;
        }

        double best_uct = -1.0;
        TreeNode* target_child = nullptr;
        for (auto* child : nd->branch) {
            if (child->visits == 0) {
                target_child = child;
                break;
            }
            double cur_u = child->compute_uct();
            if (cur_u > best_uct) {
                best_uct = cur_u;
                target_child = child;
            }
        }

        _board.apply_move(target_child->act_a.pos, nd->current_role);
        _board.apply_move(target_child->act_b.pos, nd->current_role);
        
        Stone sim_res = execute_simulation(target_child, depth_remain - 1);
        
        _board.apply_move(target_child->act_b.pos, Stone::kEmpty);
        _board.apply_move(target_child->act_a.pos, Stone::kEmpty);
        
        nd->backpropagate(sim_res);
        return sim_res;
    }

public:
    MctsAgent(GameBoard& b) : _board(b) {
        _start_time = std::chrono::steady_clock::now();
    }

    Json::Value run_decision() {
        TreeNode* root = new TreeNode(Stone::kSelf, ActionRecord(GridPos(-1, -1), 0, 0), ActionRecord(GridPos(-1, -1), 0, 0), nullptr);

        while (!is_time_exhausted(_start_time)) {
            execute_simulation(root, kMaxDepth);
        }

        TreeNode* top_node = root->branch.front();
        for (auto* child : root->branch) {
            if (child->visits > top_node->visits) {
                top_node = child;
            }
        }

        std::ostringstream dbg;
        for (auto* child : root->branch) {
            dbg << child->act_a.pos.r << ',' << child->act_a.pos.c << ' ' << child->act_a.priority << ' ';
            dbg << child->act_b.pos.r << ',' << child->act_b.pos.c << ' ' << child->act_b.priority << ' ';
            dbg << child->wins << '/' << child->visits - child->wins << "    ";
        }

        Json::Value ans;
        ans["response"]["x0"] = top_node->act_a.pos.r;
        ans["response"]["y0"] = top_node->act_a.pos.c;
        ans["response"]["x1"] = top_node->act_b.pos.r;
        ans["response"]["y1"] = top_node->act_b.pos.c;
        ans["debug"] = dbg.str();

        return ans;
    }
};

// --- 模块 3：IO 解析与驱动 ---
int main() {
    std::string raw_input;
    std::getline(std::cin, raw_input);
    
    Json::Value packet;
    Json::Reader().parse(raw_input, packet);
    
    int turn_count = packet["requests"].size();
    bool playing_black = (packet["requests"][0u]["x0"].asInt() == -1);
    
    GameBoard env(playing_black);
    
    int step = 0;
    while (step < turn_count) {
        int e_r1 = packet["requests"][step]["x0"].asInt();
        int e_c1 = packet["requests"][step]["y0"].asInt();
        int e_r2 = packet["requests"][step]["x1"].asInt();
        int e_c2 = packet["requests"][step]["y1"].asInt();

        env.apply_move(GridPos(e_r1, e_c1), Stone::kEnemy);
        env.apply_move(GridPos(e_r2, e_c2), Stone::kEnemy);
        
        if (step == turn_count - 1) break;
        
        int m_r1 = packet["responses"][step]["x0"].asInt();
        int m_c1 = packet["responses"][step]["y0"].asInt();
        int m_r2 = packet["responses"][step]["x1"].asInt();
        int m_c2 = packet["responses"][step]["y1"].asInt();

        env.apply_move(GridPos(m_r1, m_c1), Stone::kSelf);
        env.apply_move(GridPos(m_r2, m_c2), Stone::kSelf);
        
        step++;
    }

    Json::Value final_out;
    

    if (turn_count == 1 && playing_black) {
        final_out["response"]["x0"] = kGridSize / 2;
        final_out["response"]["y0"] = kGridSize / 2;
        final_out["response"]["x1"] = -1;
        final_out["response"]["y1"] = -1;
    } else {
        MctsAgent searcher(env);
        final_out = searcher.run_decision();
    }

    std::cout << Json::FastWriter().write(final_out) << std::endl;
    return 0;
}
