#pragma once

#include "ChessBoard.h"
#include "Player.h"
#include <string>
#include <vector>

class ChessRenderer
{
public:
    void ConfigureConsole() const;
    void ClearScreen() const;
    void ShowGameScreen(const ChessBoard& board, const Player& white, const Player& black,
        PlayerColor turn, bool check, const std::string& lastMove,
        const std::vector<std::string>& history, int ownTurn, bool canEnchant,
        const std::string& message, bool help, bool inspect, Position cursor,
        Position selected, const std::vector<Move>& highlights) const;
    void ShowMainMenu() const;
    void ShowElementMenu(PlayerColor color) const;
    void ShowBoard(const ChessBoard& board, const Player& white,
        const Player& black, const std::string& message = "",
        PlayerColor turn = PlayerColor::White, bool check = false,
        const std::string& lastMove = "",
        Position selected = { -1,-1 }, const std::vector<Move>& highlights = {},
        Position cursor = { -1,-1 }) const;
    void ShowSquareDetails(Position position, const Piece& piece,
        const Player& owner) const;
    void ShowMessage(const std::string& message) const;
    void ShowMatchDetails(const Player& active, int ownTurn, bool canEnchant) const;
    void ShowRecentMoves(const std::vector<std::string>& history) const;
    void ShowInformationPanel(const std::string& message, bool help, bool inspect,
        Position cursor, const Piece& piece, const Player& owner) const;
    void ShowControls() const;

private:
    char GetPieceSymbol(const Piece& piece) const;
};
