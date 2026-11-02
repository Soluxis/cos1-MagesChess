#pragma once

#include "ChessBoard.h"
#include "ChessRenderer.h"
#include "Player.h"
#include <string>

class ChessGame
{
public:
    ChessGame();
    void Run();

private:
    ChessBoard mBoard;
    Player mWhite;
    Player mBlack;
    ChessRenderer mRenderer;

    void StartNewGame();
    void InspectBoard();
    Element SelectElement(PlayerColor color);
    static int ReadChoice(int minimum, int maximum);
    static bool ParseSquare(const std::string& input, Position& position);
};
