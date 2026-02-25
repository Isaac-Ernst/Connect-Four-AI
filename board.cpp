#include "board.h"
#include <iostream>
#include <cstdint>

// adds color to the console output for better visualization of the board
#define RESET "\033[0m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"

// gets number of moves
int Board::numMoves() const
{
    return numberMoves;
}

// determines if a move is valid
bool Board::checkMove(int columnNumber) const
{
    /* Shifts a 64-bit number equal to 1 to the desired
    column but shifting my the column number times the
    number of rows. Then, adds 5 to get from the botton
    row to the top. Checks with the entire board to
    determine if the position is empty.*/
    return (mask & (1ULL << (5 + columnNumber * 7))) == 0;
}

// makes a move if it is valid
bool Board::makeMove(const int columnNumber)
{
    // determines if the desired move is valid
    if (!checkMove(columnNumber))
    {
        return false;
    }

    // Uses XOR to switch between players
    currentPosition ^= mask;

    /* Makes a move by shifting a 1 to the desired
    column and row, adds it to the mask, and then
    use the OR operator to set the bit in the mask.*/
    mask |= mask + (1ULL << (columnNumber * 7));

    numberMoves++;

    // return true after the move has been made
    return true;
}

// checks for a win condition for the current player
bool Board::checkWin() const
{
    /* When you do four shifts and use the AND
    operator to either return a binary 1 and 0,
    it will check to see if any 1 aligns with
    another 1 and return true if so. */
    uint64_t pos = currentPosition ^ mask;

    // check for horizontal wins
    if ((pos & (pos >> 7) & (pos >> 14) & (pos >> 21)) != 0)
    {
        return true;
    }

    // check for vertical wins
    if ((pos & (pos >> 1) & (pos >> 2) & (pos >> 3)) != 0)
    {
        return true;
    }

    // check for left-right diagonal wins
    if ((pos & (pos >> 8) & (pos >> 16) & (pos >> 24)) != 0)
    {
        return true;
    }

    // check for right-left diagonal wins
    if ((pos & (pos >> 6) & (pos >> 12) & (pos >> 18)) != 0)
    {
        return true;
    }

    return false;
}

// helper function for score evaluation
int Board::countPatterns(uint64_t pos) const
{
    int score = 0;

    // Empty spaces on the board
    uint64_t emptySpaces = ~(mask);

    // Actually COUNT the patterns by multiplying the number of matches by the score!
    // Unstopable 3 in a row ( _ X X X _ )
    score += (int)__popcnt64(emptySpaces & (pos >> 7) & (pos >> 14) & (pos >> 21) & (emptySpaces >> 28)) * 50;

    // Checks for 3 in a row with no open ends but a space in the center ( X X _ X ) or ( X _ X X )
    score += (int)__popcnt64(pos & (emptySpaces >> 7) & (pos >> 14) & (pos >> 21)) * 10;
    score += (int)__popcnt64(pos & (pos >> 7) & (emptySpaces >> 14) & (pos >> 21)) * 10;

    score += (int)__popcnt64(pos & (emptySpaces >> 8) & (pos >> 16) & (pos >> 24)) * 10;
    score += (int)__popcnt64(pos & (pos >> 8) & (emptySpaces >> 16) & (pos >> 24)) * 10;

    score += (int)__popcnt64(pos & (emptySpaces >> 6) & (pos >> 12) & (pos >> 18)) * 10;
    score += (int)__popcnt64(pos & (pos >> 6) & (emptySpaces >> 12) & (pos >> 18)) * 10;

    // Checks for 3 in a row with an open end ( X X X _ ) or ( _ X X X )
    score += (int)__popcnt64(pos & (pos >> 7) & (pos >> 14) & (emptySpaces >> 21)) * 7;
    score += (int)__popcnt64(emptySpaces & (pos >> 7) & (pos >> 14) & (pos >> 21)) * 7;

    score += (int)__popcnt64(pos & (pos >> 1) & (pos >> 2) & (emptySpaces >> 3)) * 5;

    score += (int)__popcnt64(pos & (pos >> 8) & (pos >> 16) & (emptySpaces >> 24)) * 7;
    score += (int)__popcnt64(emptySpaces & (pos >> 8) & (pos >> 16) & (pos >> 24)) * 7;

    score += (int)__popcnt64(pos & (pos >> 6) & (pos >> 12) & (emptySpaces >> 18)) * 7;
    score += (int)__popcnt64(emptySpaces & (pos >> 6) & (pos >> 12) & (pos >> 18)) * 7;

    // Checks for 2 in a row with two spaces
    // Checks for (X X _ _), (_ _ X X), and (X _ _ X)

    // Horizontal Runways & Gaps
    score += (int)__popcnt64(pos & (pos >> 7) & (emptySpaces >> 14) & (emptySpaces >> 21)) * 3;
    score += (int)__popcnt64(emptySpaces & (emptySpaces >> 7) & (pos >> 14) & (pos >> 21)) * 3;
    score += (int)__popcnt64(pos & (emptySpaces >> 7) & (emptySpaces >> 14) & (pos >> 21)) * 3;

    // Diagonal 1 Runways & Gaps
    score += (int)__popcnt64(pos & (pos >> 8) & (emptySpaces >> 16) & (emptySpaces >> 24)) * 3;
    score += (int)__popcnt64(emptySpaces & (emptySpaces >> 8) & (pos >> 16) & (pos >> 24)) * 3;
    score += (int)__popcnt64(pos & (emptySpaces >> 8) & (emptySpaces >> 16) & (pos >> 24)) * 3;

    // Diagonal 2 Runways & Gaps
    score += (int)__popcnt64(pos & (pos >> 6) & (emptySpaces >> 12) & (emptySpaces >> 18)) * 3;
    score += (int)__popcnt64(emptySpaces & (emptySpaces >> 6) & (pos >> 12) & (pos >> 18)) * 3;
    score += (int)__popcnt64(pos & (emptySpaces >> 6) & (emptySpaces >> 12) & (pos >> 18)) * 3;

    // Horizontal Premium (_ X X _,  X _ X _,  _ X _ X)
    score += (int)__popcnt64(emptySpaces & (pos >> 7) & (pos >> 14) & (emptySpaces >> 21)) * 3;
    score += (int)__popcnt64(pos & (emptySpaces >> 7) & (pos >> 14) & (emptySpaces >> 21)) * 3;
    score += (int)__popcnt64(emptySpaces & (pos >> 7) & (emptySpaces >> 14) & (pos >> 21)) * 3;

    // Diagonal 1 Premium (_ X X _,  X _ X _,  _ X _ X)
    score += (int)__popcnt64(emptySpaces & (pos >> 8) & (pos >> 16) & (emptySpaces >> 24)) * 3;
    score += (int)__popcnt64(pos & (emptySpaces >> 8) & (pos >> 16) & (emptySpaces >> 24)) * 3;
    score += (int)__popcnt64(emptySpaces & (pos >> 8) & (emptySpaces >> 16) & (pos >> 24)) * 3;

    // Diagonal 2 Premium (_ X X _,  X _ X _,  _ X _ X)
    score += (int)__popcnt64(emptySpaces & (pos >> 6) & (pos >> 12) & (emptySpaces >> 18)) * 3;
    score += (int)__popcnt64(pos & (emptySpaces >> 6) & (pos >> 12) & (emptySpaces >> 18)) * 3;
    score += (int)__popcnt64(emptySpaces & (pos >> 6) & (emptySpaces >> 12) & (pos >> 18)) * 3;

    // Checks for 2 in a row with a space
    // Horizontal (X X _, _ X X, X _ X)
    score += (int)__popcnt64(pos & (pos >> 7) & (emptySpaces >> 14)) * 2;
    score += (int)__popcnt64(emptySpaces & (pos >> 7) & (pos >> 14)) * 2;
    score += (int)__popcnt64(pos & (emptySpaces >> 7) & (pos >> 14)) * 2;

    // Vertical (can only be open on top: X X _)
    score += (int)__popcnt64(pos & (pos >> 1) & (emptySpaces >> 2)) * 2;

    // Diagonal 1 (X X _, _ X X, X _ X)
    score += (int)__popcnt64(pos & (pos >> 8) & (emptySpaces >> 16)) * 2;
    score += (int)__popcnt64(emptySpaces & (pos >> 8) & (pos >> 16)) * 2;
    score += (int)__popcnt64(pos & (emptySpaces >> 8) & (pos >> 16)) * 2;

    // Diagonal 2 (X X _, _ X X, X _ X)
    score += (int)__popcnt64(pos & (pos >> 6) & (emptySpaces >> 12)) * 2;
    score += (int)__popcnt64(emptySpaces & (pos >> 6) & (pos >> 12)) * 2;
    score += (int)__popcnt64(pos & (emptySpaces >> 6) & (pos >> 12)) * 2;

    return score;
}

// gets the score of the current position
int Board::score() const
{
    if (checkWin())
    {
        return -1000; // If last move won, bad for current (to move) — but negamax checks this before eval
    }

    uint64_t opp_pieces = currentPosition ^ mask; // Last player (opponent to current)
    uint64_t cur_pieces = currentPosition;        // Current player

    int cur_score = countPatterns(cur_pieces);
    int opp_score = countPatterns(opp_pieces);

    // score based on position in center
    uint64_t centerMask = 0x3FULL << 21;
    uint64_t innerMiddleMask = (0x3FULL << 14) | (0x3FULL << 28);

    cur_score += (int)__popcnt64(cur_pieces & centerMask) * 3;
    opp_score += (int)__popcnt64(opp_pieces & centerMask) * 3;
    cur_score += (int)__popcnt64(cur_pieces & innerMiddleMask) * 1;
    opp_score += (int)__popcnt64(opp_pieces & innerMiddleMask) * 1;

    // score based on position in bottom 3 rows and center 3 columns
    uint64_t sweetSpotMask = (0x7ULL << 14) | (0x7ULL << 21) | (0x7ULL << 28);
    cur_score += (int)__popcnt64(cur_pieces & sweetSpotMask) * 4;
    opp_score += (int)__popcnt64(opp_pieces & sweetSpotMask) * 4;

    /* score based on parity of pieces in rows to encourage controlling the
    mathematically advantageous rows and force opponent into disadvantageous
    rows. The bottom 3 rows and center 3 columns are the most important for
    this, so we focus on those. */
    // 0x15 = 0010101 (Rows 0, 2, 4). 0x2A = 0101010 (Rows 1, 3, 5).
    uint64_t colEven = 0x15ULL;
    uint64_t colOdd = 0x2AULL;

    // Spread the column pattern across all 7 columns
    uint64_t ROW_0_2_4 = colEven | (colEven << 7) | (colEven << 14) | (colEven << 21) | (colEven << 28) | (colEven << 35) | (colEven << 42);
    uint64_t ROW_1_3_5 = colOdd | (colOdd << 7) | (colOdd << 14) | (colOdd << 21) | (colOdd << 28) | (colOdd << 35) | (colOdd << 42);

    bool isCurrentP1 = (numberMoves % 2 == 0);
    uint64_t myParity = isCurrentP1 ? ROW_0_2_4 : ROW_1_3_5;
    uint64_t oppParity = isCurrentP1 ? ROW_1_3_5 : ROW_0_2_4;

    // Award points for aligning pieces with the rows you mathematically control
    cur_score += (int)__popcnt64(cur_pieces & myParity) * 2;
    opp_score += (int)__popcnt64(opp_pieces & oppParity) * 2;

    return cur_score - opp_score; // Positive if good for current player, negative if good for opponent
}

// old scoring method for testing purposes
int Board::oldScore() const
{
    if (checkWin())
    {
        return -1000;
    }

    uint64_t opp_pieces = currentPosition ^ mask;
    uint64_t cur_pieces = currentPosition;

    // The old, naive pattern counting (no live-threat detection)
    int cur_score = 0;
    int opp_score = 0;
    cur_score += (int)__popcnt64(cur_pieces & (cur_pieces >> 7)) * 2;
    opp_score += (int)__popcnt64(opp_pieces & (opp_pieces >> 7)) * 2;
    cur_score += (int)__popcnt64(cur_pieces & (cur_pieces >> 1)) * 2;
    opp_score += (int)__popcnt64(opp_pieces & (opp_pieces >> 1)) * 2;

    // The old simple center bias
    uint64_t centerMask = 0x3FULL << 21;
    cur_score += (int)__popcnt64(cur_pieces & centerMask) * 3;
    opp_score += (int)__popcnt64(opp_pieces & centerMask) * 3;

    return cur_score - opp_score;
}

// displays the board
void Board::displayBoard() const
{
    std::cout << BLUE << "\n  0   1   2   3   4   5   6\n"
              << RESET;
    std::cout << BLUE << "-----------------------------\n"
              << RESET;

    // Start from the top row (5) and go down to the bottom row (0)
    for (int row = 5; row >= 0; row--)
    {
        std::cout << BLUE << "|" << RESET;
        for (int col = 0; col < 7; col++)
        {
            int bitIndex = row + col * 7;
            uint64_t bit = 1ULL << bitIndex;

            if ((mask & bit) == 0)
            {
                std::cout << "   " << BLUE << "|" << RESET; // Empty space
            }
            else
            {
                // Determine whose piece it is
                bool isCurrentPlayer = (currentPosition & bit) != 0;
                bool isPlayer1 = (numberMoves % 2 == 0) ? isCurrentPlayer : !isCurrentPlayer;

                if (isPlayer1)
                    std::cout << " " << RED << "X" << RESET << BLUE << " |" << RESET; // Player 1 (Red)
                else
                    std::cout << " " << YELLOW << "O" << RESET << BLUE << " |" << RESET; // AI (Yellow)
            }
        }
        std::cout << BLUE << "\n-----------------------------\n"
                  << RESET;
    }
    std::cout << "\n";
}

// mirrors the board
uint64_t Board::mirror(uint64_t key) const
{
    return ((key & 0x000000000000007FULL) << 42) | // Move Col 0 to Col 6
           ((key & 0x0000000000003F80ULL) << 28) | // Move Col 1 to Col 5
           ((key & 0x00000000001FC000ULL) << 14) | // Move Col 2 to Col 4
           ((key & 0x000000000FE00000ULL)) |       // Keep Col 3 in the middle
           ((key & 0x00000007F0000000ULL) >> 14) | // Move Col 4 to Col 2
           ((key & 0x000003F800000000ULL) >> 28) | // Move Col 5 to Col 1
           ((key & 0x0001FC0000000000ULL) >> 42);  // Move Col 6 to Col 0
}

// gets the hash of the board for the transposition table
uint64_t Board::hash(bool &isMirror) const
{
    // Start with your unique board ID
    uint64_t baseKey = currentPosition + mask;
    uint64_t mirrorKey = mirror(baseKey);

    isMirror = mirrorKey < baseKey; // Determine if the mirrored version is "smaller" for symmetry reduction
    uint64_t key = isMirror ? mirrorKey : baseKey;

    // Use a bit-mixing function to spread the bits around and reduce collisions
    key ^= key >> 30;
    key *= 0xbf58476d1ce4e5b9ULL;
    key ^= key >> 27;
    key *= 0x94d049bb133111ebULL;
    key ^= key >> 31;

    return key;
}
