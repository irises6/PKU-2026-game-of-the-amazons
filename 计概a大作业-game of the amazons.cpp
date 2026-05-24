#define _CRT_SECURE_NO_WARNINGS 
#include <windows.h> 
#include <graphics.h>
#include <conio.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <ctime>
#include <string>
#include <tchar.h>
#include <algorithm>

using namespace std;

const int GRID_SIZE = 8;
const int CELL_SIZE = 60;
const int MARGIN_X = 50;
const int MARGIN_Y = 50;
const int BOARD_PIXEL_W = GRID_SIZE * CELL_SIZE;
const int WINDOW_W = 750;
const int WINDOW_H = 600;
const int EMPTY = 0;
const int BLACK_PIECE = 1;
const int WHITE_PIECE = 2;
const int ARROW = 3;
const int AI_PLAYER = BLACK_PIECE;
const int HUMAN_PLAYER = WHITE_PIECE;
const int MAX_DEPTH = 3;

enum GameState {
    MENU,
    PLAYING,
    GAME_OVER
};

enum GameMode {
    PVE_MODE,
    PVP_MODE
};

enum TurnStage {
    SELECT_PIECE,
    MOVE_PIECE,
    SHOOT_ARROW
};

struct Point {
    int r, c;   //行坐标，列坐标
    bool operator==(const Point& other) const { return r == other.r && c == other.c; }
    bool operator!=(const Point& other) const { return !(*this == other); }
    //重载运算符
};

struct Move {
    Point from;
    Point to;
    Point arrowTo;
    int score;
};



int board[GRID_SIZE][GRID_SIZE];
int currentPlayer;
GameMode currentGameMode = PVE_MODE;
GameState currentState = MENU;
TurnStage currentStage = SELECT_PIECE;
Point selectedPiece = { -1, -1 }; // 当前选中的棋子的位置
Point movedPiecePos = { -1, -1 }; // 棋子移动后的位置

// 按钮区域定义
RECT btnSave = { 550, 100, 700, 150 };
RECT btnLoad = { 550, 170, 700, 220 };
RECT btnUndo = { 550, 240, 700, 290 };
RECT btnExit = { 550, 310, 700, 360 };
RECT btnPVE = { 250, 200, 450, 260 };
RECT btnPVP = { 250, 280, 450, 340 };
RECT btnLoadGame = { 250, 360, 450, 420 };
RECT btnExitGame = { 250, 440, 450, 500 };

struct History {
    int board[GRID_SIZE][GRID_SIZE];
    int player;
    int GameMode;
};
std::vector<History> historyStack;

void initGame();
void drawBoard();
void drawPieces();
void drawMenu();
void drawGameSideBar();
bool isValidMove(Point from, Point to, bool isShooting);
void computerMove();
bool checkWin(int player);
void saveGame();
void loadGame();
void undoMove();
void saveStateToHistory();
Point getGridPos(int x, int y);
bool isInButton(int x, int y, RECT btn);
void drawButton(RECT btn, const TCHAR* text);


Point getGridPos(int x, int y) {
    int c = (x - MARGIN_X) / CELL_SIZE;
    int r = (y - MARGIN_Y) / CELL_SIZE;
    if (c >= 0 && c < GRID_SIZE && r >= 0 && r < GRID_SIZE)
        return { r, c };
    return { -1, -1 };
}

bool isInButton(int x, int y, RECT btn) {
    return (x >= btn.left && x <= btn.right && y >= btn.top && y <= btn.bottom);
}

void drawButton(RECT btn, const TCHAR* text) {
    setfillcolor(LIGHTGRAY);
    fillrectangle(btn.left, btn.top, btn.right, btn.bottom);
    setbkmode(TRANSPARENT);
    settextcolor(BLACK);
    settextstyle(20, 0, _T("微软雅黑"));

    int textW = textwidth(text);
    int textH = textheight(text);
    int boxW = btn.right - btn.left;
    int boxH = btn.bottom - btn.top;
    outtextxy(btn.left + (boxW - textW) / 2, btn.top + (boxH - textH) / 2, text);
}

void initGame() {
    for (int i = 0; i < GRID_SIZE; i++)
        for (int j = 0; j < GRID_SIZE; j++)
            board[i][j] = EMPTY;

    board[7][2] = WHITE_PIECE; board[7][5] = WHITE_PIECE;
    board[5][0] = WHITE_PIECE; board[5][7] = WHITE_PIECE;

    board[0][2] = BLACK_PIECE; board[0][5] = BLACK_PIECE;
    board[2][0] = BLACK_PIECE; board[2][7] = BLACK_PIECE;

    currentPlayer = WHITE_PIECE;
    currentStage = SELECT_PIECE;
    selectedPiece = { -1, -1 };
    movedPiecePos = { -1, -1 };
    historyStack.clear();
    saveStateToHistory();
}

bool isValidMove(Point from, Point to, bool isShooting) {
    if (to.r < 0 || to.r >= GRID_SIZE || to.c < 0 || to.c >= GRID_SIZE) return false;

    if (board[to.r][to.c] != EMPTY) return false;

    int dr = to.r - from.r;
    int dc = to.c - from.c;
    if (dr == 0 && dc == 0) return false;
    if (dr != 0 && dc != 0 && abs(dr) != abs(dc)) return false;

    int stepR = (dr == 0) ? 0 : (dr > 0 ? 1 : -1);
    int stepC = (dc == 0) ? 0 : (dc > 0 ? 1 : -1);
    int curR = from.r + stepR;
    int curC = from.c + stepC;
    while (curR != to.r || curC != to.c) {
        if (board[curR][curC] != EMPTY) return false;
        curR += stepR;
        curC += stepC;
    }

    return true;
}

bool checkWin(int player) {
    int dx[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
    int dy[] = { -1, 0, 1, -1, 1, -1, 0, 1 };

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (board[i][j] == player) {
                for (int k = 0; k < 8; k++) {
                    int nx = i + dx[k];
                    int ny = j + dy[k];
                    if (nx >= 0 && nx < GRID_SIZE && ny >= 0 && ny < GRID_SIZE && board[nx][ny] == EMPTY) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

void saveStateToHistory() {
    History h;
    memcpy(h.board, board, sizeof(board));
    h.player = currentPlayer;
    h.GameMode = currentGameMode;
    historyStack.push_back(h);
}

void undoMove() {
    int stepsToUndo = (currentGameMode == PVE_MODE) ? 2 : 1;
    if (historyStack.size() > stepsToUndo) {
        for (int i = 0; i < stepsToUndo; ++i) {
            historyStack.pop_back();
        }
        History h = historyStack.back();
        memcpy(board, h.board, sizeof(board));
        currentPlayer = h.player;
        currentStage = SELECT_PIECE;
        selectedPiece = { -1, -1 };
        movedPiecePos = { -1, -1 };
        if (historyStack.size() >= 2)
            currentPlayer = (currentPlayer == WHITE_PIECE ? BLACK_PIECE : WHITE_PIECE);

        MessageBox(GetHWnd(), _T("悔棋成功"), _T("提示"), MB_OK);
    }
    else {
        MessageBox(GetHWnd(), _T("无法悔棋"), _T("提示"), MB_OK);
    }
}

void saveGame() {
    FILE* fp;
    _tfopen_s(&fp, _T("amazons_save.dat"), _T("wb"));
    if (fp) {
        fwrite(board, sizeof(board), 1, fp);
        fwrite(&currentPlayer, sizeof(int), 1, fp);
        fwrite(&currentGameMode, sizeof(GameMode), 1, fp);
        fclose(fp);
        MessageBox(GetHWnd(), _T("存档成功"), _T("提示"), MB_OK);
    }
    else {
        MessageBox(GetHWnd(), _T("存档失败"), _T("错误"), MB_OK);
    }
}

void loadGame() {
    FILE* fp;
    _tfopen_s(&fp, _T("amazons_save.dat"), _T("rb"));
    if (fp) {
        fread(board, sizeof(board), 1, fp);
        fread(&currentPlayer, sizeof(int), 1, fp);
        fread(&currentGameMode, sizeof(GameMode), 1, fp);
        fclose(fp);
        currentState = PLAYING;
        currentStage = SELECT_PIECE;
        selectedPiece = { -1, -1 };
        movedPiecePos = { -1, -1 };
        historyStack.clear();
        saveStateToHistory();
        MessageBox(GetHWnd(), _T("读档成功"), _T("提示"), MB_OK);
    }
    else {
        MessageBox(GetHWnd(), _T("无存档记录"), _T("错误"), MB_OK);
    }
}

void drawBoard() {
    setbkcolor(RGB(240, 217, 181));
    cleardevice();

    setlinecolor(BLACK);
    for (int i = 0; i <= GRID_SIZE; i++) {
        line(MARGIN_X, MARGIN_Y + i * CELL_SIZE, MARGIN_X + BOARD_PIXEL_W, MARGIN_Y + i * CELL_SIZE);
        line(MARGIN_X + i * CELL_SIZE, MARGIN_Y, MARGIN_X + i * CELL_SIZE, MARGIN_Y + BOARD_PIXEL_W);
    }
}

void drawPieces() {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            int x = MARGIN_X + j * CELL_SIZE;
            int y = MARGIN_Y + i * CELL_SIZE;

            if (board[i][j] == BLACK_PIECE) {
                setfillcolor(BLACK);
                setlinecolor(BLACK);
                fillcircle(x + CELL_SIZE / 2, y + CELL_SIZE / 2, CELL_SIZE / 2 - 5);
            }
            else if (board[i][j] == WHITE_PIECE) {
                setfillcolor(WHITE);
                setlinecolor(BLACK);
                fillcircle(x + CELL_SIZE / 2, y + CELL_SIZE / 2, CELL_SIZE / 2 - 5);
            }
            else if (board[i][j] == ARROW) {
                setlinecolor(RED);
                setlinestyle(PS_SOLID, 3);
                circle(x + CELL_SIZE / 2, y + CELL_SIZE / 2, CELL_SIZE / 4);
                line(x + 10, y + 10, x + CELL_SIZE - 10, y + CELL_SIZE - 10);
                line(x + CELL_SIZE - 10, y + 10, x + 10, y + CELL_SIZE - 10);
                setlinestyle(PS_SOLID, 1);
            }
        }
    }

    // 高亮选中的棋子
    if (selectedPiece.r != -1) {
        setlinecolor(GREEN);
        setlinestyle(PS_SOLID, 3);
        int x = MARGIN_X + selectedPiece.c * CELL_SIZE;
        int y = MARGIN_Y + selectedPiece.r * CELL_SIZE;
        rectangle(x, y, x + CELL_SIZE, y + CELL_SIZE);
        setlinestyle(PS_SOLID, 1);
    }
    //高亮移动的位置
    if (currentStage == SHOOT_ARROW && movedPiecePos.r != -1) {
        setlinecolor(BLUE);
        setlinestyle(PS_SOLID, 3);
        int x = MARGIN_X + movedPiecePos.c * CELL_SIZE;
        int y = MARGIN_Y + movedPiecePos.r * CELL_SIZE;
        rectangle(x, y, x + CELL_SIZE, y + CELL_SIZE);
        setlinestyle(PS_SOLID, 1);
    }
}

void drawGameSideBar() {
    drawButton(btnSave, _T("保存进度"));
    drawButton(btnLoad, _T("读取进度"));
    drawButton(btnUndo, _T("悔棋"));
    drawButton(btnExit, _T("退出游戏"));

    settextcolor(BLACK);
    settextstyle(20, 0, _T("黑体"));

    const TCHAR* playerTypeText;
    if (currentGameMode == PVE_MODE) {
        playerTypeText = (currentPlayer == WHITE_PIECE) ? _T("人类 (白)") : _T("电脑 (黑)");
    }
    else {
        playerTypeText = (currentPlayer == WHITE_PIECE) ? _T("玩家一 (白)") : _T("玩家二 (黑)");
    }

    TCHAR turnText[50];
    _stprintf_s(turnText, _T("当前回合: %s"), playerTypeText);
    outtextxy(550, 50, turnText);

    if (currentStage == SELECT_PIECE) outtextxy(550, 80, _T("请选择棋子"));
    else if (currentStage == MOVE_PIECE) outtextxy(550, 80, _T("请移动棋子"));
    else if (currentStage == SHOOT_ARROW) outtextxy(550, 80, _T("请放置障碍"));
}

void drawMenu() {
    setbkcolor(WHITE);
    cleardevice();

    settextcolor(BLACK);
    settextstyle(50, 0, _T("黑体"));
    outtextxy(250, 100, _T("亚马逊棋"));

    drawButton(btnPVE, _T("人机对战"));
    drawButton(btnPVP, _T("双人对战"));
    drawButton(btnLoadGame, _T("读取存档"));
    drawButton(btnExitGame, _T("退出"));
}

int getMobility(int currentBoard[GRID_SIZE][GRID_SIZE], int r, int c) {
    int mobility = 0;
    int dr[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
    int dc[] = { -1, 0, 1, -1, 1, -1, 0, 1 };

    for (int k = 0; k < 8; k++) {
        int stepR = dr[k];
        int stepC = dc[k];
        int curR = r + stepR;
        int curC = c + stepC;

        while (curR >= 0 && curR < GRID_SIZE && curC >= 0 && curC < GRID_SIZE && currentBoard[curR][curC] == EMPTY) {
            mobility++;
            curR += stepR;
            curC += stepC;
        }
    }
    return mobility;
}

int evaluateBoard(int currentBoard[GRID_SIZE][GRID_SIZE], int player) {
    int score = 0;
    int opponent = (player == WHITE_PIECE) ? BLACK_PIECE : WHITE_PIECE;

    int playerMobility = 0;
    int opponentMobility = 0;

    for (int r = 0; r < GRID_SIZE; r++) {
        for (int c = 0; c < GRID_SIZE; c++) {
            if (currentBoard[r][c] == player) {
                playerMobility += getMobility(currentBoard, r, c);
            }
            else if (currentBoard[r][c] == opponent) {
                opponentMobility += getMobility(currentBoard, r, c);
            }
        }
    }
    score += (playerMobility - opponentMobility) * 10;
    return score;
}

bool isGameOver(int currentBoard[GRID_SIZE][GRID_SIZE], int player) {
    int dr[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
    int dc[] = { -1, 0, 1, -1, 1, -1, 0, 1 };

    for (int r = 0; r < GRID_SIZE; r++) {
        for (int c = 0; c < GRID_SIZE; c++) {
            if (currentBoard[r][c] == player) {
                for (int k = 0; k < 8; k++) {
                    int stepR = dr[k];
                    int stepC = dc[k];
                    int curR = r + stepR;
                    int curC = c + stepC;

                    while (curR >= 0 && curR < GRID_SIZE && curC >= 0 && curC < GRID_SIZE && currentBoard[curR][curC] == EMPTY) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

int minimax(int currentBoard[GRID_SIZE][GRID_SIZE], int depth, int alpha, int beta, bool isMaximizingPlayer) {
    int currentPlayer = isMaximizingPlayer ? AI_PLAYER : HUMAN_PLAYER;
    if (depth == 0 || isGameOver(currentBoard, currentPlayer)) {
        if (isGameOver(currentBoard, currentPlayer)) {
            return isMaximizingPlayer ? -1000000 : 1000000;
        }
        return evaluateBoard(currentBoard, AI_PLAYER);
    }

    std::vector<Move> possibleMoves; // 生成所有可能的走法 (Move-Shoot 组合)
    int dr[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
    int dc[] = { -1, 0, 1, -1, 1, -1, 0, 1 };

    std::vector<Point> pieces; // 找到当前玩家的所有棋子
    for (int r = 0; r < GRID_SIZE; r++) {
        for (int c = 0; c < GRID_SIZE; c++) {
            if (currentBoard[r][c] == currentPlayer) {
                pieces.push_back({ r, c });
            }
        }
    }

    for (const auto& p : pieces) {
        // 生成所有 (Move, ArrowShoot) 组合
        for (int moveK = 0; moveK < 8; moveK++) {
            int stepR = dr[moveK];
            int stepC = dc[moveK];
            int curR = p.r + stepR;
            int curC = p.c + stepC;

            while (curR >= 0 && curR < GRID_SIZE && curC >= 0 && curC < GRID_SIZE && currentBoard[curR][curC] == EMPTY) {
                Point to = { curR, curC };

                currentBoard[p.r][p.c] = EMPTY;
                currentBoard[to.r][to.c] = currentPlayer;

                for (int shootK = 0; shootK < 8; shootK++) {
                    int stepShootR = dr[shootK];
                    int stepShootC = dc[shootK];
                    int curShootR = to.r + stepShootR;
                    int curShootC = to.c + stepShootC;

                    while (curShootR >= 0 && curShootR < GRID_SIZE && curShootC >= 0 && curShootC < GRID_SIZE && currentBoard[curShootR][curShootC] == EMPTY) {
                        Point arrowTo = { curShootR, curShootC };

                        possibleMoves.push_back({ p, to, arrowTo, 0 });

                        curShootR += stepShootR;
                        curShootC += stepShootC;
                    }
                }

                // 回溯
                currentBoard[to.r][to.c] = EMPTY;
                currentBoard[p.r][p.c] = currentPlayer;

                curR += stepR;
                curC += stepC;
            }
        }
    }

    if (possibleMoves.empty()) {
        return isMaximizingPlayer ? -1000000 : 1000000;
    }

    if (isMaximizingPlayer) {
        int maxEval = -10000000;
        for (const auto& move : possibleMoves) {
            // 虚拟棋盘上执行走法
            int tempBoard[GRID_SIZE][GRID_SIZE];
            memcpy(tempBoard, currentBoard, sizeof(tempBoard));
            tempBoard[move.from.r][move.from.c] = EMPTY;
            tempBoard[move.to.r][move.to.c] = currentPlayer;
            tempBoard[move.arrowTo.r][move.arrowTo.c] = ARROW;

            int eval = minimax(tempBoard, depth - 1, alpha, beta, false);
            maxEval = max(maxEval, eval);
            alpha = max(alpha, eval);

            if (beta <= alpha) {
                break;
            }
        }
        return maxEval;
    }
    else {
        int minEval = 10000000;
        for (const auto& move : possibleMoves) {
            int tempBoard[GRID_SIZE][GRID_SIZE];
            memcpy(tempBoard, currentBoard, sizeof(tempBoard));
            tempBoard[move.from.r][move.from.c] = EMPTY;
            tempBoard[move.to.r][move.to.c] = currentPlayer;
            tempBoard[move.arrowTo.r][move.arrowTo.c] = ARROW;

            int eval = minimax(tempBoard, depth - 1, alpha, beta, true);
            minEval = min(minEval, eval);
            beta = min(beta, eval);

            if (beta <= alpha) {
                break;
            }
        }
        return minEval;
    }
}


Move findBestMove() {
    int currentBoard[GRID_SIZE][GRID_SIZE];
    memcpy(currentBoard, board, sizeof(currentBoard)); // 复制当前棋盘状态

    std::vector<Move> possibleMoves;
    int dr[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
    int dc[] = { -1, 0, 1, -1, 1, -1, 0, 1 };

    std::vector<Point> pieces; // 找到 AI 玩家的所有棋子
    for (int r = 0; r < GRID_SIZE; r++) {
        for (int c = 0; c < GRID_SIZE; c++) {
            if (currentBoard[r][c] == AI_PLAYER) {
                pieces.push_back({ r, c });
            }
        }
    }

    // 生成所有 (Move, ArrowShoot) 组合 并进行一步启发式评估
    for (const auto& p : pieces) {
        // 遍历所有可能的移动终点
        for (int moveK = 0; moveK < 8; moveK++) {
            int stepR = dr[moveK];
            int stepC = dc[moveK];
            int curR = p.r + stepR;
            int curC = p.c + stepC;

            while (curR >= 0 && curR < GRID_SIZE && curC >= 0 && curC < GRID_SIZE && currentBoard[curR][curC] == EMPTY) {
                Point to = { curR, curC };

                currentBoard[p.r][p.c] = EMPTY;
                currentBoard[to.r][to.c] = AI_PLAYER;

                for (int shootK = 0; shootK < 8; shootK++) {
                    int stepShootR = dr[shootK];
                    int stepShootC = dc[shootK];
                    int curShootR = to.r + stepShootR;
                    int curShootC = to.c + stepShootC;

                    while (curShootR >= 0 && curShootR < GRID_SIZE && curShootC >= 0 && curShootC < GRID_SIZE && currentBoard[curShootR][curShootC] == EMPTY) {
                        Point arrowTo = { curShootR, curShootC };

                        Move currentMove = { p, to, arrowTo, 0 };

                        // 执行走法到临时棋盘，进行一步评估
                        int tempBoard[GRID_SIZE][GRID_SIZE];
                        memcpy(tempBoard, currentBoard, sizeof(tempBoard));
                        tempBoard[arrowTo.r][arrowTo.c] = ARROW; 

                        currentMove.score = evaluateBoard(tempBoard, AI_PLAYER);
                        possibleMoves.push_back(currentMove);

                        curShootR += stepShootR;
                        curShootC += stepShootC;
                    }
                }

                // 回溯
                currentBoard[to.r][to.c] = EMPTY;
                currentBoard[p.r][p.c] = AI_PLAYER;

                curR += stepR;
                curC += stepC;
            }
        }
    }

    if (possibleMoves.empty()) {
        return { {-1, -1}, {-1, -1}, {-1, -1}, -10000000 };
    }

    // 对走法进行排序 
    std::sort(possibleMoves.begin(), possibleMoves.end(), [](const Move& a, const Move& b) {
        return a.score > b.score; 
        });

    int maxScore = -10000000;
    Move bestMove = { {-1, -1}, {-1, -1}, {-1, -1}, -10000000 };

    // 只对排名前 50 的走法进行深度搜索
    const int MAX_MOVES_TO_SEARCH = 50;
    int searchCount = 0;

    for (auto& move : possibleMoves) {
        if (searchCount >= MAX_MOVES_TO_SEARCH) break;

        int tempBoard[GRID_SIZE][GRID_SIZE];
        memcpy(tempBoard, board, sizeof(tempBoard));
        tempBoard[move.from.r][move.from.c] = EMPTY;
        tempBoard[move.to.r][move.to.c] = AI_PLAYER;
        tempBoard[move.arrowTo.r][move.arrowTo.c] = ARROW;

        move.score = minimax(tempBoard, MAX_DEPTH - 1, -10000000, 10000000, false);

        if (move.score > maxScore) {
            maxScore = move.score;
            bestMove = move;
        }
        searchCount++;
    }

    return bestMove;
}

void computerMove() {
    Move bestMove = findBestMove();

    if (bestMove.from.r != -1) {
        // 执行最佳走法
        board[bestMove.from.r][bestMove.from.c] = EMPTY;
        board[bestMove.to.r][bestMove.to.c] = BLACK_PIECE;
        board[bestMove.arrowTo.r][bestMove.arrowTo.c] = ARROW;

        movedPiecePos = bestMove.to;
        selectedPiece = { -1, -1 };
    }
}

// 宏定义
#define PLAYER_TURN_LOGIC(PIECE_COLOR, NEXT_PLAYER) \
    if (peekmessage(&msg, EM_MOUSE)) { \
        if (msg.message == WM_LBUTTONDOWN) { \
            if (isInButton(msg.x, msg.y, btnSave)) saveGame(); \
            else if (isInButton(msg.x, msg.y, btnLoad)) loadGame(); \
            else if (isInButton(msg.x, msg.y, btnExit)) currentState = MENU; \
            else if (isInButton(msg.x, msg.y, btnUndo)) { \
                undoMove(); \
            } \
            else { \
                Point p = getGridPos(msg.x, msg.y); \
                if (p.r != -1) { \
                    if (currentStage == SELECT_PIECE) { \
                        if (board[p.r][p.c] == PIECE_COLOR) { \
                            selectedPiece = p; \
                            currentStage = MOVE_PIECE; \
                        } \
                    } \
                    else if (currentStage == MOVE_PIECE) { \
                        if (board[p.r][p.c] == PIECE_COLOR) { \
                            selectedPiece = p; \
                        } \
                        else if (isValidMove(selectedPiece, p, false)) { \
                            board[p.r][p.c] = PIECE_COLOR; \
                            board[selectedPiece.r][selectedPiece.c] = EMPTY; \
                            movedPiecePos = p; \
                            selectedPiece = { -1, -1 }; \
                            currentStage = SHOOT_ARROW; \
                        } \
                    } \
                    else if (currentStage == SHOOT_ARROW) { \
                        if (isValidMove(movedPiecePos, p, true)) { \
                            board[p.r][p.c] = ARROW; \
                            saveStateToHistory(); \
                            currentPlayer = NEXT_PLAYER; \
                            currentStage = SELECT_PIECE; \
                            movedPiecePos = { -1, -1 }; \
                        } \
                    } \
                } \
            } \
        } \
    }


int main() {
    initgraph(WINDOW_W, WINDOW_H);
    setbkmode(TRANSPARENT);
    BeginBatchDraw();

    while (true) {
        ExMessage msg;

        if (currentState == MENU) {
            drawMenu();
            FlushBatchDraw();

            if (peekmessage(&msg, EM_MOUSE)) {
                if (msg.message == WM_LBUTTONDOWN) {
                    if (isInButton(msg.x, msg.y, btnPVE)) {
                        initGame();
                        currentGameMode = PVE_MODE;
                        currentState = PLAYING;
                    }
                    else if (isInButton(msg.x, msg.y, btnPVP)) {
                        initGame();
                        currentGameMode = PVP_MODE;
                        currentState = PLAYING;
                    }
                    else if (isInButton(msg.x, msg.y, btnLoadGame)) {
                        loadGame();
                    }
                    else if (isInButton(msg.x, msg.y, btnExitGame)) {
                        break;
                    }
                }
            }
        }

        else if (currentState == PLAYING) {
            drawBoard();
            drawPieces();
            drawGameSideBar();
            FlushBatchDraw();

            if (checkWin(currentPlayer)) {
                TCHAR res[50];
                int winner = (currentPlayer == WHITE_PIECE) ? BLACK_PIECE : WHITE_PIECE;
                const TCHAR* winnerName;
                if (currentGameMode == PVP_MODE) {
                    winnerName = (winner == WHITE_PIECE ? _T("玩家一 (白)") : _T("玩家二 (黑)"));
                }
                else {
                    winnerName = (winner == WHITE_PIECE ? _T("人类 (白)") : _T("电脑 (黑)"));
                }
                _stprintf_s(res, _T("游戏结束! %s 获胜!"), winnerName);
                MessageBox(GetHWnd(), res, _T("结束"), MB_OK);
                currentState = MENU;
                continue;
            }

            if (currentPlayer == WHITE_PIECE) {
                PLAYER_TURN_LOGIC(WHITE_PIECE, BLACK_PIECE);
            }
            else {
                if (currentGameMode == PVE_MODE) {
                    computerMove();
                    saveStateToHistory();
                    currentPlayer = WHITE_PIECE;
                }
                else if (currentGameMode == PVP_MODE) {
                    PLAYER_TURN_LOGIC(BLACK_PIECE, WHITE_PIECE);
                }
            }
        }
    }

    EndBatchDraw();
    closegraph();
    return 0;
}