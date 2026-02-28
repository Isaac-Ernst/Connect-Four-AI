#include "connectfour.h"
#include <iostream>
#include <utility>
#include <chrono>
#include <vector>
#include <iomanip>
#include <fstream>
#include <algorithm> // max

// constructor for the ConnectFour class, initializes scores, nodes evaluated, transposition table, and history heuristic
ConnectFour::ConnectFour() : scorePlayer1(0), scorePlayer2(0),
                             nodesEvaluated(0), ttCollisions(0), ttSize(0),
                             transpositionTable(transTableSize, 0ULL)
{
    int defaultHistory[7] = {0, 10, 20, 30, 20, 10, 0};

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 7; j++)
        {
            historyHeuristic[i][j] = defaultHistory[j]; // Initialize history heuristic to default values
        }
    }
}

// Recursively explores the opening tree to build the book
void ConnectFour::generateBookDFS(Board currentBoard, int currentMove, int maxMoves, int searchDepth, bool usingOldScoreFunction)
{
    if (currentMove > maxMoves || currentBoard.checkWin())
        return;

    bool isMirror = false;
    uint64_t boardHash = currentBoard.hash(isMirror);

    // 1. Check if we already solved this exact board
    bool alreadySolved = false;
    {
        std::lock_guard<std::mutex> lock(bookMutex);
        if (openingBook.find(boardHash) != openingBook.end())
        {
            alreadySolved = true;
        }
    }

    // 2. ONLY do the heavy math if it's a completely new board
    if (!alreadySolved)
    {
        int currentScore = 0;
        int canonicalBestMove = 3; // Default fallback

        for (int d = 1; d <= searchDepth; d++)
        {
            // ONLY evaluate the thread's local board!
            auto result = MTD(currentBoard, currentScore, d, usingOldScoreFunction);
            currentScore = result.first;
            if (result.second != -1)
            {
                // If the board is mirrored, we MUST flip the move before saving to the canonical dictionary!
                canonicalBestMove = isMirror ? (6 - result.second) : result.second;
            }
        }

        // safe saving for threads
        {
            std::lock_guard<std::mutex> lock(bookMutex);
            openingBook[boardHash] = canonicalBestMove;
            static int solvedCount = 0;
            solvedCount++;

            if (solvedCount % 10 == 0)
            {
                // Calculate how full the Transposition Table is
                double ttFillPercent = 100.0 * ttSize / transTableSize;

                std::cout << "\r[New Positions: " << solvedCount
                          << "] [Nodes: " << (nodesEvaluated / 1000000) << "M] "
                          << "[TT Fill: " << std::fixed << std::setprecision(2) << ttFillPercent << "%] "
                          << "[TT Collisions: " << (ttCollisions / 1000000) << "M]      " << std::flush;
            }

            // Save every 1,000 NEW positions
            if (solvedCount % 1000 == 0)
            {
                std::cout << "\n[Auto-Save] Backing up to disk...\n";
                saveOpeningBook();
            }
        }
    }

    // Recursion
    for (int col = 0; col < 7; col++)
    {
        if (currentBoard.checkMove(col))
        {
            Board nextBoard = currentBoard;
            nextBoard.makeMove(col);
            generateBookDFS(nextBoard, currentMove + 1, maxMoves, searchDepth, usingOldScoreFunction);
        }
    }
}

// builds the opening book by doing a depth first search of the game tree and storing the best move for each board state in the opening book
void ConnectFour::buildOpeningBook(int maxMoves, int searchDepth, bool usingOldScoreFunction)
{
    loadOpeningBook();
    std::vector<std::thread> threads;
    Board emptyBoard;

    bool isMirror;
    uint64_t emptyHash = emptyBoard.hash(isMirror);
    openingBook[emptyHash] = 3; // 3 is the mathematically proven best first move

    // Launch a separate thread for each of the 7 starting columns
    for (int col = 0; col < 7; col++)
    {
        threads.emplace_back([this, emptyBoard, col, maxMoves, searchDepth, usingOldScoreFunction]()
                             {
            Board firstMoveBoard = emptyBoard;
            if (firstMoveBoard.makeMove(col)) {
                generateBookDFS(firstMoveBoard, 1, maxMoves, searchDepth, usingOldScoreFunction);
            } });
    }

    for (auto &th : threads)
        th.join();
    saveOpeningBook();
}

// loads the opening book from a file
void ConnectFour::loadOpeningBook()
{
    std::ifstream inFile("opening_book.bin", std::ios::binary);
    if (!inFile.is_open())
    {
        std::cout << "No opening book found. AI will calculate from scratch.\n";
        return;
    }

    uint64_t hash;
    uint8_t move;

    while (inFile.read(reinterpret_cast<char *>(&hash), sizeof(hash)) &&
           inFile.read(reinterpret_cast<char *>(&move), sizeof(move)))
    {
        openingBook[hash] = (int)move;
    }

    inFile.close();
    std::cout << "Loaded " << openingBook.size() << " perfect opening moves into AI memory.\n";
}

// Safely serializes the RAM dictionary to the hard drive
void ConnectFour::saveOpeningBook()
{
    std::ofstream outFile("opening_book.bin", std::ios::binary);
    for (const auto &pair : openingBook)
    {
        uint64_t hash = pair.first;
        uint8_t move = (uint8_t)pair.second;

        outFile.write(reinterpret_cast<const char *>(&hash), sizeof(hash));
        outFile.write(reinterpret_cast<const char *>(&move), sizeof(move));
    }
    outFile.close();
}

// determines best possible move
std::pair<int, int> ConnectFour::negamax(const Board board, int depth, int alpha, int beta, bool usingOldScoreFunction)
{
    // Store initial alpha value for transposition table flag determination
    int originalAlpha = alpha;

    // Increments the number of nodes evaluated
    nodesEvaluated++;

    // Checks the mirror state of the board for the transposition table
    bool isMirror = false;

    uint64_t boardHash = board.hash(isMirror);
    int index = boardHash & sizeMask;
    uint32_t signature = (uint32_t)(boardHash >> 32);

    // --- UNPACK THE 64-BIT INTEGER ---
    uint64_t ttData = transpositionTable[index];
    uint32_t ttSignature = (uint32_t)(ttData >> 32);
    int ttScore = (int16_t)(ttData >> 16);
    int ttDepth = (ttData >> 10) & 0x3F;
    int ttRawMove = (ttData >> 7) & 0x7;
    int ttFlag = (ttData >> 5) & 0x3;

    int ttBestMove = -1;

    if (ttData != 0 && ttSignature == signature)
    {
        if (ttRawMove != 7)
        {
            ttBestMove = isMirror ? (6 - ttRawMove) : ttRawMove;
        }

        if (ttDepth >= depth)
        {
            if (ttFlag == 0)
            {
                return {ttScore, ttBestMove}; // Exact Match
            }
            if (ttFlag == 1 && ttScore > alpha)
            {
                alpha = ttScore; // Lower Bound
            }
            if (ttFlag == 2 && ttScore < beta)
            {
                beta = ttScore; // Upper Bound
            }
            if (alpha >= beta)
            {
                return {ttScore, ttBestMove}; // Cutoff!
            }
        }
    }

    /* First base case is to check for a win.
    This will also prioritize wins that occur
    sooner. */
    if (board.checkWin())
    {
        return {-1000 - depth, -1};
    }

    /* Second base case is to exit if the board
    is full and return nothing. */
    if ((board.numMoves() == 42 || depth == 0) && !strongSolver)
    {
        if (!usingOldScoreFunction)
        {
            return {board.score(), -1}; // use the new, improved score function for the new brain
        }
        else
        {
            return {board.oldScore(), -1}; // use the old, naive score function for the old brain (for testing purposes)
        }
    }
    else if ((board.numMoves() == 42 || depth == 0) && strongSolver)
    {
        return {0, -1}; // strong solver only evaluates wins and losses
    }

    // Initialize score and move
    int bestScore = -9999;
    int bestMove = -1;

    // History heuristic move ordering
    int currentPlayer = board.numMoves() % 2; // 0 for player 1, 1 for player 2

    /* Create a vecotr to prioritize the most
    likely best moves first. */
    // int columnSearchOrder[7] = {3, 2, 4, 1, 5, 0, 6};

    // Order the moves to prioritize the most likely best moves first
    int bestColumnSearchOrder[7];
    int numBestMoves = 0;

    if (ttBestMove != -1 && board.checkMove(ttBestMove))
    {
        bestColumnSearchOrder[numBestMoves++] = ttBestMove;
    }

    int remainingMoves[7];
    int numRemaining = 0;
    int defaultOrder[7] = {3, 2, 4, 1, 5, 0, 6};

    for (int col : defaultOrder)
    {
        if (col != ttBestMove && board.checkMove(col))
        {
            remainingMoves[numRemaining++] = col;
        }
    }

    // Insertion Sort based on History Score
    for (int i = 1; i < numRemaining; ++i)
    {
        int keyMove = remainingMoves[i];
        int keyScore = historyHeuristic[currentPlayer][keyMove];
        int j = i - 1;

        // Move elements that have a smaller history score down the line
        while (j >= 0 && historyHeuristic[currentPlayer][remainingMoves[j]] < keyScore)
        {
            remainingMoves[j + 1] = remainingMoves[j];
            j = j - 1;
        }
        remainingMoves[j + 1] = keyMove;
    }

    for (int i = 0; i < numRemaining; ++i)
    {
        bestColumnSearchOrder[numBestMoves++] = remainingMoves[i];
    }

    // Principle Variation Search with alpha-beta pruning
    bool firstMove = true;

    /* Iterate through the moves and determine
    best move. */
    for (int i = 0; i < numBestMoves; i++)
    {
        int col = bestColumnSearchOrder[i]; // Get the clean, sorted column

        if (board.checkMove(col))
        {
            Board nextBoard = board;
            nextBoard.makeMove(col);

            /* This is the recursive part. It filps the value
            of the score so that it is always the opposite of
            the best move the opponent can get. */
            int score;
            if (firstMove) // PVS assumes the first move is the best
            {
                score = -negamax(nextBoard, depth - 1, -beta, -alpha, usingOldScoreFunction).first;
                firstMove = false;
            }
            else
            {
                int reduction = 0;
                if (i >= 3 && depth >= 4)
                {
                    reduction = 1; // Reduce search by 1 full ply
                }

                // Will only search with a narrow window if it is not the first move
                score = -negamax(nextBoard, depth - 1 - reduction, -alpha - 1, -alpha, usingOldScoreFunction).first;

                if (reduction > 0 && score > alpha)
                {
                    score = -negamax(nextBoard, depth - 1, -alpha - 1, -alpha, usingOldScoreFunction).first;
                }
                // If the score is between alpha and beta, we need to re-search with the full window
                if (score > alpha && score < beta)
                {
                    score = -negamax(nextBoard, depth - 1, -beta, -score, usingOldScoreFunction).first;
                }
            }

            if (score > bestScore)
            {
                bestScore = score;
                bestMove = col;
            }

            // alpha-beta pruning implementation
            if (bestScore > alpha)
            {
                alpha = bestScore;
            }
            if (alpha >= beta) // opponent won't let this move happen
            {
                /* Update history heuristic for move ordering (tries to make a
                trap for the opponent by making this move more likely to be searched
                earlier in the future). */
                historyHeuristic[currentPlayer][col] += depth * depth; // More depth = more valuable move

                break;
            }
        }
    }

    if (ttData == 0)
    {
        ttSize++;
    }
    else if (ttSignature != signature)
    {
        ttCollisions++;
    }

    // ONLY overwrite if empty, exact match, or deeper search
    if (ttData == 0 || ttSignature == signature || depth >= ttDepth)
    {
        int flagToSave = 0;
        if (bestScore <= originalAlpha)
        {
            flagToSave = 2; // Upper Bound
        }
        else if (bestScore >= beta)
        {
            flagToSave = 1; // Lower Bound
        }

        int moveToSave = (bestMove == -1) ? 7 : (isMirror ? (6 - bestMove) : bestMove);

        // --- PACK THE BITS INTO A SINGLE 64-BIT INTEGER ---
        uint64_t packed = 0;
        packed |= (uint64_t)signature << 32;
        packed |= ((uint64_t)(uint16_t)bestScore) << 16;
        packed |= (uint64_t)(depth & 0x3F) << 10;
        packed |= (uint64_t)(moveToSave & 0x7) << 7;
        packed |= (uint64_t)(flagToSave & 0x3) << 5;
        packed |= 1ULL; // Set the very last bit to 1 so the entry is never '0'

        // Save it in one unbreakable hardware instruction
        transpositionTable[index] = packed;
    }

    return {bestScore, bestMove};
};

// searches a small window to make large alpha-beta cutoffs early into search
std::pair<int, int> ConnectFour::MTD(Board currentBoard, int firstGuess, int depth, bool usingOldScoreFunction)
{
    int guess = firstGuess;
    int upperBound = 9999;
    int lowerBound = -9999;
    int bestMove = -1;

    while (lowerBound < upperBound)
    {
        int beta = std::max(guess, lowerBound + 1);
        auto result = negamax(currentBoard, depth, beta - 1, beta, usingOldScoreFunction);
        guess = result.first;

        // Secure the best move directly
        if (result.second != -1)
        {
            bestMove = result.second;
        }

        if (beta > guess)
        {
            upperBound = guess;
        }
        else
        {
            lowerBound = guess;
        }
    }
    return {guess, bestMove};
}

// gets the move of the AI
int ConnectFour::getAIMove(int initDepth, bool usingOldScoreFunction)
{
    bool isMirror = false;
    uint64_t currentHash = board.hash(isMirror);

    // check the opening book for the best move for this board state
    if (openingBook.find(currentHash) != openingBook.end())
    {
        int bookMove = openingBook[currentHash];
        // If the board was mirrored, we must flip the move!
        int finalMove = isMirror ? (6 - bookMove) : bookMove;
        std::cout << ">>> BOOK MOVE FOUND! Playing instantly. <<<\n";
        return finalMove;
    }

    /* This is the implementation of iterative deepening. Primarily
    helps make the transposition table more effective during the deepest
    searches. */
    int bestMove = 3;   // default move is the middle column
    nodesEvaluated = 0; // zero out the number of nodes each turn
    auto start = std::chrono::steady_clock::now();
    int currentScore = 0;
    int maxDepth;

    // Switch from heuristic solver to strong solver at a certain depth
    if (board.numMoves() >= 12) // After 6 moves each, switch to strong solver that only evaluates wins and losses for faster deeper searches
    {
        if (!strongSolver)
        {
            std::cout << "Switching to strong solver mode for deeper searches...\n";
        }
        maxDepth = initDepth - board.numMoves();
        strongSolver = true;
    }
    else
    {
        maxDepth = 24;
    }

    for (int depth = 1; depth <= maxDepth; depth++)
    {
        auto result = MTD(board, currentScore, depth, usingOldScoreFunction);
        currentScore = result.first;

        // Grab the move directly (No flipping needed, MTD searches the actual board!)
        if (result.second != -1)
        {
            bestMove = result.second;
        }

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        // prints status of the search for each depth
        std::cout << "\r Depth: " << depth + board.numMoves() << " >> "
                  << "| Search Time: " << duration.count() << "ms | "
                  << "Nodes Evaluated: " << nodesEvaluated
                  << " | TT Collisions: " << ttCollisions
                  << " | TT Space: " << std::fixed << std::setprecision(2) << 100.0 * ttSize / transTableSize << "%"
                  << " | Best move: " << bestMove << "     ";
        std::cout.flush();
    }
    std::cout << "\n";

    return bestMove;
};

// gets user input
int ConnectFour::getHumanMove()
{
    int col;
    while (true)
    {
        std::cout << "Enter your move (0-6): ";
        std::cin >> col;

        // Check if the input is a valid number and a valid column
        if (std::cin.fail() || col < 0 || col > 6 || !board.checkMove(col))
        {
            std::cin.clear();             // Clear the error flag
            std::cin.ignore(10000, '\n'); // Throw away the bad input
            std::cout << "Invalid move. Please try again.\n";
        }
        else
        {
            return col;
        }
    }
}

// runs the game
void ConnectFour::startGame()
{
    std::cout << "=================================\n";
    std::cout << "       C++ CONNECT FOUR AI       \n";
    std::cout << "=================================\n";

    board.displayBoard();

    while (true)
    {
        int move;

        // Player 1's turn (Even move count)
        if (board.numMoves() % 2 == 0)
        {
            std::cout << "AI is thinking (X)...\n";
            move = getAIMove(42, false);
            std::cout << "\nAI chose column: " << move << "\n";
        }
        // Player 2's turn (Odd move count)
        else
        {
            std::cout << "Player 1's Turn (O)\n";
            move = getHumanMove();
            // move = getAIMove(42, true);
        }

        // Apply the move and show the board
        board.makeMove(move);
        board.displayBoard();

        // Check for a winner (the player who JUST moved)
        if (board.checkWin())
        {
            if (board.numMoves() % 2 == 1)
            {
                std::cout << "\n*** AI WINS! ***\n";
            }
            else
            {
                std::cout << "\n*** PLAYER 1 WINS! ***\n";
            }
        }
        else if (board.numMoves() == 42) // Check for a draw
        {
            std::cout << "\n*** IT'S A DRAW! ***\n";
        }

        // Ask if the player wants to play again
        if (board.checkWin() || board.numMoves() == 42)
        {
            if (!continueGame())
            {
                break;
            }
            else
            {
                std::cout << "\nStarting a new game...\n";
                board = Board();      // Reset the board for a new game
                strongSolver = false; // Reset strong solver mode for new game
                // transpositionTable.assign(transTableSize, {0, 0, 0, 0, 0});
                transpositionTable.assign(transTableSize, 0ULL);
                ttSize = 0;
                ttCollisions = 0;

                board.displayBoard();
            }
        }
    }
}

// asks the user if they want to play again
bool ConnectFour::continueGame()
{
    char choice;
    while (true)
    {
        std::cout << "Do you want to play again? (y/n): ";
        std::cin >> choice;

        if (choice == 'y' || choice == 'Y')
        {
            return true;
        }
        else if (choice == 'n' || choice == 'N')
        {
            return false;
        }
        else
        {
            std::cout << "Invalid input. Please enter 'y' or 'n'.\n";
        }
    }
}
