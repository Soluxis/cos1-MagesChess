#pragma once
#include "ChessBoard.h"
#include "ChessRenderer.h"
#include "Player.h"
#include "MoveValidator.h"
#include <string>

class ChessGame {
public:
    ChessGame();
    void Run();
private:
    ChessBoard mBoard;
    Player mWhite;
    Player mBlack;
    ChessRenderer mRenderer;
    PlayerColor mTurn = PlayerColor::White;
    Position mEnPassantTarget{ -1,-1 };
    int mFullMoveNumber = 1;
    std::string mLastMove;
    void StartNewGame();
    void PlayMatch();
    Element SelectElement(PlayerColor color);
    static int ReadChoice(int minimum, int maximum);
    static bool ParseSquare(const std::string& input, Position& position);
    static bool ParseMove(const std::string& input, Move& move);
};
