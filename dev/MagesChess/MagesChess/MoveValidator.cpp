#include "MoveValidator.h"
#include <algorithm>
#include <cmath>

namespace {
    int Sign(int n) { return (n > 0) - (n < 0); }
    bool IsEnemy(const ChessBoard& b, Position p, PlayerColor c) {
        return b.IsInside(p) && !b.IsEmpty(p) && b.GetPiece(p).color != c;
    }
    bool CanLand(const ChessBoard& b, Position p, PlayerColor c) {
        return b.IsInside(p) && (b.IsEmpty(p) || IsEnemy(b, p, c)) &&
            (b.IsEmpty(p) || b.GetPiece(p).type != PieceType::King);
    }
}

bool MoveValidator::ClearPath(const ChessBoard& b, Position a, Position z) {
    int dr = Sign(z.row - a.row), dc = Sign(z.column - a.column);
    Position p{ a.row + dr,a.column + dc };
    while (p != z) {
        if (!b.IsInside(p) || !b.IsEmpty(p)) return false;
        p.row += dr; p.column += dc;
    }
    return true;
}

bool MoveValidator::Attacks(const ChessBoard& b, Position a, Position z) {
    const Piece p = b.GetPiece(a);
    if (p.type == PieceType::None || a == z) return false;
    int dr = z.row - a.row, dc = z.column - a.column;
    switch (p.type) {
    case PieceType::Pawn: return dr == (p.color == PlayerColor::White ? -1 : 1) && std::abs(dc) == 1;
    case PieceType::Knight: return (std::abs(dr) == 2 && std::abs(dc) == 1) || (std::abs(dr) == 1 && std::abs(dc) == 2);
    case PieceType::Bishop: return std::abs(dr) == std::abs(dc) && ClearPath(b, a, z);
    case PieceType::Rook: return (dr == 0 || dc == 0) && ClearPath(b, a, z);
    case PieceType::Queen: return (dr == 0 || dc == 0 || std::abs(dr) == std::abs(dc)) && ClearPath(b, a, z);
    case PieceType::King: return std::max(std::abs(dr), std::abs(dc)) == 1;
    default: return false;
    }
}

bool MoveValidator::IsSquareAttacked(const ChessBoard& b, Position z, PlayerColor by) {
    for (int r = 0; r < 8; ++r) for (int c = 0; c < 8; ++c) {
        Position a{ r,c }; const Piece p = b.GetPiece(a);
        if (p.type != PieceType::None && p.color == by && Attacks(b, a, z)) return true;
    }
    return false;
}

bool MoveValidator::IsInCheck(const ChessBoard& b, PlayerColor side) {
    for (int r = 0; r < 8; ++r) for (int c = 0; c < 8; ++c) {
        Position p{ r,c }; const Piece piece = b.GetPiece(p);
        if (piece.type == PieceType::King && piece.color == side)
            return IsSquareAttacked(b, p, Opponent(side));
    }
    return true; // A missing king is invalid, not safe.
}

std::vector<Move> MoveValidator::PseudoMoves(const ChessBoard& b, PlayerColor side, Position ep) {
    std::vector<Move> out;
    auto add = [&](Position from, Position to, PieceType type) {
        if (type == PieceType::Pawn && (to.row == 0 || to.row == 7)) {
            for (auto promote : { PieceType::Queen,PieceType::Rook,PieceType::Bishop,PieceType::Knight })
                out.push_back({ from,to,false,promote });
        }
        else out.push_back({ from,to,false,PieceType::None });
        };
    for (int r = 0; r < 8; ++r) for (int c = 0; c < 8; ++c) {
        Position from{ r,c }; Piece piece = b.GetPiece(from);
        if (piece.type == PieceType::None || piece.color != side) continue;
        if (piece.type == PieceType::Pawn) {
            int d = side == PlayerColor::White ? -1 : 1;
            Position one{ r + d,c }, two{ r + 2 * d,c };
            if (b.IsInside(one) && b.IsEmpty(one)) {
                add(from, one, piece.type);
                if (!piece.hasMoved && r == (side == PlayerColor::White ? 6 : 1) && b.IsEmpty(two)) add(from, two, piece.type);
            }
            for (int dc : {-1, 1}) {
                Position to{ r + d,c + dc };
                if (!b.IsInside(to)) continue;
                if (IsEnemy(b, to, side) && b.GetPiece(to).type != PieceType::King) add(from, to, piece.type);
                else if (to == ep && b.IsEmpty(to)) {
                    Position victim{ r,c + dc };
                    if (b.IsInside(victim) && b.GetPiece(victim).type == PieceType::Pawn &&
                        b.GetPiece(victim).color != side) add(from, to, piece.type);
                }
            }
            continue;
        }
        for (int tr = 0; tr < 8; ++tr) for (int tc = 0; tc < 8; ++tc) {
            Position to{ tr,tc };
            if (CanLand(b, to, side) && Attacks(b, from, to)) add(from, to, piece.type);
        }
        if (piece.type == PieceType::King && !piece.hasMoved && c == 4 &&
            r == (side == PlayerColor::White ? 7 : 0) && !IsInCheck(b, side)) {
            for (int rookCol : {0, 7}) {
                Position rookPos{ r,rookCol }; Piece rook = b.GetPiece(rookPos);
                if (rook.type != PieceType::Rook || rook.color != side || rook.hasMoved) continue;
                int d = rookCol == 7 ? 1 : -1;
                bool clear = true;
                for (int x = c + d; x != rookCol; x += d) if (!b.IsEmpty({ r,x })) { clear = false; break; }
                if (clear && !IsSquareAttacked(b, { r,c + d }, Opponent(side)) &&
                    !IsSquareAttacked(b, { r,c + 2 * d }, Opponent(side))) add(from, { r,c + 2 * d }, piece.type);
            }
        }
    }
    return out;
}

void MoveValidator::Apply(ChessBoard& b, const Move& m, Position ep) {
    Piece piece = b.GetPiece(m.from);
    if (piece.type == PieceType::Pawn && m.to == ep && b.IsEmpty(m.to) &&
        m.from.column != m.to.column) b.SetPiece({ m.from.row,m.to.column }, {});
    if (piece.type == PieceType::King && std::abs(m.to.column - m.from.column) == 2) {
        int rookFrom = m.to.column > m.from.column ? 7 : 0;
        int rookTo = m.to.column > m.from.column ? m.to.column - 1 : m.to.column + 1;
        Piece rook = b.GetPiece({ m.from.row,rookFrom }); rook.hasMoved = true;
        b.SetPiece({ m.from.row,rookFrom }, {});
        b.SetPiece({ m.from.row,rookTo }, rook);
    }
    piece.hasMoved = true;
    if (m.promotion != PieceType::None) piece.type = m.promotion;
    b.SetPiece(m.to, piece);
    b.SetPiece(m.from, {});
}

std::vector<Move> MoveValidator::LegalMoves(const ChessBoard& b, PlayerColor side, Position ep) {
    std::vector<Move> out;
    for (const Move& m : PseudoMoves(b, side, ep)) {
        ChessBoard test = b;
        Apply(test, m, ep);
        if (!IsInCheck(test, side)) out.push_back(m);
    }
    return out;
}

bool MoveValidator::IsLegal(const ChessBoard& b, PlayerColor side, const Move& move, Position ep) {
    auto moves = LegalMoves(b, side, ep);
    return std::any_of(moves.begin(), moves.end(), [&](const Move& m) {
        return m.from == move.from && m.to == move.to && m.promotion == move.promotion && !move.enchanted;
        });
}

bool MoveValidator::HasLegalMoves(const ChessBoard& b, PlayerColor side, Position ep) {
    return !LegalMoves(b, side, ep).empty();
}
