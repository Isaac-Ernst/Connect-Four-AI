#include "connectfour.h"
#include <iostream>

int main()
{
    ConnectFour game;
    // game.buildOpeningBook(8, 20, false); // Build the opening book with a depth of 20 for the first 8 moves using the new score function
    game.loadOpeningBook();
    game.startGame();
    return 0;
}