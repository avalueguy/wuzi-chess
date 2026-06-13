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

using namespace std;
using int64 = long long;


const int BOARD_ROWS = 15;
const int BOARD_COLS = 15;
const int64 SCORE_INFINITY = 1e18;
const int MAX_SEARCH_DEPTH = 12;
const double UCT_EXPLORE_CONST = 1.5;

constexpr int WIDTH_LIMITS[MAX_SEARCH_DEPTH + 1] = {2, 3, 3, 4, 4, 5, 5, 6, 6, 8, 8, 12, 12};

constexpr int64 W_ATK[6][3] = {
    {1, 1, 1}, {1, 1, 1}, {1, 1, 2}, {1, 3, 8}, {1, 100, 10000}, {1, 1000, 10050}
};
constexpr int64 W_HOP_ATK[6][3] = {
    {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 2, 5}, {1, 110, 120}, {900, 950, 1050}
};


constexpr int64 W_DEF[6][3] = {
    {1, 1, 1}, {1, 2, 3}, {1, 5, 20}, {1, 100, 800}, {1, 20000, 50000}, {1, 50000, 100000}
};
constexpr int64 W_HOP_DEF[6][3] = {
    {1, 1, 1}, {1, 1, 1}, {1, 2, 5}, {1, 20, 80}, {1, 15000, 20000}, {9000, 10000, 20000}
};

enum Faction {
    MYSELF = 0,
    OPPONENT = 1,
    EMPTY_CELL = 2,
    OUT_OF_BOUNDS = 3
};

struct Point2D {
    int r, c;
    Point2D(int r_val = -1, int c_val = -1) {
        r = r_val;
        c = c_val;
    }
    Point2D operator+(const Point2D &other) const { return Point2D(r + other.r, c + other.c); }
    Point2D operator-(const Point2D &other) const { return Point2D(r - other.r, c - other.c); }
    Point2D operator*(int multiplier) const { return Point2D(r * multiplier, c * multiplier); }
    friend Point2D operator*(int multiplier, const Point2D &pt) { return Point2D(multiplier * pt.r, multiplier * pt.c); }
    bool operator==(const Point2D &other) const { return r == other.r && c == other.c; }
    bool operator!=(const Point2D &other) const { return r != other.r || c != other.c; }
    bool operator<(const Point2D &other) const {
        if (r == other.r) return c < other.c;
        return r < other.r;
    }
};

const Point2D DELTA_DIRS[4] = { Point2D(0, 1), Point2D(1, 0), Point2D(1, 1), Point2D(1, -1) };

class AlphaGomokuEngine;

struct CandidateAction {
    Point2D pt;
    int64 priorityScore;
    int centerDistance;

    CandidateAction() {
        pt = Point2D(-1, -1);
        priorityScore = 0;
        centerDistance = 0;
    }

    CandidateAction(AlphaGomokuEngine *enginePtr, Point2D pos);
    
    CandidateAction(Point2D pos, int64 w) {
        pt = pos;
        priorityScore = w;
        int dr = abs(pos.r - BOARD_ROWS / 2) + 1;
        int dc = abs(pos.c - BOARD_COLS / 2) + 1;
        centerDistance = dr * dc;
    }


    bool operator<(const CandidateAction &other) const {
        bool prioDiff = (priorityScore != other.priorityScore);
        if (prioDiff) {
            return priorityScore > other.priorityScore;
        }
        bool distDiff = (centerDistance != other.centerDistance);
        if (distDiff) {
            return centerDistance < other.centerDistance;
        }
        return pt < other.pt;
    }
};

struct SequenceState {
    Faction ownerSide;
    int straightLen[2];     
    bool isOpenEnd[2];  
    int jumpLen[2];  
    bool isJumpOpen[2]; 

    SequenceState() {
        straightLen[0] = straightLen[1] = 0;
        isOpenEnd[0] = isOpenEnd[1] = false;
        jumpLen[0] = jumpLen[1] = 0;
        isJumpOpen[0] = isJumpOpen[1] = false;
    }

    void mergeForward(const SequenceState &prevNode, Faction targetSide, int dirFlag) {
        if (targetSide == EMPTY_CELL) {
            straightLen[dirFlag] = 0;
            isOpenEnd[dirFlag] = true;
            jumpLen[dirFlag] = prevNode.straightLen[dirFlag] + 1;
            isJumpOpen[dirFlag] = prevNode.isOpenEnd[dirFlag];
        } else if (targetSide == ownerSide) {
            straightLen[dirFlag] = prevNode.straightLen[dirFlag] + 1;
            isOpenEnd[dirFlag] = prevNode.isOpenEnd[dirFlag];
            jumpLen[dirFlag] = prevNode.jumpLen[dirFlag] + 1;
            isJumpOpen[dirFlag] = prevNode.isJumpOpen[dirFlag];
        } else {
            straightLen[dirFlag] = 0;
            jumpLen[dirFlag] = 0;
            isOpenEnd[dirFlag] = false;
            isJumpOpen[dirFlag] = false;
        }
    }

    int64 computeThreat(bool applyDefensiveBoost) const {
        int totalConnected = straightLen[0] + straightLen[1] + 1;
        if (totalConnected >= 6) return SCORE_INFINITY;
        
        int hopLeft = jumpLen[0] + straightLen[1];
        int hopRight = straightLen[0] + jumpLen[1];

        int state1 = isOpenEnd[0] + isOpenEnd[1];
        int state2 = isJumpOpen[0] + isOpenEnd[1];
        int state3 = isOpenEnd[0] + isJumpOpen[1];

        int limitLeft = min(5, hopLeft);
        int limitRight = min(5, hopRight);

        if (applyDefensiveBoost) {
            int64 val1 = W_DEF[totalConnected][state1];
            int64 val2 = W_HOP_DEF[limitLeft][state2];
            int64 val3 = W_HOP_DEF[limitRight][state3];
            return max({val1, val2, val3});
        } else {
            int64 val1 = W_ATK[totalConnected][state1];
            int64 val2 = W_HOP_ATK[limitLeft][state2];
            int64 val3 = W_HOP_ATK[limitRight][state3];
            return max({val1, val2, val3});
        }
    }
};

class AlphaGomokuEngine {
private:
    bool isInsideBoard(Point2D pt) {
        return pt.r >= 0 && pt.r < BOARD_ROWS && pt.c >= 0 && pt.c < BOARD_COLS;
    }

    Faction getCellFaction(Point2D pt) const {
        if (pt.r < 0 || pt.r >= BOARD_ROWS || pt.c < 0 || pt.c >= BOARD_COLS) {
            return OUT_OF_BOUNDS;
        }
        return boardMatrix[pt.r][pt.c];
    }

    bool hasTimedOut() const {
        auto now = chrono::steady_clock::now();
        auto diff = chrono::duration_cast<chrono::milliseconds>(now - timestampStart);
        return diff.count() > 960;
    }

public:
    Faction boardMatrix[BOARD_ROWS][BOARD_COLS];
    SequenceState statusCache[BOARD_ROWS][BOARD_COLS][4][2];
    int64 evaluateMap[BOARD_ROWS][BOARD_COLS][2];
    
    set<CandidateAction> validMoves;
    chrono::time_point<chrono::steady_clock> timestampStart;
    bool playingBlack;

    explicit AlphaGomokuEngine(bool iAmBlack) {
        timestampStart = chrono::steady_clock::now();
        playingBlack = iAmBlack;

        for (int r = 0; r < BOARD_ROWS; ++r) {
            for (int c = 0; c < BOARD_COLS; ++c) {
                boardMatrix[r][c] = EMPTY_CELL;
                Point2D currentPt(r, c);
                
                int dirIdx = 0;
                while (dirIdx < 4) {
                    SequenceState& stateMe = statusCache[r][c][dirIdx][0];
                    SequenceState& stateOp = statusCache[r][c][dirIdx][1];
                    
                    stateMe.ownerSide = MYSELF;
                    stateOp.ownerSide = OPPONENT;
                    
                    Point2D backStep = currentPt - DELTA_DIRS[dirIdx];
                    if (isInsideBoard(backStep)) {
                        stateMe.isOpenEnd[0] = true;
                        stateOp.isOpenEnd[0] = true;
                        stateMe.jumpLen[0] = 1;
                        stateOp.jumpLen[0] = 1;
                        Point2D doubleBackStep = currentPt - DELTA_DIRS[dirIdx] * 2;
                        if (isInsideBoard(doubleBackStep)) {
                            stateMe.isJumpOpen[0] = true;
                            stateOp.isJumpOpen[0] = true;
                        }
                    }
                    
                    Point2D fwdStep = currentPt + DELTA_DIRS[dirIdx];
                    if (isInsideBoard(fwdStep)) {
                        stateMe.isOpenEnd[1] = true;
                        stateOp.isOpenEnd[1] = true;
                        stateMe.jumpLen[1] = 1;
                        stateOp.jumpLen[1] = 1;
                        Point2D doubleFwdStep = currentPt + DELTA_DIRS[dirIdx] * 2;
                        if (isInsideBoard(doubleFwdStep)) {
                            stateMe.isJumpOpen[1] = true;
                            stateOp.isJumpOpen[1] = true;
                        }
                    }
                    dirIdx++;
                }
                updateCellScore(currentPt, MYSELF);
                updateCellScore(currentPt, OPPONENT);
                validMoves.emplace(this, currentPt);
            }
        }
    }

    void updateCellScore(Point2D pt, Faction role) {
        int64 aggregateScore = 1;
        bool defCondition = (playingBlack && role == OPPONENT);

        for (int i = 0; i < 4; ++i) {
            int64 lineW = statusCache[pt.r][pt.c][i][role].computeThreat(defCondition);
            if (lineW == SCORE_INFINITY) {
                aggregateScore = SCORE_INFINITY;
                break;
            }
            aggregateScore *= lineW;
        }
        evaluateMap[pt.r][pt.c][role] = aggregateScore;
    }

    int64 queryScore(Point2D pt, Faction role) const { 
        return evaluateMap[pt.r][pt.c][role]; 
    }

    void commitPlay(Point2D pt, Faction role) {
        if (pt.r == -1) return;
        if (role == getCellFaction(pt)) return;

        if (boardMatrix[pt.r][pt.c] == EMPTY_CELL) {
            validMoves.erase({this, pt});
        }
        boardMatrix[pt.r][pt.c] = role;

        static vector<vector<int>> modifyTracker(BOARD_ROWS, vector<int>(BOARD_COLS, 0));
        static int currentStamp = 0;
        currentStamp += 1;
        vector<CandidateAction> affectedCells;

        for (int d = 0; d < 4; ++d) {
            for (int side = 1; side >= 0; --side) { 
                SequenceState originState = statusCache[pt.r][pt.c][d][side];
                
                for (int isForward = 0; isForward <= 1; ++isForward) {
                    Point2D traverseDir;
                    if (isForward == 1) traverseDir = DELTA_DIRS[d];
                    else traverseDir = Point2D(-DELTA_DIRS[d].r, -DELTA_DIRS[d].c);
                    
                    int spanLength = originState.jumpLen[isForward] + originState.isJumpOpen[isForward] + 1;
                    Point2D boundaryPt = pt + traverseDir * spanLength;
                    
                    Point2D cursor = pt + traverseDir;
                    while (cursor != boundaryPt) {
                        if (getCellFaction(cursor) == EMPTY_CELL) {
                            if (modifyTracker[cursor.r][cursor.c] != currentStamp) {
                                modifyTracker[cursor.r][cursor.c] = currentStamp;
                                affectedCells.emplace_back(this, cursor);
                            }
                        }
                        Point2D prevCursor = cursor - traverseDir;
                        int reverseFlag = isForward ^ 1;
                        statusCache[cursor.r][cursor.c][d][side].mergeForward(
                            statusCache[prevCursor.r][prevCursor.c][d][side], 
                            getCellFaction(prevCursor), 
                            reverseFlag
                        );
                        cursor = cursor + traverseDir;
                    }
                }
            }
        }

        for (size_t k = 0; k < affectedCells.size(); k++) {
            validMoves.erase(affectedCells[k]);
        }
        for (size_t k = 0; k < affectedCells.size(); k++) {
            updateCellScore(affectedCells[k].pt, MYSELF);
            updateCellScore(affectedCells[k].pt, OPPONENT);
            validMoves.emplace(this, affectedCells[k].pt);
        }
        
        if (role == EMPTY_CELL) {
            validMoves.emplace(this, pt);
        }
    }

    struct SearchTree {
        int nVisits;
        int nWins;
        Faction turnRole;
        Faction outcome;
        CandidateAction step1;
        CandidateAction step2;
        SearchTree *parentLink;
        vector<SearchTree *> childNodes;

        SearchTree(Faction role, CandidateAction s1, CandidateAction s2, SearchTree *pa, Faction endSt = EMPTY_CELL) 
            : step1(s1), step2(s2) {
            nVisits = 0;
            nWins = 0;
            turnRole = role;
            outcome = endSt;
            parentLink = pa;
        }

        void propagateResult(Faction victor) {
            nVisits += 2;
            Faction nextTurn = (Faction)(turnRole ^ 1);
            if (victor == nextTurn) {
                nWins += 2;
            } else if (victor == EMPTY_CELL) {
                nWins += 1;
            }
        }

        double calculateUCT() const {
            double exploitPart = static_cast<double>(nWins) / nVisits;
            double explorePart = UCT_EXPLORE_CONST * std::sqrt(std::log(parentLink->nVisits) / nVisits);
            return exploitPart + explorePart;
        }
    };

    Faction executeMCTS(SearchTree *nodePtr, int depthLeft = MAX_SEARCH_DEPTH) {
        if (nodePtr->nVisits == 0) {
            if (nodePtr->outcome == EMPTY_CELL && depthLeft > 0) {
                
                int64 minThreshold = std::min(1000.0, std::sqrt(validMoves.begin()->priorityScore));
                vector<CandidateAction> topCandidates;
                
                for (auto it = validMoves.begin(); it != validMoves.end(); ++it) {
                    int currentSize = topCandidates.size();
                    int widthTarget = std::max(2, WIDTH_LIMITS[depthLeft] / 2);
                    if (currentSize >= widthTarget) {
                        if (it->priorityScore < minThreshold || currentSize >= WIDTH_LIMITS[depthLeft]) {
                            break;
                        }
                    }
                    topCandidates.push_back(*it);
                }
                
                set<pair<Point2D, Point2D>> combinationSet;
                for (int idx = 0; idx < (int)topCandidates.size(); ++idx) {
                    CandidateAction actA = topCandidates[idx];
                    
                    if (queryScore(actA.pt, nodePtr->turnRole) == SCORE_INFINITY) {
                        CandidateAction partner = topCandidates[idx == 0 ? 1 : idx - 1];
                        Faction nextRole = (Faction)(nodePtr->turnRole ^ 1);
                        nodePtr->childNodes = { new SearchTree(nextRole, actA, partner, nodePtr, nodePtr->turnRole) };
                        break;
                    }
                    
                    int loopCounter = 0;
                    bool immediateWin = false;
                    commitPlay(actA.pt, nodePtr->turnRole);
                    
                    for (auto actB : validMoves) {
                        Point2D pMin = min(actA.pt, actB.pt);
                        Point2D pMax = max(actA.pt, actB.pt);
                        if (!combinationSet.insert({pMin, pMax}).second) continue;
                        
                        loopCounter++;
                        int pruneThreshold = (WIDTH_LIMITS[depthLeft] - idx) / 2 + 1;
                        if (loopCounter > pruneThreshold) break;
                        
                        Faction nextRole = (Faction)(nodePtr->turnRole ^ 1);
                        if (queryScore(actB.pt, nodePtr->turnRole) == SCORE_INFINITY) {
                            immediateWin = true;
                            nodePtr->childNodes = { new SearchTree(nextRole, actA, actB, nodePtr, nodePtr->turnRole) };
                            break;
                        }
                        nodePtr->childNodes.push_back(new SearchTree(nextRole, actA, actB, nodePtr));
                    }
                    commitPlay(actA.pt, EMPTY_CELL);
                    if (immediateWin) break;
                }
            }
        }

        if (nodePtr->childNodes.empty()) {
            nodePtr->propagateResult(nodePtr->outcome);
            return nodePtr->outcome;
        }

        double highestUCT = -1.0;
        SearchTree *chosenChild = nullptr;
        
        for (size_t i = 0; i < nodePtr->childNodes.size(); i++) {
            SearchTree *cv = nodePtr->childNodes[i];
            if (cv->nVisits == 0) {
                chosenChild = cv;
                break;
            }
            double curUCT = cv->calculateUCT();
            if (curUCT > highestUCT) {
                highestUCT = curUCT;
                chosenChild = cv;
            }
        }

        commitPlay(chosenChild->step1.pt, nodePtr->turnRole);
        commitPlay(chosenChild->step2.pt, nodePtr->turnRole);
        
        Faction simResult = executeMCTS(chosenChild, depthLeft - 1);
        
        commitPlay(chosenChild->step2.pt, EMPTY_CELL);
        commitPlay(chosenChild->step1.pt, EMPTY_CELL);
        
        nodePtr->propagateResult(simResult);
        return simResult;
    }

    Json::Value beginSearch() {
        SearchTree *rootNode = new SearchTree(MYSELF, CandidateAction(Point2D(-1, -1), 0), CandidateAction(Point2D(-1, -1), 0), nullptr);

        do {
            executeMCTS(rootNode);
        } while (!hasTimedOut());

        SearchTree *optimalNode = rootNode->childNodes[0];
        for (size_t i = 1; i < rootNode->childNodes.size(); ++i) {
            if (rootNode->childNodes[i]->nVisits > optimalNode->nVisits) {
                optimalNode = rootNode->childNodes[i];
            }
        }

        ostringstream debugTracker;
        for (size_t i = 0; i < rootNode->childNodes.size(); ++i) {
            SearchTree *cv = rootNode->childNodes[i];
            debugTracker << cv->step1.pt.r << ',' << cv->step1.pt.c << ' ' << cv->step1.priorityScore << ' ';
            debugTracker << cv->step2.pt.r << ',' << cv->step2.pt.c << ' ' << cv->step2.priorityScore << ' ';
            debugTracker << cv->nWins << '/' << cv->nVisits - cv->nWins << "    ";
        }

        Json::Value responseJson;
        responseJson["response"]["x0"] = optimalNode->step1.pt.r;
        responseJson["response"]["y0"] = optimalNode->step1.pt.c;
        responseJson["response"]["x1"] = optimalNode->step2.pt.r;
        responseJson["response"]["y1"] = optimalNode->step2.pt.c;
        responseJson["debug"] = debugTracker.str();

        return responseJson;
    }

    bool handleOpening(int turnCounter, Json::Value& outJson) {
        if (turnCounter == 1 && playingBlack) {
            outJson["response"]["x0"] = BOARD_ROWS / 2;
            outJson["response"]["y0"] = BOARD_COLS / 2;
            outJson["response"]["x1"] = -1;
            outJson["response"]["y1"] = -1;
            return true;
        }
        return false;
    }
};

CandidateAction::CandidateAction(AlphaGomokuEngine *enginePtr, Point2D pos) {
    pt = pos;
    int dr = abs(pos.r - BOARD_ROWS / 2) + 1;
    int dc = abs(pos.c - BOARD_COLS / 2) + 1;
    centerDistance = dr * dc;

    int64 scoreMe = enginePtr->queryScore(pos, MYSELF);
    int64 scoreOp = enginePtr->queryScore(pos, OPPONENT);
    
    if (enginePtr->playingBlack) {
        if (scoreMe >= SCORE_INFINITY / 100) {
            priorityScore = scoreMe * 2;
        } else if (scoreOp >= SCORE_INFINITY / 100) {
            priorityScore = scoreOp * 2;
        } else {
            priorityScore = max(scoreMe, scoreOp * 2); 
        }
    } else {
        priorityScore = max(scoreMe, scoreOp);
    }
}

int main() {
    string jsonString;
    getline(cin, jsonString);
    Json::Value parsedPayload;
    Json::Reader().parse(jsonString, parsedPayload);
    
    int turnAmount = parsedPayload["requests"].size();
    bool checkBlack = parsedPayload["requests"][0u]["x0"].asInt() == -1;
    
    AlphaGomokuEngine core(checkBlack); 
    
    for (int k = 0; k < turnAmount; k++) {
        int reqX0 = parsedPayload["requests"][k]["x0"].asInt();
        int reqY0 = parsedPayload["requests"][k]["y0"].asInt();
        int reqX1 = parsedPayload["requests"][k]["x1"].asInt();
        int reqY1 = parsedPayload["requests"][k]["y1"].asInt();

        core.commitPlay(Point2D(reqX0, reqY0), OPPONENT);
        core.commitPlay(Point2D(reqX1, reqY1), OPPONENT);
        
        if (k == turnAmount - 1) break;
        
        int resX0 = parsedPayload["responses"][k]["x0"].asInt();
        int resY0 = parsedPayload["responses"][k]["y0"].asInt();
        int resX1 = parsedPayload["responses"][k]["x1"].asInt();
        int resY1 = parsedPayload["responses"][k]["y1"].asInt();

        core.commitPlay(Point2D(resX0, resY0), MYSELF);
        core.commitPlay(Point2D(resX1, resY1), MYSELF);
    }

    Json::Value finalAction;

    if (!core.handleOpening(turnAmount, finalAction)) {
        finalAction = core.beginSearch();
    }

    cout << Json::FastWriter().write(finalAction) << endl;

    return 0;
}
