#include "EnchantmentManager.h"
#include "MoveValidator.h"
#include <algorithm>
#include <cmath>

namespace {
    int Sign(int n) { return (n > 0) - (n < 0); }
    bool Empty(const ChessBoard& b, Position p) { return b.IsInside(p) && b.IsEmpty(p); }
    bool Enemy(const ChessBoard& b, Position p, PlayerColor c) {
        return b.IsInside(p) && !b.IsEmpty(p) && b.GetPiece(p).color != c && b.GetPiece(p).type != PieceType::King;
    }
    bool Own(const ChessBoard& b, Position p, PlayerColor c) {
        return b.IsInside(p) && !b.IsEmpty(p) && b.GetPiece(p).color == c;
    }
    void Destroy(SpellResult& result, Position p, PlayerColor caster) {
        Piece victim = result.board.GetPiece(p);
        if (victim.type == PieceType::None) return;
        if (victim.color == caster) ++result.friendlyLost;
        else ++result.enemyLost;
        result.board.SetPiece(p, {});
    }
    void Relocate(SpellResult& result, Position from, Position to) {
        Piece p = result.board.GetPiece(from);
        p.hasMoved = true;
        result.board.SetPiece(from, {});
        result.board.SetPiece(to, p);
    }
}
int EnchantmentManager::GetCost(PieceType type) {
    switch (type) {
    case PieceType::Pawn: return 10;
    case PieceType::Bishop: return 20;
    case PieceType::Knight: return 25;
    case PieceType::Rook: return 40;
    default: return -1;
    }
}
bool EnchantmentManager::CanEnchant(const Player& p, PieceType type, int turn) {
    int cost = GetCost(type);
    return cost >= 0 && p.GetMana() >= cost && p.CanEnchant(turn);
}
const char* EnchantmentManager::GetAbilityName(PieceType type, Element element) {
    if (GetCost(type) < 0) return "Unavailable";
    switch (element) {
    case Element::Scorch:
        switch (type) {
        case PieceType::Pawn: return "Forward Capture";
        case PieceType::Bishop: return "Sacrificial Orthogonal Capture";
        case PieceType::Knight: return "Sacrificial Path Capture";
        case PieceType::Rook: return "Destructive Passage";
        default: break;
        } break;
    case Element::River:
        switch (type) {
        case PieceType::Pawn: return "Backward Diagonal";
        case PieceType::Bishop: return "Rightward Capture";
        case PieceType::Knight: return "Flowing Movement";
        case PieceType::Rook: return "Adjacent Friendly Swap";
        default: break;
        } break;
    case Element::Zephyr:
        switch (type) {
        case PieceType::Pawn: return "Sideways Movement";
        case PieceType::Bishop: return "Single-Piece Jump";
        case PieceType::Knight: return "Extended Leap";
        case PieceType::Rook: return "Friendly-Piece Jump";
        default: break;
        } break;
    }
    return "Unavailable";
}

bool EnchantmentManager::TryApply(const ChessBoard& b, const Move& m, PlayerColor side, Element element, SpellResult& out) {
    if (!m.enchanted || !b.IsInside(m.from) || !b.IsInside(m.to) || m.from == m.to) return false;
    Piece piece = b.GetPiece(m.from);
    if (piece.type == PieceType::None || piece.color != side || GetCost(piece.type) < 0) return false;
    if (m.promotion != PieceType::None && m.promotion != PieceType::Queen && m.promotion != PieceType::Rook &&
        m.promotion != PieceType::Bishop && m.promotion != PieceType::Knight) return false;
    const int dr = m.to.row - m.from.row, dc = m.to.column - m.from.column;
    const int ar = std::abs(dr), ac = std::abs(dc);
    const int forward = side == PlayerColor::White ? -1 : 1;
    bool allowed = false, sacrifice = false, swap = false;
    std::vector<Position> victims;
    if (element == Element::Scorch) {
        switch (piece.type) {
        case PieceType::Pawn:
            allowed = dr == forward && dc == 0 && Enemy(b, m.to, side); break;
        case PieceType::Bishop:
            if ((dr == 0 || dc == 0) && Enemy(b, m.to, side)) {
                int sr = Sign(dr), sc = Sign(dc);
                allowed = true;
                for (Position p{ m.from.row + sr,m.from.column + sc }; p != m.to; p.row += sr, p.column += sc)
                    if (!b.IsEmpty(p)) { allowed = false; break; }
                sacrifice = allowed;
            } break;
        case PieceType::Knight:
            if ((ar == 2 && ac == 1) || (ar == 1 && ac == 2)) {
                // Defined path: two squares along the long axis, then one along the short axis.
                Position p = m.from;
                int sr = ar == 2 ? Sign(dr) : 0, sc = ac == 2 ? Sign(dc) : 0;
                for (int i = 0; i < 2; ++i) {
                    p.row += sr; p.column += sc;
                    if (!b.IsEmpty(p)) { if (b.GetPiece(p).type == PieceType::King)return false; victims.push_back(p); }
                }
                if (!b.IsEmpty(m.to)) { if (b.GetPiece(m.to).type == PieceType::King)return false; victims.push_back(m.to); }
                allowed = true; sacrifice = true;
            } break;
        case PieceType::Rook:
            if ((dr == 0 || dc == 0) && !Own(b, m.to, side)) {
                int sr = Sign(dr), sc = Sign(dc); allowed = true;
                for (Position p{ m.from.row + sr,m.from.column + sc }; p != m.to; p.row += sr, p.column += sc) {
                    if (!b.IsEmpty(p)) {
                        if (b.GetPiece(p).type == PieceType::King || b.GetPiece(p).color != side) { allowed = false; break; }
                        victims.push_back(p);
                    }
                }
                if (allowed && !b.IsEmpty(m.to)) { if (b.GetPiece(m.to).type == PieceType::King)return false; victims.push_back(m.to); }
            } break;
        default: break;
        }
    }
    else if (element == Element::River) {
        switch (piece.type) {
        case PieceType::Pawn:
            allowed = dr == -forward && ac == 1 && (Empty(b, m.to) || Enemy(b, m.to, side)); break;
        case PieceType::Bishop:
            // "Right" is relative to the player's forward direction.
            allowed = dr == 0 && dc == (side == PlayerColor::White ? 1 : -1) && Enemy(b, m.to, side); break;
        case PieceType::Knight:
            // Milestone 3 interpretation: straight orthogonal flow, 1-3 clear squares.
            if ((dr == 0 && ac <= 3) || (dc == 0 && ar <= 3)) {
                allowed = Empty(b, m.to);
                for (int i = 1; allowed && i < std::max(ar, ac); ++i)
                    if (!Empty(b, { m.from.row + i * Sign(dr),m.from.column + i * Sign(dc) }))allowed = false;
            } break;
        case PieceType::Rook:
            allowed = ar + ac == 1 && Own(b, m.to, side); swap = allowed; break;
        default: break;
        }
    }
    else if (element == Element::Zephyr) {
        switch (piece.type) {
        case PieceType::Pawn:
            allowed = dr == 0 && ac >= 1 && ac <= 2 && Empty(b, m.to);
            if (allowed && ac == 2 && !Empty(b, { m.from.row,m.from.column + Sign(dc) }))allowed = false;
            break;
        case PieceType::Bishop:
            if (ar == ac && ar > 1 && Empty(b, m.to)) {
                int blockers = 0;
                for (int i = 1; i < ar; ++i) if (!b.IsEmpty({ m.from.row + i * Sign(dr),m.from.column + i * Sign(dc) }))++blockers;
                allowed = blockers == 1;
            } break;
        case PieceType::Knight:
            allowed = ((ar == 3 && ac == 1) || (ar == 1 && ac == 3)) && (Empty(b, m.to) || Enemy(b, m.to, side)); break;
        case PieceType::Rook:
            if ((dr == 0 || dc == 0) && Empty(b, m.to)) {
                int blockers = 0; allowed = true;
                for (int i = 1; i < std::max(ar, ac); ++i) {
                    Position p{ m.from.row + i * Sign(dr),m.from.column + i * Sign(dc) };
                    if (!b.IsEmpty(p)) {
                        if (b.GetPiece(p).color != side) { allowed = false; break; }
                        ++blockers;
                    }
                }
                allowed = allowed && blockers >= 1;
            } break;
        default: break;
        }
    }
    if (!allowed) return false;
    if (piece.type == PieceType::Pawn && (m.to.row == 0 || m.to.row == 7)) {
        if (m.promotion == PieceType::None) return false;
    }
    else if (m.promotion != PieceType::None) return false;
    out = { b,0,0 };
    if (swap) {
        Piece other = out.board.GetPiece(m.to); other.hasMoved = true;
        out.board.SetPiece(m.to, {}); Relocate(out, m.from, m.to);
        out.board.SetPiece(m.from, other);
    }
    else {
        for (Position p : victims)Destroy(out, p, side);
        if (!out.board.IsEmpty(m.to))Destroy(out, m.to, side);
        Relocate(out, m.from, m.to);
        if (sacrifice)Destroy(out, m.to, side);
        else if (m.promotion != PieceType::None) {
            Piece promoted = out.board.GetPiece(m.to); promoted.type = m.promotion; out.board.SetPiece(m.to, promoted);
        }
    }
    // Magic cannot check the king through its movement pattern; only the final vanilla attacks count.
    return !MoveValidator::IsInCheck(out.board, side);
}
std::vector<Move> EnchantmentManager::LegalMoves(const ChessBoard& b, const Player& player, int ownTurn) {
    std::vector<Move> moves;
    if (!player.CanEnchant(ownTurn))return moves;
    for (int r = 0; r < 8; ++r)for (int c = 0; c < 8; ++c) {
        Position from{ r,c }; Piece p = b.GetPiece(from);
        if (p.type == PieceType::None || p.color != player.GetColor() || !CanEnchant(player, p.type, ownTurn))continue;
        for (int tr = 0; tr < 8; ++tr)for (int tc = 0; tc < 8; ++tc) {
            Position to{ tr,tc };
            for (PieceType promotion : (p.type == PieceType::Pawn && (tr == 0 || tr == 7))
                ? std::vector<PieceType>{PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight}
            : std::vector<PieceType>{ PieceType::None }) {
                Move m{ from,to,true,promotion }; SpellResult result;
                if (TryApply(b, m, player.GetColor(), player.GetElement(), result))moves.push_back(m);
            }
        }
    }
    return moves;
}
