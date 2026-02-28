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
    // We check the opponent's pieces because they are the one who just moved.
    uint64_t pos = currentPosition ^ mask;
    uint64_t m;

    // Horizontal Check
    m = pos & (pos >> 7);
    if (m & (m >> 14))
    {
        return true;
    }

    // Vertical Check
    m = pos & (pos >> 1);
    if (m & (m >> 2))
    {
        return true;
    }

    // Diagonal 1 Check (\)
    m = pos & (pos >> 8);
    if (m & (m >> 16))
    {
        return true;
    }

    // Diagonal 2 Check (/)
    m = pos & (pos >> 6);
    if (m & (m >> 12))
    {
        return true;
    }

    return false;
}

// helper function for score evaluation
int Board::countPatterns(uint64_t pos) const
{
    uint64_t empty = ~(mask);

    // Accumulators for each score weight.
    uint64_t w50 = 0, w10 = 0, w7 = 0, w5 = 0, w3 = 0, w2 = 0;

    // --- HORIZONTAL (Shift 7) ---
    uint64_t p_7 = pos >> 7, p_14 = pos >> 14, p_21 = pos >> 21;
    uint64_t e_7 = empty >> 7, e_14 = empty >> 14, e_21 = empty >> 21;

    w50 |= empty & p_7 & p_14 & p_21 & (empty >> 28); // _XXX_

    w10 |= pos & e_7 & p_14 & p_21; // X_XX
    w10 |= pos & p_7 & e_14 & p_21; // XX_X

    w7 |= pos & p_7 & p_14 & e_21;   // XXX_
    w7 |= empty & p_7 & p_14 & p_21; // _XXX

    w3 |= pos & p_7 & e_14 & e_21;   // XX__
    w3 |= empty & e_7 & p_14 & p_21; // __XX
    w3 |= pos & e_7 & e_14 & p_21;   // X__X
    w3 |= empty & p_7 & p_14 & e_21; // _XX_
    w3 |= pos & e_7 & p_14 & e_21;   // X_X_
    w3 |= empty & p_7 & e_14 & p_21; // _X_X

    w2 |= pos & p_7 & e_14;   // XX_
    w2 |= empty & p_7 & p_14; // _XX
    w2 |= pos & e_7 & p_14;   // X_X

    // --- VERTICAL (Shift 1) ---
    uint64_t p_1 = pos >> 1, p_2 = pos >> 2;
    uint64_t e_2 = empty >> 2, e_3 = empty >> 3;

    w5 |= pos & p_1 & p_2 & e_3; // XXX_ (Vertical only open on top)
    w2 |= pos & p_1 & e_2;       // XX_

    // --- DIAGONAL 1 (Shift 8) ---
    uint64_t p_8 = pos >> 8, p_16 = pos >> 16, p_24 = pos >> 24;
    uint64_t e_8 = empty >> 8, e_16 = empty >> 16, e_24 = empty >> 24;

    w10 |= pos & e_8 & p_16 & p_24;
    w10 |= pos & p_8 & e_16 & p_24;

    w7 |= pos & p_8 & p_16 & e_24;
    w7 |= empty & p_8 & p_16 & p_24;

    w3 |= pos & p_8 & e_16 & e_24;
    w3 |= empty & e_8 & p_16 & p_24;
    w3 |= pos & e_8 & e_16 & p_24;
    w3 |= empty & p_8 & p_16 & e_24;
    w3 |= pos & e_8 & p_16 & e_24;
    w3 |= empty & p_8 & e_16 & p_24;

    w2 |= pos & p_8 & e_16;
    w2 |= empty & p_8 & p_16;
    w2 |= pos & e_8 & p_16;

    // --- DIAGONAL 2 (Shift 6) ---
    uint64_t p_6 = pos >> 6, p_12 = pos >> 12, p_18 = pos >> 18;
    uint64_t e_6 = empty >> 6, e_12 = empty >> 12, e_18 = empty >> 18;

    w10 |= pos & e_6 & p_12 & p_18;
    w10 |= pos & p_6 & e_12 & p_18;

    w7 |= pos & p_6 & p_12 & e_18;
    w7 |= empty & p_6 & p_12 & p_18;

    w3 |= pos & p_6 & e_12 & e_18;
    w3 |= empty & e_6 & p_12 & p_18;
    w3 |= pos & e_6 & e_12 & p_18;
    w3 |= empty & p_6 & p_12 & e_18;
    w3 |= pos & e_6 & p_12 & e_18;
    w3 |= empty & p_6 & e_12 & p_18;

    w2 |= pos & p_6 & e_12;
    w2 |= empty & p_6 & p_12;
    w2 |= pos & e_6 & p_12;

    // Execute popcounts only once per weight class
    return ((int)__popcnt64(w50) * 50) +
           ((int)__popcnt64(w10) * 10) +
           ((int)__popcnt64(w7) * 7) +
           ((int)__popcnt64(w5) * 5) +
           ((int)__popcnt64(w3) * 3) +
           ((int)__popcnt64(w2) * 2);
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
