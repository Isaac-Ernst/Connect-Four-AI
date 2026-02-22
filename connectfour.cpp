#include "connectfour.h"
#include <iostream>
#include <utility>
#include <chrono>
#include <vector>
#include <iomanip>
#include <fstream>

// constructor for the ConnectFour class, initializes scores, nodes evaluated, transposition table, and history heuristic
ConnectFour::ConnectFour() : scorePlayer1(0), scorePlayer2(0),
                             nodesEvaluated(0), ttCollisions(0), ttSize(0),
                             transpositionTable(transTableSize, {0, 0, -1, -1, -1})
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
void ConnectFour::generateBookDFS(Board currentBoard, int currentMove, int maxMoves, int searchDepth)
{
    if (currentMove >= maxMoves || currentBoard.checkWin())
        return;

    bool isMirror = false;
    uint64_t boardHash = currentBoard.hash(isMirror);

    // If we haven't solved this exact board yet, solve it
    if (openingBook.find(boardHash) == openingBook.end())
    {
        this->board = currentBoard;

        int bestMove = 3;
        for (int d = 1; d <= searchDepth; d++)
        {
            bestMove = negamax(this->board, d, -9999, 9999).second;
        }

        openingBook[boardHash] = bestMove;
    }
    // Recursively play every valid column to go one move deeper
    for (int col = 0; col < 7; col++)
    {
        if (currentBoard.checkMove(col))
        {
            Board nextBoard = currentBoard;
            nextBoard.makeMove(col);
            generateBookDFS(nextBoard, currentMove + 1, maxMoves, searchDepth);
        }
    }
}

// builds the opening book by doing a depth first search of the game tree and storing the best move for each board state in the opening book
void ConnectFour::buildOpeningBook(int maxMoves, int searchDepth)
{
    std::cout << "--- STARTING BOOK GENERATION ---\n";
    std::cout << "This will take a long time. Do not close the terminal.\n";

    // Clear the transposition table so the engine runs fresh
    transpositionTable.assign(transTableSize, {0, 0, -1, -1, -1});

    // Start the recursive solver from an empty board
    Board emptyBoard;
    generateBookDFS(emptyBoard, 0, maxMoves, searchDepth);

    // Write the resulting dictionary to a highly compressed binary file
    std::ofstream outFile("opening_book.bin", std::ios::binary);
    for (const auto &pair : openingBook)
    {
        uint64_t hash = pair.first;
        uint8_t move = (uint8_t)pair.second; // Compress the move to 1 byte

        outFile.write(reinterpret_cast<const char *>(&hash), sizeof(hash));
        outFile.write(reinterpret_cast<const char *>(&move), sizeof(move));
    }
    outFile.close();

    std::cout << "--- BOOK GENERATION COMPLETE ---\n";
    std::cout << "Saved " << openingBook.size() << " positions to opening_book.bin\n";
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

// determines best possible move
std::pair<int, int> ConnectFour::negamax(const Board board, int depth, int alpha, int beta)
{
    // Store initial alpha value for transposition table flag determination
    int originalAlpha = alpha;

    // Increments the number of nodes evaluated
    nodesEvaluated++;

    // Checks the mirror state of the board for the transposition table
    bool isMirror = false;

    // Transposition table lookup
    uint64_t boardHash = board.hash(isMirror); // gets the hash of the mirrored board
    int index = boardHash % transTableSize;
    TransTEntry &ttEntry = transpositionTable[index];
    int ttBestMove = -1;

    if (ttEntry.boardHash == boardHash)
    {
        // filps the best move if the board is mirrored for symmetry reduction
        ttBestMove = isMirror ? (6 - ttEntry.bestMove) : ttEntry.bestMove;
        if (ttEntry.depth >= depth)
        {
            if (ttEntry.flag == 0)
            {
                return {ttEntry.score, ttBestMove}; // Exact Match
            }
            if (ttEntry.flag == 1 && ttEntry.score > alpha)
            {
                alpha = ttEntry.score; // Lower Bound
            }
            if (ttEntry.flag == 2 && ttEntry.score < beta)
            {
                beta = ttEntry.score; // Upper Bound
            }
            if (alpha >= beta)
            {
                return {ttEntry.score, ttBestMove}; // Cutoff!
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
    if (board.numMoves() == 42 || depth == 0)
    {
        return {board.score(), -1};
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
    std::vector<int> bestColumnSearchOrder;
    bestColumnSearchOrder.reserve(7);

    if (ttBestMove != -1 && board.checkMove(ttBestMove))
    {
        bestColumnSearchOrder.push_back(ttBestMove);
    }

    std::vector<int> remainingMoves;
    remainingMoves.reserve(7);

    int numRemaining = 0;
    int defaultOrder[7] = {3, 2, 4, 1, 5, 0, 6};

    for (int col : defaultOrder)
    {
        if (col != ttBestMove && board.checkMove(col))
        {
            remainingMoves.push_back(col);
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

    for (int col : remainingMoves)
    {
        bestColumnSearchOrder.push_back(col);
    }

    // Principle Variation Search with alpha-beta pruning
    bool firstMove = true;

    /* Iterate through the moves and determine
    best move. */
    for (int col : bestColumnSearchOrder)
    {
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
                score = -negamax(nextBoard, depth - 1, -beta, -alpha).first;
                firstMove = false;
            }
            else
            {
                // Will only search with a narrow window if it is not the first move
                score = -negamax(nextBoard, depth - 1, -alpha - 1, -alpha).first;

                // If the score is between alpha and beta, we need to re-search with the full window
                if (score > alpha && score < beta)
                {
                    score = -negamax(nextBoard, depth - 1, -beta, -score).first;
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

    // Tracks transposition table space and collisions
    if (ttEntry.depth == -1)
    {
        ttSize++;
    }
    else if (ttEntry.boardHash != boardHash)
    {
        ttCollisions++;
    }

    // ONLY overwrite if the slot is empty, it's the exact same board,
    // OR the new search is deeper (more valuable) than the old one.
    if (ttEntry.depth == -1 || ttEntry.boardHash == boardHash || depth >= ttEntry.depth)
    {
        // Store the result in the transposition table
        ttEntry.boardHash = boardHash;
        ttEntry.score = bestScore;
        ttEntry.depth = depth;
        // flips the best move if the board is mirrored for symmetry reduction
        ttEntry.bestMove = isMirror ? (6 - bestMove) : bestMove;

        if (bestScore <= originalAlpha)
        {
            ttEntry.flag = 2; // Upper Bound
        }
        else if (bestScore >= beta)
        {
            ttEntry.flag = 1; // Lower Bound
        }
        else
        {
            ttEntry.flag = 0; // Exact Match
        }
    }

    return {bestScore, bestMove};
};

// gets the move of the AI
int ConnectFour::getAIMove(int initDepth)
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

    for (int depth = 1; depth <= initDepth; depth++)
    {
        bestMove = negamax(board, depth, -9999, 9999).second;

        // Gets the time for each search depth
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        // prints status of the search for each depth
        std::cout << " Depth: " << depth << " >> "
                  << "| Search Time: " << duration.count() << "ms | "
                  << "Nodes Evaluated: " << nodesEvaluated
                  << " | TT Collisions: " << ttCollisions
                  << " | TT Space: " << std::fixed << std::setprecision(2) << 100.0 * ttSize / transTableSize << "%"
                  << " | Best move: " << bestMove << std::endl;
    }

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
            std::cout << "Player 1's Turn (X)\n";
            move = getHumanMove();
        }
        // Player 2's turn (Odd move count)
        else
        {
            std::cout << "AI is thinking (O)...\n";
            move = getAIMove(20);
            std::cout << "\nAI chose column: " << move << "\n";
        }

        // Apply the move and show the board
        board.makeMove(move);
        board.displayBoard();

        // Check for a winner (the player who JUST moved)
        if (board.checkWin())
        {
            if (board.numMoves() % 2 == 1)
            {
                std::cout << "\n*** PLAYER 1 WINS! ***\n";
            }
            else
            {
                std::cout << "\n*** AI WINS! ***\n";
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
                board = Board(); // Reset the board for a new game
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
