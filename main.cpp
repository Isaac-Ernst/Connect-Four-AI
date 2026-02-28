#include "connectfour.h"
#include <iostream>

int main(int argc, char *argv[])
{
    ConnectFour game;
    game.loadOpeningBook(); // Loads your 129,498 move masterpiece

    // API MODE: If we run `./engine.exe --api 333`
    if (argc >= 3 && std::string(argv[1]) == "--api")
    {
        std::string history = argv[2];

        // Replay the game history
        for (char c : history)
        {
            int col = c - '0'; // Convert char to int
            game.makeMove(col);
        }

        // Calculate the AI's response and print ONLY the number
        int aiMove = game.getAIMove(42, false);
        std::cout << aiMove << std::endl;

        return 0; // Shut down instantly
    }

    // NORMAL MODE: If we just run `./engine.exe`
    game.startGame();
    return 0;
}