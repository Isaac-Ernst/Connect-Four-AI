#pragma once

#include <cstdint>

/*

Board Representation using bitborads:

 0 1 2 3 4 5 6 (columns)
|0 0 0 0 0 0 0| (top row)
|0 0 0 0 0 0 0|
|0 0 0 0 0 0 0|
|0 0 0 0 0 0 0|
|0 0 0 0 0 0 0|
|0 0 0 0 0 0 0| (bottom row)
 0 0 0 0 0 0 0  (ghost row)
---------------

Shift by 7 to the left to move left a column.

Column-major ordering. The bottom row is the least significant
bits, and the top row is the most significant bits. The ghost row
is used to detect wins without needing to check for out-of-bounds
conditions.

*/

class Board
{
private:
    uint64_t mask;                         // 1 where ever there is a piece
    uint64_t currentPosition;              // 1 where there is a piece of the current player
    int numberMoves;                       // determines current player, and optimize win checking
    int countPatterns(uint64_t pos) const; // helper function for score evaluation
    uint64_t mirror(uint64_t key) const;   // helper function for symmetry reduction in transposition table

public:
    Board() : mask(0ULL), currentPosition(0ULL), numberMoves(0) {}
    int numMoves() const;
    bool checkMove(int columnNumber) const;
    bool makeMove(const int columnNumber);
    bool checkWin() const;
    int score() const;
    void displayBoard() const;
    uint64_t hash(bool &isMirror) const;
};
