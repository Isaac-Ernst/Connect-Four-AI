#pragma once

#include "board.h"
#include <iostream>
#include <utility>       // Pair implementation for negamax return type
#include <chrono>        // Time measurement
#include <vector>        // Transposition table implementation
#include <unordered_map> // Opening book implementation
#include <mutex>         // multithreading book generation
#include <thread>

class ConnectFour
{
private:
    Board board;
    int scorePlayer1;
    int scorePlayer2;
    uint64_t nodesEvaluated;

    // Strong solver mode toggle
    bool strongSolver = false;

    std::mutex bookMutex;

    // Determines move ordering based on the history heuristic
    int historyHeuristic[2][7]; // [player][column] for move ordering

    // Transposition table (~1 GB) to store previously evaluated board states
    const int transTableSize = 67108864;
    const int sizeMask = transTableSize - 1;

    std::vector<uint64_t> transpositionTable;
    // std::vector<TransTEntry> transpositionTable;

    // Tracks transposition table hits and misses
    uint64_t ttCollisions;
    uint64_t ttSize;

    // Opening book for the first few moves to speed up the game and make it more challenging
    std::unordered_map<uint64_t, int> openingBook;

    std::pair<int, int> negamax(const Board board, int depth, int alpha, int beta, bool usingOldScoreFunction);
    // Memory-Enhanced Test Driver - searches the tree with a minimal window to get a better score estimate for the next search
    std::pair<int, int> MTD(Board currentBoard, int firstGuess, int depth, bool usingOldScoreFunction);
    int getAIMove(int initDepth, bool usingOldScoreFunction);
    int getHumanMove();
    void generateBookDFS(Board currentBoard, int currentMove, int maxMoves, int searchDepth, bool usingOldScoreFunction);

public:
    ConnectFour();
    void startGame();
    bool continueGame();
    void buildOpeningBook(int maxMoves, int searchDepth, bool usingOldScoreFunction);
    void loadOpeningBook();
    void saveOpeningBook();
};
