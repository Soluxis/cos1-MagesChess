#pragma once
#include <string>

enum class PieceType { None, Pawn, Knight, Bishop, Rook, Queen, King };
enum class PlayerColor { White, Black };
enum class Element { Scorch, River, Zephyr };

struct Position {
    int row = 0;
    int column = 0;
};
inline bool operator==(Position a, Position b) { return a.row == b.row && a.column == b.column; }
inline bool operator!=(Position a, Position b) { return !(a == b); }
inline PlayerColor Opponent(PlayerColor c) { return c == PlayerColor::White ? PlayerColor::Black : PlayerColor::White; }

struct Piece {
    PieceType type = PieceType::None;
    PlayerColor color = PlayerColor::White;
    bool hasMoved = false;
};
struct Move {
    Position from;
    Position to;
    bool enchanted = false; // Reserved for Milestone Three.
    PieceType promotion = PieceType::None;
};
inline std::string ToString(Element e) {
    switch (e) { case Element::Scorch: return "Scorch"; case Element::River: return "River"; case Element::Zephyr: return "Zephyr"; }
                                     return "Unknown";
}
inline std::string ToString(PlayerColor c) { return c == PlayerColor::White ? "White" : "Black"; }
inline std::string ToString(PieceType p) {
    switch (p) { case PieceType::Pawn: return "Pawn"; case PieceType::Knight: return "Knight"; case PieceType::Bishop: return "Bishop"; case PieceType::Rook: return "Rook"; case PieceType::Queen: return "Queen"; case PieceType::King: return "King"; default: return "Empty"; }
}
inline std::string SquareName(Position p) { return std::string(1, char('a' + p.column)) + std::to_string(8 - p.row); }
