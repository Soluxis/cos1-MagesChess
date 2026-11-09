#pragma once

#include "ChessBoard.h"
#include "Player.h"
#include <string>

class ChessRenderer
{
public:
    void ClearScreen() const;
    void ShowMainMenu() const;
    void ShowElementMenu(PlayerColor color) const;
    void ShowBoard(const ChessBoard& board, const Player& white,
        const Player& black, const std::string& message = "",
        PlayerColor turn = PlayerColor::White, bool check = false,
        const std::string& lastMove = "") const;
    void ShowSquareDetails(Position position, const Piece& piece,
        const Player& owner) const;
    void ShowMessage(const std::string& message) const;

private:
    char GetPieceSymbol(const Piece& piece) const;
};
