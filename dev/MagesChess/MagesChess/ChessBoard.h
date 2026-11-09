#pragma once
#include "ChessTypes.h"
#include <vector>

class ChessBoard {
public:
    static constexpr int Size = 8;
    ChessBoard();
    void Reset();
    bool IsInside(Position position) const;
    const Piece& GetPiece(Position position) const;
    void SetPiece(Position position, Piece piece);
    bool IsEmpty(Position position) const;
private:
    std::vector<std::vector<Piece>> mSquares;
    void PlaceBackRank(int row, PlayerColor color);
};
