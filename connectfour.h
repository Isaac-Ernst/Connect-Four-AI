#pragma once

#include "board.h"
#include <iostream>
#include <utility>       // Pair implementation for negamax return type
#include <chrono>        // Time measurement
#include <vector>        // Transposition table implementation
#include <unordered_map> // Opening book implementation

// Struct for transposition table entries
struct TransTEntry
{
    // uint64_t boardHash; // key
    uint32_t signature; // to verify the entry is correct (can be a portion of the hash)
    int16_t score;
    uint16_t depth : 6;
    uint16_t bestMove : 3;
    uint16_t flag : 2;
};

class ConnectFour
{
private:
    Board board;
    int scorePlayer1;
    int scorePlayer2;
    uint64_t nodesEvaluated;

    // Strong solver mode toggle
    bool strongSolver = false;

    // Determines move ordering based on the history heuristic
    int historyHeuristic[2][7]; // [player][column] for move ordering

    // Transposition table (~1 GB) to store previously evaluated board states
    const int transTableSize = 67108879; // A prime number to reduce collisions
    std::vector<TransTEntry> transpositionTable;

    // Tracks transposition table hits and misses
    uint64_t ttCollisions;
    uint64_t ttSize;

    // Opening book for the first few moves to speed up the game and make it more challenging
    std::unordered_map<uint64_t, int> openingBook;

    std::pair<int, int> negamax(const Board board, int depth, int alpha, int beta);
    // Memory-Enhanced Test Driver - searches the tree with a minimal window to get a better score estimate for the next search
    int MTD(int firstGuess, int depth);
    int getAIMove(int initDepth);
    int getHumanMove();
    void generateBookDFS(Board currentBoard, int currentMove, int maxMoves, int searchDepth);

public:
    ConnectFour();
    void startGame();
    bool continueGame();
    void buildOpeningBook(int maxMoves, int searchDepth);
    void loadOpeningBook();
};
