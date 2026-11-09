#pragma once
#include "ChessBoard.h"
#include <vector>

// Standard chess rules ONLY. Elemental movement must never be used for king attacks.
class MoveValidator {
public:
    static bool IsSquareAttacked(const ChessBoard& board, Position square, PlayerColor by);
    static bool IsInCheck(const ChessBoard& board, PlayerColor side);
    static std::vector<Move> LegalMoves(const ChessBoard& board, PlayerColor side,
        Position enPassantTarget = { -1,-1 });
    static bool IsLegal(const ChessBoard& board, PlayerColor side, const Move& move,
        Position enPassantTarget = { -1,-1 });
    static void Apply(ChessBoard& board, const Move& move, Position enPassantTarget = { -1,-1 });
    static bool HasLegalMoves(const ChessBoard& board, PlayerColor side,
        Position enPassantTarget = { -1,-1 });
private:
    static bool Attacks(const ChessBoard& board, Position from, Position to);
    static bool ClearPath(const ChessBoard& board, Position from, Position to);
    static std::vector<Move> PseudoMoves(const ChessBoard& board, PlayerColor side,
        Position enPassantTarget);
};
