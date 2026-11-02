#pragma once

#include <string>

enum class PieceType { None, Pawn, Knight, Bishop, Rook, Queen, King };
enum class PlayerColor { White, Black };
enum class Element { Scorch, River, Zephyr };

struct Position
{
    int row = 0;
    int column = 0;
};

struct Piece
{
    PieceType type = PieceType::None;
    PlayerColor color = PlayerColor::White;
    bool hasMoved = false;
};

struct Move
{
    Position from;
    Position to;
    bool enchanted = false;
};

inline std::string ToString(Element element)
{
    switch (element)
    {
    case Element::Scorch: return "Scorch";
    case Element::River: return "River";
    case Element::Zephyr: return "Zephyr";
    }
    return "Unknown";
}

inline std::string ToString(PlayerColor color)
{
    return color == PlayerColor::White ? "White" : "Black";
}

inline std::string ToString(PieceType type)
{
    switch (type)
    {
    case PieceType::Pawn: return "Pawn";
    case PieceType::Knight: return "Knight";
    case PieceType::Bishop: return "Bishop";
    case PieceType::Rook: return "Rook";
    case PieceType::Queen: return "Queen";
    case PieceType::King: return "King";
    default: return "Empty";
    }
}
