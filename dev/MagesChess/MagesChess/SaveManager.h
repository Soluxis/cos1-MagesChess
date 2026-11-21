#pragma once
#include "ChessBoard.h"
#include "Player.h"
#include <string>
#include <vector>

struct SavedGame {
    ChessBoard board;
    Element whiteElement = Element::Scorch, blackElement = Element::Scorch;
    int whiteMana = 50, blackMana = 50;
    int whiteLastEnchant = -1, blackLastEnchant = -1;
    PlayerColor turn = PlayerColor::White;
    Position enPassant{ -1,-1 };
    int fullMove = 1, whiteTurns = 0, blackTurns = 0;
    std::string lastMove;
    std::vector<std::string> history;
};

class SaveManager {
public:
    static bool Save(const SavedGame& game, const std::string& filename, std::string& error);
    static bool Load(const std::string& filename, SavedGame& game, std::string& error);
};
